#pragma once

#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"
#include "../includes/Utils.h"


class BypassEffect : public AudioStream {
public:
    BypassEffect() : AudioStream(2, inputQueueArray_) {}

    int StringIndex;
    float preampVal;

    void setStringIndex(int index) { 
        StringIndex = index;
        switch (StringIndex) {
            case 0:
                preampVal = 0.75f * 10.0f;
                break;
            case 1:
                preampVal = 0.75f * 14.0f * 2.0f;
                break;
            case 2:
                preampVal = 0.75f * 12.0f;
                break;
            case 3:
                preampVal = 0.75f * 12.0f;
                break;
            case 4:
                preampVal = 0.75f * 14.0f;
                break;
            case 5:
                preampVal = 0.75f * 12.0f * 2.0f;
                break;
            default: 
                preampVal = 0.75f * 14.0f;
                break;
        }
    }

#if PeakAnalysage
    // Retourne le volume RMS lissé de cette corde (0.0 à 1.0)
    float getVolume() const { return volume_; }
#endif

    virtual void update() override {
        audio_block_t* inL = receiveReadOnly(0);
        audio_block_t* inR = receiveReadOnly(1);

        if (!inL && !inR) return;

        audio_block_t* outL = allocate();
        audio_block_t* outR = allocate();

        if (!outL || !outR) {
            if (outL) release(outL);
            if (outR) release(outR);
            if (inL) release(inL);
            if (inR) release(inR);
            return;
        }

#if PeakAnalysage
        // Calcul RMS sur le bloc d'entrée (canal L)
        float sumSquares = 0.0f;
#endif

        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            // passage en flottants
            float inputL = inL ? ((float)inL->data[i] / 32768.0f) : 0.0f;
            float inputR = inR ? ((float)inR->data[i] / 32768.0f) : inputL; 


            float outputL = (inputL * preampVal);
            float outputR = (inputR * preampVal);

#if PeakAnalysage
            sumSquares += outputL * outputR;
#endif

            // float treshold = 0.003f;
            // float rate = 0.5f;

            // if (inputL < treshold && inputL > -treshold){
            //     outputL = (inputL * 10.0f) * rate;
            //     outputR = (inputR * 10.0f) * rate;
            // }
            // else{
            //     outputL = (inputL * 10.0f);
            //     outputR = (inputR * 10.0f);
            // }

            // retour en entier
            outL->data[i] = (int16_t)(outputL * 32767.0f);
            outR->data[i] = (int16_t)(outputR * 32767.0f);
        }

#if PeakAnalysage
        // RMS instantané du bloc, clampé à [0, 1]
        float rms = sqrtf(sumSquares / AUDIO_BLOCK_SAMPLES);
        if (rms > 1.0f) rms = 1.0f;

        // Lissage exponentiel (smoothing) pour éviter le jitter
        // alpha = 0.1 → réactif mais stable
        volume_ = volume_ + 0.1f * (rms - volume_);
#endif

        transmit(outL, 0);
        transmit(outR, 1);
        
        release(outL);
        release(outR);
        if (inL) release(inL);
        if (inR) release(inR);
    }

private:
    audio_block_t* inputQueueArray_[2];
#if PeakAnalysage
    float volume_ = 0.0f;  // Volume RMS lissé (0.0 à 1.0)
#endif
};