#include "synth.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <SDL3/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int SAMPLE_RATE = 44100;

// Pentatonic scale intervals (semitones from root): always sounds good
static const int PENTATONIC[] = {0, 2, 4, 7, 9};
static const int PENTATONIC_COUNT = 5;

// Whole-tone scale: dreamy, floaty, no resolution — used during trippy
static const int WHOLE_TONE[] = {0, 2, 4, 6, 8, 10};
static const int WHOLE_TONE_COUNT = 6;

// Chord progression: I → IV → V → I (semitone offsets for root)
static const int CHORD_ROOTS[] = {0, 5, 7, 0};
static const int CHORD_COUNT = 4;

// Convert semitones from A4 to frequency
static float semitone_to_freq(int semitone) {
    return 440.0f * powf(2.0f, semitone / 12.0f);
}

void synth_music_init(MusicState& state) {
    state.root_semitone = 0;    // A
    state.bpm = 120.0f;
    state.beat_time = 0.0;
    state.last_note_idx = 0;
    state.chord_degree = 0;
    state.chord_timer = 0.0;
}

void synth_music_tick(MusicState& state, float dt) {
    state.beat_time += dt;

    // Change chord every 4 beats (2 seconds at 120 BPM)
    state.chord_timer -= dt;
    if (state.chord_timer <= 0.0) {
        state.chord_degree = (state.chord_degree + 1) % CHORD_COUNT;
        state.chord_timer = 4.0 * 60.0 / state.bpm;
    }
}

// Pick a note from the scale relative to current root + chord
// trippy_level > 0 switches to whole-tone scale for dreamier sound
static float pick_note(float hue, int octave, MusicState& music, float trippy_level = 0.0f) {
    int root = music.root_semitone + CHORD_ROOTS[music.chord_degree];

    const int* scale = PENTATONIC;
    int scale_count = PENTATONIC_COUNT;

    // Trippy: switch to whole-tone scale
    if (trippy_level >= 1.0f) {
        scale = WHOLE_TONE;
        scale_count = WHOLE_TONE_COUNT;
    }

    int idx = (int)(hue * scale_count) % scale_count;

    // Bias toward nearby notes from last played (favor stepwise motion)
    int diff = idx - music.last_note_idx;
    if (diff > 2) idx = music.last_note_idx + 1 + (idx % 2);
    else if (diff < -2) idx = music.last_note_idx - 1 - (idx % 2);
    idx = ((idx % scale_count) + scale_count) % scale_count;

    music.last_note_idx = idx;

    int semitone = root + scale[idx] + octave * 12;
    return semitone_to_freq(semitone);
}

// Waveform generators (phase 0-2pi)
static float wave_sine(float phase) {
    return sinf(phase);
}

static float wave_saw(float phase) {
    float t = phase / (2.0f * M_PI);
    return 2.0f * (t - floorf(t + 0.5f));
}

static float wave_square(float phase) {
    return fmodf(phase, 2.0f * M_PI) < M_PI ? 1.0f : -1.0f;
}

static float wave_triangle(float phase) {
    float t = fmodf(phase / (2.0f * M_PI), 1.0f);
    return t < 0.5f ? 4.0f * t - 1.0f : 3.0f - 4.0f * t;
}

// ADSR envelope
static float adsr(const Envelope& env, float t, float duration) {
    float total = env.attack + env.decay;
    float release_start = duration - env.release;

    if (t < env.attack) {
        return t / env.attack; // attack
    } else if (t < total) {
        float decay_t = (t - env.attack) / env.decay;
        return 1.0f - decay_t * (1.0f - env.sustain); // decay
    } else if (t < release_start) {
        return env.sustain; // sustain
    } else if (t < duration) {
        float rel_t = (t - release_start) / env.release;
        return env.sustain * (1.0f - rel_t); // release
    }
    return 0.0f;
}

// One-pole low-pass filter
struct LPFilter {
    float y = 0.0f;
    float process(float x, float alpha) {
        y = alpha * x + (1.0f - alpha) * y;
        return y;
    }
};

