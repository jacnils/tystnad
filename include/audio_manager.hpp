#pragma once

#ifdef MACOS
#include <AudioToolbox/AudioToolbox.h>
#endif
#ifdef LINUX
#include <alsa/asoundlib.h>
#endif
#include <fstream>
#include <vector>
#include <thread>
#include <filesystem>
#include <audio_manager.hpp>
#include <stdexcept>
#include <cstring>

#ifdef MACOS
class audio_manager {
    AudioQueueRef queue{};
    AudioStreamBasicDescription format{};
    std::vector<char> buffer;
    size_t offset = 0;

    static void AQCallback(void* data, AudioQueueRef aq, AudioQueueBufferRef buf) {
    	auto* player = static_cast<audio_manager*>(data);

    	size_t bytes = std::min(
			static_cast<size_t>(buf->mAudioDataBytesCapacity),
			player->buffer.size() - player->offset
		);

        if (bytes > 0) {
            memcpy(buf->mAudioData, player->buffer.data() + player->offset, bytes);
            buf->mAudioDataByteSize = static_cast<uint32_t>(bytes);
            player->offset += bytes;

            AudioQueueEnqueueBuffer(aq, buf, 0, nullptr);
        } else {
            AudioQueueStop(aq, false);
        }
    }
public:
    bool init(const std::vector<char>& data) {
        buffer = data;
        offset = 0;

        format.mSampleRate = 44100;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        format.mBitsPerChannel = 16;
        format.mChannelsPerFrame = 2;
        format.mBytesPerPacket = format.mBytesPerFrame = (format.mBitsPerChannel / 8) * format.mChannelsPerFrame;
        format.mFramesPerPacket = 1;
        format.mReserved = 0;

        OSStatus status = AudioQueueNewOutput(&format, AQCallback, this, nullptr, nullptr, 0, &queue);
        if (status != noErr) {
            throw std::runtime_error{"AudioQueueNewOutput failed: " + std::to_string(status)};
        }

        static constexpr int buf_num = 3;
        for (int i = 0; i < buf_num; i++) {
            AudioQueueBufferRef buf;
            status = AudioQueueAllocateBuffer(queue, 4096, &buf);
            if (status != noErr) {
            	throw std::runtime_error{"AudioQueueAllocateBuffer failed: " + std::to_string(status)};
            }
            AQCallback(this, queue, buf);
        }

        status = AudioQueueStart(queue, nullptr);
        if (status != noErr) {
        	throw std::runtime_error{"AudioQueueStart failed: " + std::to_string(status)};
        }
        return true;
    }
	bool init(const std::string& file_path) {
    	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    	if (!file) {
    		throw std::runtime_error("Failed to open file: " + file_path);
    	}

    	std::streamsize size = file.tellg();
    	file.seekg(0, std::ios::beg);

    	std::vector<char> buffer(size);
    	if (!file.read(buffer.data(), size)) {
    		throw std::runtime_error("Failed to read file: " + file_path);
    	}

    	return this->init(buffer);
    }
	audio_manager() = default;
	explicit audio_manager(const std::vector<char>& data) {
		this->init(data);
	}
	explicit audio_manager(const std::string& file_path) {
		this->init(file_path);
	}

    void wait_until_done() const {
        while (offset < buffer.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

	void stop() const {
    	AudioQueueStop(queue, true);
    	AudioQueueDispose(queue, true);
    }

    ~audio_manager() {
    	this->stop();
    }
};
#endif
#if LINUX
struct audio_manager {
    snd_pcm_t* pcm_handle = nullptr;
    snd_pcm_hw_params_t* hw_params = nullptr;
    std::vector<char> buffer;
    size_t offset = 0;
    unsigned int channels = 2;
    unsigned int rate = 44100;
    snd_pcm_uframes_t period_size = 0;

    bool init(const std::vector<char>& data, const std::string& alsa_sink = "default") {
        buffer = data;
        offset = 0;

        int err;

        if ((err = snd_pcm_open(&pcm_handle, alsa_sink.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            throw std::runtime_error{std::string{"snd_pcm_open failed: "} + snd_strerror(err)};
        }

        snd_pcm_hw_params_malloc(&hw_params);
        snd_pcm_hw_params_any(pcm_handle, hw_params);

        snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
        snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, nullptr);

        snd_pcm_hw_params_get_period_size(hw_params, &period_size, nullptr);

        if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0) {
            throw std::runtime_error{"snd_pcm_hw_params failed"};
        }

        snd_pcm_hw_params_free(hw_params);
        hw_params = nullptr;

        const size_t frame_size = channels * sizeof(int16_t);
        const size_t chunk_bytes = period_size * frame_size;

        while (offset < buffer.size()) {
            size_t remaining = buffer.size() - offset;
            size_t write_size = std::min(chunk_bytes, remaining);
            int frames = write_size / frame_size;

            int written = snd_pcm_writei(pcm_handle, buffer.data() + offset, frames);
            if (written < 0) {
                if (written == -EPIPE) {
                    snd_pcm_prepare(pcm_handle);
                    continue;
                } else {
                    throw std::runtime_error{std::string{"snd_pcm_writei failed: "} + snd_strerror(written)};
                }
            }
            offset += written * frame_size;
        }

	/*
        std::vector<char> silence(chunk_bytes, 0);
        for (int i = 0; i < 4; ++i) {
            snd_pcm_writei(pcm_handle, silence.data(), period_size);
        }
	*/

        return true;
    }

    bool init(const std::string& file_path, const std::string& sink = "default") {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("Failed to read file: " + file_path);
        }

        return this->init(buffer, sink);
    }

    audio_manager() = default;
    audio_manager(const std::vector<char>& data) { this->init(data); }
    audio_manager(const std::string& file_path) { this->init(file_path); }

    void wait_until_done() const {
        snd_pcm_sframes_t delay = 0;
        while (true) {
            if (snd_pcm_delay(pcm_handle, &delay) < 0) break;
            if (delay <= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    void stop() {
        if (pcm_handle) {
            snd_pcm_drain(pcm_handle);
            snd_pcm_close(pcm_handle);
            pcm_handle = nullptr;
        }
    }

    ~audio_manager() {
        stop();
    }
};
#endif
