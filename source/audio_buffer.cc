#include <cstring>

#include "audio_buffer.h"

void audio_packet::copy_float(void **data_, uint32_t n_channels_, uint32_t n_samples_) noexcept
{
	n_channels = n_channels_;
	n_samples = n_samples_;

	data.resize(sizeof(float) * n_channels_ * n_samples_);

	for (uint32_t i_channel = 0; i_channel < n_channels_; i_channel++) {
		uint8_t *start = static_cast<uint8_t *>(data_[i_channel]);
		memcpy(data.data() + sizeof(float) * n_samples_ * i_channel, start, sizeof(float) * n_samples_);
	}
}

void audio_buffer::add_float(void **data, uint32_t n_channels, uint32_t n_samples)
{
	std::unique_lock q1_lock(q1_mutex, std::try_to_lock);

	if (q1_lock) {
		while (q2.size()) {
			auto &pkt = q1.emplace();
			pkt.swap(q2.front());
			q2.pop();
		}

		auto &pkt = q1.emplace();
		pkt.copy_float(data, n_channels, n_samples);

		cond.notify_one();
	} else {
		auto &pkt = q2.emplace();
		pkt.copy_float(data, n_channels, n_samples);
	}
}

bool audio_buffer::get(audio_packet &pkt)
{
	std::unique_lock q1_lock(q1_mutex);

	if (!q1.size())
		return false;

	pkt.swap(q1.front());
	q1.pop();
	return true;
}

void audio_buffer::notify()
{
	std::unique_lock q1_lock(q1_mutex);
	cond.notify_one();
}