// Pitch bend: start sharp and bend down over the attack phase
static float pitch_bend(float base_freq, float pitch_drop_semitones, float t, float attack) {
    if (pitch_drop_semitones <= 0.0f || t > attack) return base_freq;
    float bend = pitch_drop_semitones * (1.0f - t / attack);
    return base_freq * powf(2.0f, bend / 12.0f);
}

// FM Synthesis: bell/metallic
static void gen_fm_bell(std::vector<int16_t>& buf, const SynthParams& p) {
    int n = (int)(SAMPLE_RATE * p.duration);
    buf.resize(n * 2);
    float carrier_phase = 0.0f;
    float mod_phase = 0.0f;
    float sub_phase = 0.0f;

    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = adsr(p.envelope, t, p.duration);
        float freq = pitch_bend(p.frequency, p.pitch_drop, t, p.envelope.attack);
        float mod_freq = freq * p.fm_ratio;

        // FM: modulator modulates carrier phase
        float mod = sinf(mod_phase) * p.fm_depth;
        float sample = sinf(carrier_phase + mod);

        // Add subtle second operator for shimmer
        sample += sinf(carrier_phase * 2.003f + mod * 0.7f) * 0.15f;

        // Sub-bass: pure sine one octave below
        if (p.sub_bass > 0.0f)
            sample += sinf(sub_phase) * p.sub_bass;

        sample *= env * p.volume;
        sample += ((rand() % 1000) / 1000.0f - 0.5f) * p.noise_amount * env;

        int16_t s = (int16_t)(std::clamp(sample, -1.0f, 1.0f) * 32000);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;

        carrier_phase += 2.0f * M_PI * freq * (1.0f + p.detune) / SAMPLE_RATE;
        mod_phase += 2.0f * M_PI * mod_freq / SAMPLE_RATE;
        sub_phase += 2.0f * M_PI * freq * 0.5f / SAMPLE_RATE;
    }
}

// Karplus-Strong: plucked string
static void gen_karplus_strong(std::vector<int16_t>& buf, const SynthParams& p) {
    int n = (int)(SAMPLE_RATE * p.duration);
    buf.resize(n * 2);

    // Ring buffer sized to desired pitch
    int ring_len = std::max(2, (int)(SAMPLE_RATE / p.frequency));
    std::vector<float> ring(ring_len);
    for (int i = 0; i < ring_len; i++) {
        ring[i] = ((rand() % 2000) / 1000.0f - 1.0f); // noise burst
    }

    int ring_pos = 0;
    float damping = 0.996f - p.filter_cutoff * 0.01f; // higher cutoff = brighter

    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = adsr(p.envelope, t, p.duration);

        float sample = ring[ring_pos];

        // Average with next sample (low-pass) and apply damping
        int next = (ring_pos + 1) % ring_len;
        ring[ring_pos] = (ring[ring_pos] + ring[next]) * 0.5f * damping;

        sample *= env * p.volume;

        int16_t s = (int16_t)(std::clamp(sample, -1.0f, 1.0f) * 32000);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;

        ring_pos = (ring_pos + 1) % ring_len;
    }
}

