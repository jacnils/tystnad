#include <cstdint>
#include <cstddef>
#include <vector>
#include <wav.hpp>

void apply_fade_in(std::vector<char>& buffer, int in_samples, int channels) {
   	auto* samples = reinterpret_cast<int16_t*>(buffer.data());
   	auto total = buffer.size() / sizeof(int16_t);

   	int frames = std::min(in_samples, static_cast<int>(total / channels));

   	for (int frame = 0; frame < frames; ++frame) {
   		float gain = static_cast<float>(frame) / static_cast<float>(frames);

   		for (int ch = 0; ch < channels; ++ch) {
   			int idx = frame * channels + ch;
   			int16_t sample = samples[idx];

   			float faded = static_cast<float>(sample) * gain;

   			if (faded > 32767.f) faded = 32767.f;
   			if (faded < -32768.f) faded = -32768.f;

   			samples[idx] = static_cast<int16_t>(faded);
   		}
   	}
}

std::vector<char> generate_empty_sound(int duration_seconds, int sample_rate, int num_channels, int bits_per_sample) {
	int byte_rate = sample_rate * num_channels * bits_per_sample / 8;
	int block_align = num_channels * bits_per_sample / 8;
	int num_samples = duration_seconds * sample_rate;
	int data_chunk_size = num_samples * block_align;
	int fmt_chunk_size = 16;
	int audio_format = 1; // PCM
	int riff_chunk_size = 4 + (8 + fmt_chunk_size) + (8 + data_chunk_size);

	std::vector<char> buffer;

	auto write_bytes = [&](const void* data, size_t size) {
		const char* bytes = static_cast<const char*>(data);
		buffer.insert(buffer.end(), bytes, bytes + size);
	};

	auto write_le32 = [&](uint32_t val) {
		buffer.push_back(static_cast<char>(val & 0xFF));
		buffer.push_back(static_cast<char>((val >> 8) & 0xFF));
		buffer.push_back(static_cast<char>((val >> 16) & 0xFF));
		buffer.push_back(static_cast<char>((val >> 24) & 0xFF));
	};

	auto write_le16 = [&](uint16_t val) {
		buffer.push_back(static_cast<char>(val & 0xFF));
		buffer.push_back(static_cast<char>((val >> 8) & 0xFF));
	};

	write_bytes("RIFF", 4);
	write_le32(riff_chunk_size);
	write_bytes("WAVE", 4);

	write_bytes("fmt ", 4);
	write_le32(fmt_chunk_size);
	write_le16(audio_format);
	write_le16(num_channels);
	write_le32(sample_rate);
	write_le32(byte_rate);
	write_le16(block_align);
	write_le16(bits_per_sample);

	write_bytes("data", 4);
	write_le32(data_chunk_size);

	buffer.insert(buffer.end(), data_chunk_size, 0);

	return buffer;
}