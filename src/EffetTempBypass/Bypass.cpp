#include "Bypass.h"

void BypassEffect::update() {
    audio_block_t* inL = receiveReadOnly(0);
    audio_block_t* inR = receiveReadOnly(1);

    if (!inL && !inR) return;

    audio_block_t* out = allocate();
    if (!out) {
        if (inL) release(inL);
        if (inR) release(inR);
        return;
    }

    // Buffer float mono pour tout le traitement
    float buffer[AUDIO_BLOCK_SAMPLES];

#if PeakAnalysage
    float sumSquares = 0.0f;
#endif

    // Conversion int16 → float avec preamp
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float inputL = inL ? ((float)inL->data[i] / 32768.0f) : 0.0f;
        buffer[i] = inputL * preampVal;

#if PeakAnalysage
        sumSquares += buffer[i] * buffer[i];
#endif
    }

#if PeakAnalysage
    // RMS instantané du bloc, clampé à [0, 1]
    float rms = sqrtf(sumSquares / AUDIO_BLOCK_SAMPLES);
    if (rms > 1.0f) rms = 1.0f;

    // Lissage exponentiel (smoothing) pour éviter le jitter
    volume_ = volume_ + 0.1f * (rms - volume_);
#endif

    // Chaînage des effets via les 4 slots — dispatch polymorphique
#if UtilEffet && UtilBypassRoutage
    for (int slot = 0; slot < 4; slot++) {
        int id = slots_[slot];
        if (id > 0 && id < NUM_EFFECT_IDS && effects_[id]) {
            effects_[id]->update(buffer, AUDIO_BLOCK_SAMPLES);
        }
    }
#endif

    // Conversion float → int16 et transmission
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float val = buffer[i];
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        out->data[i] = (int16_t)(val * 32767.0f);
    }

    transmit(out, 0);
    release(out);
    if (inL) release(inL);
    if (inR) release(inR);
}