// Filtered sawtooth: warm bass
static void gen_saw_bass(std::vector<int16_t>& buf, const SynthParams& p) {
    int n = (int)(SAMPLE_RATE * p.duration);
    buf.resize(n * 2);
    float phase = 0.0f;
    float phase2 = 0.1f;
    float sub_phase = 0.0f;
    LPFilter filter;

    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = adsr(p.envelope, t, p.duration);
        float freq = pitch_bend(p.frequency, p.pitch_drop, t, p.envelope.attack);

        float sample = wave_saw(phase) * 0.5f + wave_saw(phase2) * 0.3f;
        sample += wave_square(phase * 0.5f) * 0.15f; // sub octave square

        // Pure sub-bass sine
        if (p.sub_bass > 0.0f)
            sample += sinf(sub_phase) * p.sub_bass;

        // Envelope-following filter: opens with attack
        float cutoff = p.filter_cutoff * env;
        float alpha = std::min(1.0f, cutoff * 0.5f);
        sample = filter.process(sample, alpha);

        sample *= env * p.volume;
        sample += ((rand() % 1000) / 1000.0f - 0.5f) * p.noise_amount * env * 0.3f;

        int16_t s = (int16_t)(std::clamp(sample, -1.0f, 1.0f) * 32000);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;

        freq *= (1.0f + p.detune);
        phase += 2.0f * M_PI * freq / SAMPLE_RATE;
        phase2 += 2.0f * M_PI * freq * 1.005f / SAMPLE_RATE;
        sub_phase += 2.0f * M_PI * freq * 0.5f / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        if (phase2 > 2.0f * M_PI) phase2 -= 2.0f * M_PI;
        if (sub_phase > 2.0f * M_PI) sub_phase -= 2.0f * M_PI;
    }
}

// Soft sine pad
static void gen_sine_pad(std::vector<int16_t>& buf, const SynthParams& p) {
    int n = (int)(SAMPLE_RATE * p.duration);
    buf.resize(n * 2);
    float phase = 0.0f;
    float sub_phase = 0.0f;

    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = adsr(p.envelope, t, p.duration);
        float freq = pitch_bend(p.frequency, p.pitch_drop, t, p.envelope.attack);

        // Soft sine with gentle vibrato
        float vibrato = sinf(2.0f * M_PI * 5.0f * t) * 0.003f;
        float sample = sinf(phase) * 0.6f;
        sample += sinf(phase * 2.0f) * 0.15f;
        sample += sinf(phase * 1.498f) * 0.1f;

        if (p.sub_bass > 0.0f)
            sample += sinf(sub_phase) * p.sub_bass;

        sample *= env * p.volume;

        int16_t s = (int16_t)(std::clamp(sample, -1.0f, 1.0f) * 32000);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;

        freq *= (1.0f + p.detune + vibrato);
        phase += 2.0f * M_PI * freq / SAMPLE_RATE;
        sub_phase += 2.0f * M_PI * freq * 0.5f / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        if (sub_phase > 2.0f * M_PI) sub_phase -= 2.0f * M_PI;
    }
}

