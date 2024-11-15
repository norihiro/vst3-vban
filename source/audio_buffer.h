#pragma once

#include <cstdint>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

struct audio_packet
{
	std::vector<uint8_t> data;
	uint32_t n_channels;
	uint32_t n_samples;

	void copy_float(void **data, uint32_t n_channels, uint32_t n_samples) noexcept;

	inline void swap(struct audio_packet &x) noexcept
	{
		std::swap(n_channels, x.n_channels);
		std::swap(n_samples, x.n_samples);
		data.swap(x.data);
	}
};

struct audio_buffer
{
	std::queue<audio_packet> q1, q2;
	std::mutex q1_mutex;
	std::condition_variable cond;

	void add_float(void **data, uint32_t n_channels, uint32_t n_samples);
	bool get(audio_packet &pkt);

	void notify();
};
