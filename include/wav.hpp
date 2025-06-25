#pragma once

#include <vector>

void apply_fade_in(std::vector<char>& buffer, int in_samples, int channels);
std::vector<char> generate_empty_sound(int duration_seconds, int sample_rate = 44100,
	int num_channels = 2, int bits_per_sample = 16);