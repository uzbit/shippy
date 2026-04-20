#include <SDL3/SDL.h>
#include <mutex>
#include <string>
#include <vector>

#include "z_libpd.h"

#include "pdaudio.h"
#include "platform.h"

static SDL_AudioStream* g_stream = nullptr;
static void* g_patch = nullptr;
static std::mutex g_pd_mutex;
static constexpr int PD_OUT_CHANNELS = 2;
static constexpr int PD_SAMPLE_RATE = 44100;

static void SDLCALL audio_callback(void* /*ud*/, SDL_AudioStream* stream,
                                    int additional_amount, int /*total_amount*/) {
    if (additional_amount <= 0) return;

    const int block_size = libpd_blocksize(); // 64 samples by default
    const int bytes_per_frame = PD_OUT_CHANNELS * (int)sizeof(float);
    int frames_needed = additional_amount / bytes_per_frame;
    int ticks = (frames_needed + block_size - 1) / block_size;
    int out_samples = ticks * block_size * PD_OUT_CHANNELS;

    std::vector<float> out_buf(out_samples);
    {
        std::lock_guard<std::mutex> lock(g_pd_mutex);
        libpd_process_float(ticks, nullptr, out_buf.data());
    }
    SDL_PutAudioStreamData(stream, out_buf.data(),
                           ticks * block_size * bytes_per_frame);
}

static void pd_print_hook(const char* s) {
    SDL_Log("pd: %s", s);
}

bool pd_init() {
    libpd_set_printhook(pd_print_hook);
    libpd_init();
    libpd_init_audio(0, PD_OUT_CHANNELS, PD_SAMPLE_RATE);

    // Turn on DSP: send [; pd dsp 1(
    libpd_start_message(1);
    libpd_add_float(1.0f);
    libpd_finish_message("pd", "dsp");

    std::string patch_full = asset_path("shippy.pd");
    size_t slash = patch_full.find_last_of('/');
    std::string dir = (slash != std::string::npos)
        ? patch_full.substr(0, slash) : std::string(".");
    std::string name = (slash != std::string::npos)
        ? patch_full.substr(slash + 1) : patch_full;

    // Register rjlib's abstractions directory so [s_hstr], [s_chip], etc. resolve.
    std::string rj_path = dir + "/rjlib/rj";
    libpd_add_to_search_path(rj_path.c_str());

    g_patch = libpd_openfile(name.c_str(), dir.c_str());
    if (!g_patch) {
        SDL_Log("pd_init: libpd_openfile failed for %s in %s",
                name.c_str(), dir.c_str());
        return false;
    }

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = PD_OUT_CHANNELS;
    spec.freq = PD_SAMPLE_RATE;

    g_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, nullptr);
    if (!g_stream) {
        SDL_Log("pd_init: SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        return false;
    }
    SDL_ResumeAudioStreamDevice(g_stream);
    SDL_Log("pd_init: libpd running (%d Hz, %d ch, block=%d)",
            PD_SAMPLE_RATE, PD_OUT_CHANNELS, libpd_blocksize());
    return true;
}

void pd_shutdown() {
    if (g_stream) {
        SDL_DestroyAudioStream(g_stream);
        g_stream = nullptr;
    }
    if (g_patch) {
        std::lock_guard<std::mutex> lock(g_pd_mutex);
        libpd_closefile(g_patch);
        g_patch = nullptr;
    }
}

void pd_send_float(const char* recv, float f) {
    std::lock_guard<std::mutex> lock(g_pd_mutex);
    libpd_float(recv, f);
}

void pd_send_bang(const char* recv) {
    std::lock_guard<std::mutex> lock(g_pd_mutex);
    libpd_bang(recv);
}
