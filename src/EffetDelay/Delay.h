#pragma once

#include "../includes/Utils.h"

#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"

enum DelaySubdivision {
    SUBDIV_WHOLE = 0,    // Ronde (4.0)
    SUBDIV_HALF,         // Blanche (2.0)
    SUBDIV_QUARTER,      // Noire (1.0)
    SUBDIV_DOTTED_8TH,   // Croche pointée (0.75)
    SUBDIV_8TH,          // Croche (0.5)
    SUBDIV_16TH          // Double croche (0.25)
};
 
class DelayEffect : public AudioStream{
public:

    static constexpr size_t MAX_DELAY = static_cast<size_t>(44100 * 2.0f);

    // Paramètres internes
    float dryMix = 0.5f;
    float wetMix = 0.5f;
    float volume = 1;
    float vdelayTime = 0.5f;
    float vdelayFDBK = 0.7f;
    float timeFirstTap; 

    // Nouvelles variables pour BPM / Manuel
    int delayMode = 0;           // 0 = Manual, 1 = Tempo
    float manualTimeMs = 500.0f; // Temps en ms si mode manuel
    float currentBPM = 120.0f;   // BPM courant
    DelaySubdivision currentSubdivision = SUBDIV_QUARTER;
    
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
        bool active = false;

        void Init(float sampleRate, uint32_t max_delay_samples);
        void Free();

        float Process(float in);
    };

    DelayChannel delay;

    DelayEffect();
    ~DelayEffect();
    bool begin();

    virtual void update() override;

    // ON/OFF soft
    void setEnabled(bool e) { active = e; }
    bool isEnabled() const { return active; }


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
    float getMix() {return wetMix;};

    void setParameter(int param_id, float value);
private:
    bool active = false;
    audio_block_t* inputQueueArray_[1];
};