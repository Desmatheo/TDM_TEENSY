#include "Compresseur.h"

CompresseurEffect::CompresseurEffect()
#if !UtilBypassRoutage
: AudioStream(1, inputQueueArray_) 
#endif
{
    setThreshold(-20.0f);
    setRatio(4.0f);
    setAttack(10.0f);
    setRelease(100.0f);
    setMakeupGain(3.0f);
}

void CompresseurEffect::calculateCoefs() {
    if (attackMs_ < 0.1f) attackMs_ = 0.1f;
    if (releaseMs_ < 0.1f) releaseMs_ = 0.1f;
    // Les coefficients pour le détecteur d'enveloppe
    attackCoef_ = expf(-1.0f / (attackMs_ * 0.001f * AUDIO_SAMPLE_RATE_EXACT));
    releaseCoef_ = expf(-1.0f / (releaseMs_ * 0.001f * AUDIO_SAMPLE_RATE_EXACT));
}

void CompresseurEffect::setThreshold(float thresholdDb) {
    thresholdDb_ = thresholdDb;
    thresholdLinear_ = dbToLinear(thresholdDb_);
}

void CompresseurEffect::setRatio(float ratio) {
    ratio_ = ratio;
    if (ratio_ < 1.0f) ratio_ = 1.0f;
}

void CompresseurEffect::setAttack(float attackMs) {
    attackMs_ = attackMs;
    calculateCoefs();
}

void CompresseurEffect::setRelease(float releaseMs) {
    releaseMs_ = releaseMs;
    calculateCoefs();
}

void CompresseurEffect::setMakeupGain(float gainDb) {
    makeupGainLinear_ = dbToLinear(gainDb);
}

void CompresseurEffect::setParameter(int param_id, float value) {
    switch (param_id) {
        case 0: // Threshold (-60dB to 0dB)
            setThreshold(-60.0f + (value * 60.0f));
            break;
        case 1: // Ratio (1:1 to 20:1)
            setRatio(1.0f + (value * 19.0f));
            break;
        case 2: // Attack (1ms to 100ms)
            setAttack(1.0f + (value * 99.0f));
            break;
        case 3: // Release (10ms to 1000ms)
            setRelease(10.0f + (value * 990.0f));
            break;
        case 4: // Makeup Gain (0dB to 24dB)
            setMakeupGain(value * 24.0f);
            break;
        default:
            break;
    }
}

#if !TEENSY
void CompresseurEffect::update(const float** in, float** out, int idx) {
    // Daisy not implemented
}
#else
#if !UtilBypassRoutage
void CompresseurEffect::update() {
    audio_block_t* in = receiveReadOnly(0);
    if (!in) return;
    
    if (!active_) {
        transmit(in, 0);
        release(in);
        return;
    }

    audio_block_t* out = allocate();
    if (!out) {
        release(in);
        return;
    }

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float sample = (float)in->data[i] / 32768.0f;
        float absSample = fabsf(sample);

        // Détecteur d'enveloppe
        if (absSample > envelope_) {
            envelope_ = attackCoef_ * envelope_ + (1.0f - attackCoef_) * absSample;
        } else {
            envelope_ = releaseCoef_ * envelope_ + (1.0f - releaseCoef_) * absSample;
        }

        // Conversion du niveau détecté en dB
        float envDb = linearToDb(envelope_);
        
        // Calcul du gain de compression
        float gainDb = 0.0f;
        if (envDb > thresholdDb_) {
            // Dépassement du seuil : on atténue
            float overshootDb = envDb - thresholdDb_;
            float attenuationDb = overshootDb * (1.0f - (1.0f / ratio_));
            gainDb = -attenuationDb;
        }
        
        // Conversion du gain dB en linéaire
        float gainLinear = dbToLinear(gainDb);
        
        // Application du gain et du makeup gain
        float output = sample * gainLinear * makeupGainLinear_;

        // Saturation hard clipping si dépassement pour éviter les erreurs d'overflow
        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;

        out->data[i] = (int16_t)(output * 32767.0f);
    }

    transmit(out, 0);
    release(out);
    release(in);
}
#else
void CompresseurEffect::update(float* buffer, int numSamples) {
    if (!active_) return;

    for (int i = 0; i < numSamples; i++) {
        float sample = buffer[i];
        float absSample = fabsf(sample);

        // Détecteur d'enveloppe
        if (absSample > envelope_) {
            envelope_ = attackCoef_ * envelope_ + (1.0f - attackCoef_) * absSample;
        } else {
            envelope_ = releaseCoef_ * envelope_ + (1.0f - releaseCoef_) * absSample;
        }

        // Conversion du niveau détecté en dB
        float envDb = linearToDb(envelope_);
        
        // Calcul du gain de compression
        float gainDb = 0.0f;
        if (envDb > thresholdDb_) {
            float overshootDb = envDb - thresholdDb_;
            float attenuationDb = overshootDb * (1.0f - (1.0f / ratio_));
            gainDb = -attenuationDb;
        }
        
        // Conversion du gain dB en linéaire
        float gainLinear = dbToLinear(gainDb);
        
        // Application du gain et du makeup gain
        float output = sample * gainLinear * makeupGainLinear_;

        buffer[i] = output;
    }
}
#endif 
#endif
