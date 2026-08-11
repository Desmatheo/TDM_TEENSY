#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"

#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"

class DelayEffect 
#if !UtilBypassRoutage
: public AudioStream
#else
: public Effect
#endif
{
public:

    static constexpr size_t MAX_DELAY = static_cast<size_t>(44100 * 4.0f);

    DelayEffect();
    ~DelayEffect();
    bool begin();

#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else 
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif

    // Setters pour personnalisation
    void setMix(float mix);
    void setVolume(float vol);
    void setDelayMode(float mode);
    void setDelayTime(float time);
    void setDelayTimeTap(float time);
    void setFeedback(float fdbk);
    void setBpm(float bpm);
    void setBpmTap(float time);
    void setSubdivision(float value);
    float getMix() {return wetMix;}

    void setParameter(int param_id, float value);

private:
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[1];
#endif

    // Paramètres internes
    float dryMix = 0.5f;
    float wetMix = 0.5f;
    float volume = 1;
    float vdelayFDBK = 0.7f;

    // Variables pour BPM / Manuel
    int delayMode = 0;           // 0 = Manual, 1 = Tempo
    float manualTimeMs = 500.0f; // Temps en ms si mode manuel
    float currentBPM = 120.0f;   // BPM courant
    float currentSubdivisionMult = 1.0f; // Multiplicateur rythmique (1.0 = Noire)

    void recalculateDelayTime();

    // Structure interne pour gérer le buffer de delay
    struct DelayChannel {
        float* buffer = nullptr;
        uint32_t buf_len = 0;
        uint32_t write_idx = 0;
        bool use_extmem = false;
        
        float tone_z1 = 0.0f;
        float tone_a0 = 1.0f;
        float tone_b1 = 0.0f;

        float muteFade = 1.0f;
        uint32_t standbyTimer = 0;
        float lastTarget = 0.0f;
        float currentDelay = 0.0f;
        float delayTarget = 0.0f;
        float feedback = 0.0f;
        bool active = true;

        void Init(float sampleRate, uint32_t max_delay_samples);
        void Free();

        float Process(float in);
    };

    DelayChannel delay;
};