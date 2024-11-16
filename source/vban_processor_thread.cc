#include <algorithm>
#include <chrono>
#include "vban.h"
#include "vban_processor.h"
#include "socket.h"

namespace NagaterNet {

void CVBANPluginProcessor::thread_start()
{
	cont = true;
	has_error = false;
	pthread_create(&thread, NULL, CVBANPluginProcessor::thread_entry, this);
}

void CVBANPluginProcessor::thread_stop()
{
	cont = false;
	packets.notify();
	pthread_join(thread, NULL);
}

struct loop_context
{
	union {
		uint8_t vban_packet[VBAN_PROTOCOL_MAX_SIZE];
		VBanHeader vban_header;
	};

	inline uint8_t *vban_payload()
	{
		return vban_packet + VBAN_HEADER_SIZE;
	}

	uint32_t vban_packet_frames;
	uint32_t vban_channels;
	uint32_t vban_sample_bytes;
	uint32_t vban_frame_bytes; /* = vban_sample_bytes * vban_channels */

	std::vector<uint8_t> interleaved_audio;
	std::chrono::steady_clock::time_point next_send;
	uint32_t last_packet_size;
	bool send_soon = false;

	socket_t vban_socket;

	loop_context()
	{
		vban_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

	~loop_context()
	{
		closesocket(vban_socket);
	}
};

bool CVBANPluginProcessor::thread_loop_init(struct loop_context &ctx)
{
	memcpy(&ctx.vban_header.vban, "VBAN", 4);

	int32_t sr_req = (int32_t)(processSetup.sampleRate + 0.5);
	bool sr_found = false;
	for (uint8_t isr = 0; isr < VBAN_SR_MAXNUMBER; isr++) {
		if (std::abs(VBanSRList[isr] - sr_req) < 10) {
			ctx.vban_header.format_SR = isr | VBAN_PROTOCOL_AUDIO;
			sr_found = true;
			break;
		}
	}
	if (!sr_found) {
		fprintf(stderr, "Error: VBAN cannot send the requested sample rate %d Hz\n", sr_req);
		return false;
	}

	ctx.vban_sample_bytes = 4; // 32-bit float
	ctx.vban_channels = 2;
	ctx.vban_frame_bytes = ctx.vban_sample_bytes * ctx.vban_channels;
	ctx.vban_packet_frames = std::min(256u, VBAN_DATA_MAX_SIZE / ctx.vban_frame_bytes);

	ctx.vban_header.format_nbc = ctx.vban_channels - 1; // Stereo only // TODO: Allow other settings
	ctx.vban_header.format_bit = VBAN_BITFMT_32_FLOAT;
	strncpy(ctx.vban_header.streamname, "VST3", VBAN_STREAM_NAME_SIZE); // TODO: Set name

	ctx.vban_header.format_nbs = (uint8_t)(ctx.vban_packet_frames - 1);

	ctx.next_send = std::chrono::steady_clock::now();
	ctx.send_soon = true;

	return true;
}

static void copy_packet_to_buffer(struct loop_context &ctx, std::vector<uint8_t> &dst, struct audio_packet &pkt)
{
	const uint32_t n_samples = pkt.n_samples;

	dst.reserve(dst.size() + n_samples * ctx.vban_frame_bytes);

	for (uint32_t i = 0; i < n_samples; i++) {
		for (uint32_t ch = 0; ch < ctx.vban_channels; ch++) {
			uint8_t *begin = pkt.data.data() + (i + ch * n_samples) * ctx.vban_sample_bytes;
			dst.insert(dst.end(), begin, begin + ctx.vban_sample_bytes);
		}
	}
}

bool CVBANPluginProcessor::thread_loop_obtain_from_queue(struct loop_context &ctx)
{
	bool cont_local;
	bool received = false;

	{
		std::unique_lock q1_lock(packets.q1_mutex);
		cont_local = cont;

		if (!cont_local)
			return false;

		if (ctx.send_soon)
			packets.cond.wait_for(q1_lock, std::chrono::milliseconds(2));
		else
			packets.cond.wait_until(q1_lock, ctx.next_send);

		while (packets.q1.size() && ((cont_local = cont))) {
			struct audio_packet pkt;
			pkt.swap(packets.q1.front());
			packets.q1.pop();

			q1_lock.unlock();

			if (ctx.vban_header.format_nbc + 1 != pkt.n_channels)
				return false;

			ctx.last_packet_size = pkt.data.size();
			copy_packet_to_buffer(ctx, ctx.interleaved_audio, pkt);

			received = true;

			q1_lock.lock();
		}
	}

	if (received) {
		if (ctx.send_soon)
			/* Make the next timeout faster */
			ctx.next_send = std::chrono::steady_clock::now();
		else
			return thread_loop_obtain_from_queue(ctx);
	}

	return cont_local;
}

uint32_t CVBANPluginProcessor::thread_loop_send(struct loop_context &ctx)
{
	uint32_t payload_bytes = ctx.vban_packet_frames * ctx.vban_frame_bytes;

	if (ctx.interleaved_audio.size() < payload_bytes)
		return 0;

	memcpy(ctx.vban_payload(), ctx.interleaved_audio.data(), payload_bytes);
	ctx.interleaved_audio.erase(ctx.interleaved_audio.begin(), ctx.interleaved_audio.begin() + payload_bytes);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	{
		std::unique_lock lk(props_mutex);
		addr.sin_addr.s_addr = htonl(dest_addr);
		addr.sin_port = htons(dest_port);
	}

	if (addr.sin_addr.s_addr) {
		int ret = sendto(ctx.vban_socket, ctx.vban_packet, VBAN_HEADER_SIZE + payload_bytes, 0,
				 (struct sockaddr *)&addr, (socklen_t)sizeof(addr));
		if (ret != VBAN_HEADER_SIZE + payload_bytes)
			fprintf(stderr, "Error: Failed to send VBAN packet. errno=%d\n", errno);
	}

	ctx.vban_header.nuFrame++;

	return ctx.vban_packet_frames;
}

void CVBANPluginProcessor::thread_loop()
{
	struct loop_context ctx;

	if (!thread_loop_init(ctx)) {
		has_error = true;
		return;
	}

	double sample_us = 1e6 / processSetup.sampleRate;
	bool first = true;

	while (thread_loop_obtain_from_queue(ctx)) {

		if (!ctx.last_packet_size) {
			ctx.send_soon = true;
			continue;
		}

		uint32_t upper_buffer_bytes = ctx.last_packet_size * 2;
		while (upper_buffer_bytes < ctx.last_packet_size + ctx.vban_packet_frames * ctx.vban_frame_bytes)
			upper_buffer_bytes += ctx.last_packet_size;

		if (first) {
			/* Wait until enough packets have arrived. */
			if (ctx.interleaved_audio.size() < upper_buffer_bytes) {
				ctx.send_soon = true;
				continue;
			}

			ctx.send_soon = false;
			first = false;
		}

		uint32_t peak_buffer_size = ctx.interleaved_audio.size();

		uint32_t n_frames = thread_loop_send(ctx);

		if (n_frames) {
			double duration_us = n_frames * sample_us;
			if (ctx.interleaved_audio.size() < ctx.last_packet_size) {
				/* If the are small number of remaining samples, add 1% or 1us to the wait time. */
				duration_us = duration_us < 1e2 ? duration_us + 1.0 : duration_us * 1.01;
			} else if (peak_buffer_size > upper_buffer_bytes) {
				/* If the are large number of remaining samples, subtract 1% or 1us to the wait time. */
				duration_us = duration_us < 1e2 ? duration_us - 1.0 : duration_us * 0.99;
			}
			ctx.next_send += std::chrono::microseconds((int)duration_us);
			ctx.send_soon = false;
		} else {
			ctx.send_soon = true;
		}
	}
}

void *CVBANPluginProcessor::thread_entry(void *data)
{
	auto ptr = static_cast<CVBANPluginProcessor *>(data);

	while (ptr->cont && !ptr->has_error)
		ptr->thread_loop();

	return NULL;
}
}
