#ifndef _SYNTH_H_
#define _SYNTH_H_

#include <vector>
#include <cstdint>

// Synthesis method
enum class SynthType {
    FM_BELL,        // FM synthesis — bell/metallic tones
    KARPLUS_STRONG, // Plucked string
    SAW_BASS,       // Filtered sawtooth — warm bass
    SINE_PAD,       // Soft sine pad
    HIHAT,          // Filtered noise burst — percussive
};

// ADSR envelope parameters
struct Envelope {
    float attack;   // seconds
    float decay;    // seconds
    float sustain;  // level 0-1
    float release;  // seconds
};

// Parameters for a single synthesized note
struct SynthParams {
    SynthType type;
    float frequency;    // Hz
    float duration;     // seconds
    float volume;       // 0-1
    Envelope envelope;

    // FM synthesis
    float fm_ratio;     // modulator:carrier frequency ratio
    float fm_depth;     // modulation depth

    // Sawtooth filter
    float filter_cutoff; // low-pass cutoff 0-1 (fraction of sample rate)

    // General
    float noise_amount; // 0-1
    float detune;       // slight pitch offset factor

    // Bass enhancement
    float sub_bass;     // 0-1, level of sub-oscillator (sine at half freq)
    float pitch_drop;   // semitones to bend down from at attack start (0 = none)

    // Effect modifiers (set by game state)
    float trippy_level; // 0-3, increases FM depth, detune, vibrato
    float reverb_amount; // 0-1, adds delay-based reverb tail (tied to tracers)
};

// Global musical state — keeps everything in key and on beat
struct MusicState {
    int root_semitone;      // root note (semitones from A4, e.g. 0=A, 3=C, 5=D)
    float bpm;              // tempo
    double beat_time;       // accumulated beat time in seconds
    int last_note_idx;      // index into scale of last note played
    int chord_degree;       // current chord degree (0=I, 1=IV, 2=V, etc.)
    double chord_timer;     // seconds until next chord change
};

// Initialize / reset the music state
void synth_music_init(MusicState& state);

// Advance beat clock (call once per frame, pass dt)
void synth_music_tick(MusicState& state, float dt);

// Generate a stereo PCM buffer (44100 Hz, 16-bit)
std::vector<int16_t> synth_generate(const SynthParams& params);

// Role in the mix
enum class SynthRole {
    BASS,     // Ship collisions — deep bass/sub
    HIHAT,    // Duder impacts — percussive noise bursts
    MID,      // Bullet/projectile impacts — mid-range melodic
    GENERAL,  // Generic (launched duder, etc.)
};

// Map object properties to musical synth parameters
// hue: 0-1, sat: 0-1, val: 0-1, size: object radius, palette: 0-4
SynthParams synth_from_color(float hue, float sat, float val, float size, int palette,
                              SynthRole role, MusicState& music, float trippy_level = 0.0f);

// Generate a frog croak sound
std::vector<int16_t> synth_croak(int palette);

// Build a WAV file in memory from PCM samples (caller must SDL_free the result)
uint8_t* synth_build_wav(const int16_t* samples, int num_samples, int sample_rate, int* out_size);

#endif