// Hi-hat: filtered noise burst
static void gen_hihat(std::vector<int16_t>& buf, const SynthParams& p) {
    int n = (int)(SAMPLE_RATE * p.duration);
    buf.resize(n * 2);
    LPFilter hpf;  // we'll use as high-pass by subtracting
    LPFilter lpf;

    // High metallic tone mixed with noise
    float phase = 0.0f;
    float freq = p.frequency;

    for (int i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = adsr(p.envelope, t, p.duration);

        // White noise
        float noise = ((rand() % 2000) / 1000.0f - 1.0f);

        // Band-pass: low-pass then subtract another low-pass (crude high-pass)
        float lp = lpf.process(noise, 0.3f);
        float hp = noise - hpf.process(noise, 0.05f);
        float sample = hp * 0.7f + lp * 0.1f;

        // Add metallic ring (high freq square-ish)
        sample += wave_square(phase) * 0.15f;
        sample += sinf(phase * 1.414f) * 0.1f; // inharmonic for metallic flavor

        sample *= env * p.volume;

        int16_t s = (int16_t)(std::clamp(sample, -1.0f, 1.0f) * 32000);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;

        phase += 2.0f * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
}

// Public API

std::vector<int16_t> synth_generate(const SynthParams& params) {
    // Apply trippy modulation to a copy of params
    SynthParams p = params;
    if (p.trippy_level > 0.0f) {
        float trippy = p.trippy_level;

        // Heavy FM modulation — metallic, alien timbres
        p.fm_depth += trippy * 4.0f;
        if (p.fm_ratio > 0.0f)
            p.fm_ratio += trippy * 0.7f;
        else {
            // Force FM even on non-FM synths
            p.fm_ratio = 1.414f * trippy;
            p.fm_depth = trippy * 3.0f;
        }

        // Wide chorus-like detuning
        p.detune += trippy * 0.03f;

        // More noise texture
        p.noise_amount += trippy * 0.05f;

        // Longer, more spacious
        p.duration += trippy * 0.15f;
        p.envelope.release += trippy * 0.1f;

        // Pitch instability — frequency wobbles
        p.frequency *= 1.0f + sinf(SDL_GetTicks() * 0.003f) * trippy * 0.02f;

        // Sub-bass gets weirder too
        p.sub_bass += trippy * 0.15f;
    }

    std::vector<int16_t> buf;
    switch (p.type) {
        case SynthType::FM_BELL:        gen_fm_bell(buf, p); break;
        case SynthType::KARPLUS_STRONG: gen_karplus_strong(buf, p); break;
        case SynthType::SAW_BASS:       gen_saw_bass(buf, p); break;
        case SynthType::SINE_PAD:       gen_sine_pad(buf, p); break;
        case SynthType::HIHAT:          gen_hihat(buf, p); break;
    }

    // Apply reverb (delay-based) if tracers are active
    if (p.reverb_amount > 0.0f && !buf.empty()) {
        int n = (int)buf.size() / 2;  // stereo sample count
        int delay1 = 3528;   // ~80ms at 44100Hz
        int delay2 = 5292;   // ~120ms
        float feedback = p.reverb_amount * 0.35f;
        float mix = p.reverb_amount * 0.3f;

        // Extend buffer for reverb tail
        int tail_samples = (int)(SAMPLE_RATE * p.reverb_amount * 0.8f);
        int total = n + tail_samples;
        buf.resize(total * 2, 0);

        // Two-tap comb filter reverb (in-place, backwards-safe with delay)
        for (int i = 0; i < total; i++) {
            for (int ch = 0; ch < 2; ch++) {
                int idx = i * 2 + ch;
                float dry = (float)buf[idx];

                float tap1 = 0.0f, tap2 = 0.0f;
                if (i >= delay1) tap1 = (float)buf[(i - delay1) * 2 + ch];
                if (i >= delay2) tap2 = (float)buf[(i - delay2) * 2 + ch];

                float wet = tap1 * feedback + tap2 * feedback * 0.6f;
                float out = dry + wet * mix;
                buf[idx] = (int16_t)std::clamp(out, -32000.0f, 32000.0f);
            }
        }
    }

    return buf;
}

SynthParams synth_from_color(float hue, float sat, float val, float size, int palette,
                              SynthRole role, MusicState& music, float trippy_level) {
    SynthParams p = {};
    p.trippy_level = trippy_level;
    p.frequency = pick_note(hue, 0, music, trippy_level);

    // Use a seed from hue+sat+val+size for consistent-per-object but varied selection
    int variant = (int)(hue * 1000 + sat * 100 + size) % 100;

    switch (role) {
        case SynthRole::BASS: {
            int octave = -2 - (int)std::min(1.0f, size / 150.0f);
            p.frequency = pick_note(hue, octave, music, trippy_level);
            p.sub_bass = 0.4f + sat * 0.4f;
            p.volume = 0.6f + val * 0.2f;

            if (variant < 30) {
                // Deep sine throb
                p.type = SynthType::SINE_PAD;
                p.pitch_drop = 5.0f + size / 20.0f;
                p.envelope = {0.005f, 0.4f, 0.5f, 0.5f};
                p.duration = 0.6f + val * 0.3f;
                p.sub_bass += 0.3f;
            } else if (variant < 60) {
                // Filtered saw growl
                p.type = SynthType::SAW_BASS;
                p.filter_cutoff = 0.05f + sat * 0.2f;
                p.detune = -0.02f;
                p.pitch_drop = 3.0f;
                p.envelope = {0.003f, 0.2f, 0.4f, 0.3f};
                p.duration = 0.4f + val * 0.3f;
            } else if (variant < 80) {
                // FM bass thump
                p.type = SynthType::FM_BELL;
                p.fm_ratio = 0.5f;
                p.fm_depth = 2.0f + sat * 3.0f;
                p.pitch_drop = 8.0f;
                p.envelope = {0.002f, 0.15f, 0.2f, 0.3f};
                p.duration = 0.3f + val * 0.2f;
            } else {
                // Karplus bass pluck
                p.type = SynthType::KARPLUS_STRONG;
                p.filter_cutoff = 0.1f;
                p.pitch_drop = 2.0f;
                p.envelope = {0.001f, 0.3f, 0.3f, 0.4f};
                p.duration = 0.5f + val * 0.3f;
            }
            p.noise_amount = 0.01f;
            break;
        }
        case SynthRole::MID: {
            int octave = (int)(val * 1.5f);
            p.frequency = pick_note(hue, octave, music, trippy_level);
            p.volume = 0.35f + val * 0.15f;

            if (variant < 25) {
                // FM bell chime
                p.type = SynthType::FM_BELL;
                p.fm_ratio = 1.414f + hue * 2.0f;
                p.fm_depth = 1.0f + sat * 2.0f;
                p.envelope = {0.003f, 0.15f, 0.2f, 0.2f};
                p.duration = 0.2f + val * 0.15f;
            } else if (variant < 50) {
                // Plucked string
                p.type = SynthType::KARPLUS_STRONG;
                p.filter_cutoff = 0.2f + sat * 0.4f;
                p.envelope = {0.001f, 0.1f, 0.15f, 0.15f};
                p.duration = 0.15f + val * 0.2f;
            } else if (variant < 75) {
                // Soft sine blip
                p.type = SynthType::SINE_PAD;
                p.envelope = {0.01f, 0.08f, 0.3f, 0.1f};
                p.duration = 0.12f + val * 0.15f;
                p.detune = sat * 0.005f;
            } else {
                // Short filtered saw stab
                p.type = SynthType::SAW_BASS;
                p.filter_cutoff = 0.15f + sat * 0.3f;
                p.envelope = {0.002f, 0.06f, 0.15f, 0.08f};
                p.duration = 0.1f + val * 0.1f;
                p.frequency *= 2.0f;  // up an octave from bass
            }
            p.noise_amount = 0.02f;
            break;
        }
        case SynthRole::HIHAT: {
            p.volume = 0.25f + val * 0.15f;

            if (variant < 40) {
                // Closed hihat — short noise burst
                p.type = SynthType::HIHAT;
                p.frequency = 8000.0f + hue * 4000.0f;
                p.envelope = {0.001f, 0.02f, 0.0f, 0.01f};
                p.duration = 0.03f + sat * 0.02f;
            } else if (variant < 70) {
                // Open hihat — longer noise wash
                p.type = SynthType::HIHAT;
                p.frequency = 6000.0f + hue * 3000.0f;
                p.envelope = {0.001f, 0.06f, 0.1f, 0.05f};
                p.duration = 0.08f + sat * 0.06f;
            } else if (variant < 85) {
                // Metallic click — very short FM ping
                p.type = SynthType::FM_BELL;
                p.frequency = 2000.0f + hue * 3000.0f;
                p.fm_ratio = 5.0f + hue * 3.0f;
                p.fm_depth = 0.5f;
                p.envelope = {0.001f, 0.015f, 0.0f, 0.01f};
                p.duration = 0.025f;
            } else {
                // Rim shot — noise + pitched click
                p.type = SynthType::HIHAT;
                p.frequency = 1500.0f + hue * 2000.0f;
                p.envelope = {0.001f, 0.01f, 0.0f, 0.015f};
                p.duration = 0.03f;
                p.pitch_drop = 6.0f;
            }
            p.noise_amount = 0.7f + sat * 0.3f;
            p.sub_bass = 0.0f;
            p.detune = 0.0f;
            break;
        }
        case SynthRole::GENERAL:
        default: {
            int octave = 1 - (int)std::min(2.0f, size / 60.0f);
            p.frequency = pick_note(hue, octave, music, trippy_level);
            float size_norm = std::min(1.0f, size / 150.0f);
            p.sub_bass = size_norm * 0.4f;
            p.volume = 0.3f + val * 0.2f;

            if (variant < 30) {
                p.type = SynthType::FM_BELL;
                p.fm_ratio = 1.0f + hue * 4.0f;
                p.fm_depth = sat * 3.0f;
                p.envelope = {0.005f, 0.12f, 0.2f, 0.15f};
                p.duration = 0.2f + val * 0.2f;
            } else if (variant < 55) {
                p.type = SynthType::KARPLUS_STRONG;
                p.filter_cutoff = 0.15f + sat * 0.35f;
                p.envelope = {0.001f, 0.15f, 0.2f, 0.2f};
                p.duration = 0.2f + val * 0.2f;
            } else if (variant < 80) {
                p.type = SynthType::SINE_PAD;
                p.envelope = {0.02f, 0.15f, 0.4f, 0.2f};
                p.duration = 0.3f + val * 0.2f;
                p.pitch_drop = size_norm * 3.0f;
            } else {
                p.type = SynthType::SAW_BASS;
                p.filter_cutoff = 0.1f + sat * 0.3f;
                p.envelope = {0.005f, 0.1f, 0.2f, 0.15f};
                p.duration = 0.2f + val * 0.15f;
            }
            p.noise_amount = 0.02f;
            p.detune = hue * 0.01f;
            break;
        }
    }

    // FM defaults
    if (p.type == SynthType::FM_BELL && p.fm_ratio == 0.0f) {
        p.fm_ratio = 2.0f;
        p.fm_depth = 2.0f;
    }

    // Trippy modulation: psychedelic warping
    // (trippy_level and reverb_amount are set by the caller from game state)

    return p;
}

std::vector<int16_t> synth_croak(int palette) {
    SynthParams p = {};
    p.type = SynthType::FM_BELL;
    p.volume = 0.4f;
    p.noise_amount = 0.08f;
    p.detune = 0.0f;

    float base_freq = 250.0f + rand() % 150;

    // Palette shifts character
    switch (palette) {
        case 1: base_freq *= 0.8f; break;  // WARM: deeper
        case 2: base_freq *= 1.3f; break;  // COOL: higher
        case 3: p.noise_amount = 0.15f; break; // NEON: noisier
    }

    p.frequency = base_freq;
    p.fm_ratio = 0.5f;
    p.fm_depth = 3.0f + (rand() % 200) / 100.0f;
    p.duration = 0.08f + (rand() % 80) * 0.001f;
    p.envelope = {0.005f, 0.03f, 0.4f, 0.02f};

    return synth_generate(p);
}

uint8_t* synth_build_wav(const int16_t* samples, int num_samples, int sample_rate, int* out_size) {
    int data_size = num_samples * 2 * sizeof(int16_t); // stereo
    int wav_size = 44 + data_size;
    uint8_t* wav = (uint8_t*)SDL_malloc(wav_size);
    if (!wav) { *out_size = 0; return nullptr; }

    memcpy(wav, "RIFF", 4);
    *(uint32_t*)(wav + 4) = wav_size - 8;
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);
    *(uint32_t*)(wav + 16) = 16;
    *(uint16_t*)(wav + 20) = 1;
    *(uint16_t*)(wav + 22) = 2;
    *(uint32_t*)(wav + 24) = sample_rate;
    *(uint32_t*)(wav + 28) = sample_rate * 4;
    *(uint16_t*)(wav + 32) = 4;
    *(uint16_t*)(wav + 34) = 16;
    memcpy(wav + 36, "data", 4);
    *(uint32_t*)(wav + 40) = data_size;
    memcpy(wav + 44, samples, data_size);

    *out_size = wav_size;
    return wav;
}
