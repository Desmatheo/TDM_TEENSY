#include "Delay.h"

// ------ Partie TEENSY -------

void DelayEffect::DelayChannel::Init(float sampleRate, uint32_t max_delay_samples) {
    buf_len = max_delay_samples;
    
    // Allocation en priorité sur la PSRAM (extmem_malloc)
    buffer = (float*)extmem_malloc(buf_len * sizeof(float));
    if (buffer) {
        use_extmem = true;
    } else {
        buffer = (float*)malloc(buf_len * sizeof(float));
        use_extmem = false;
    }

    if (buffer) {
        memset(buffer, 0, buf_len * sizeof(float));
    }
    write_idx = 0;
    
    // Initialisation du filtre Tone (Low-pass 1-pole) à 3000 Hz
    float tone_hz = 3000.0f;
    float alpha = expf(-2.0f * (float)PI * tone_hz / sampleRate);
    tone_a0 = 1.0f - alpha;
    tone_b1 = alpha;
    tone_z1 = 0.0f;

    currentDelay = 2400.0f;
    delayTarget = 2400.0f;
    feedback = 0.5f;
    active = true;
    muteFade = 1.0f;
    standbyTimer = 0;
    lastTarget = 2400.0f;
}

void DelayEffect::DelayChannel::Free() {
    if (buffer) {
        if (use_extmem) {
            extmem_free(buffer);
        } else {
            free(buffer);
        }
        buffer = nullptr;
    }
}

float DelayEffect::DelayChannel::Process(float in) {
    if (!buffer) return in;

    // Si le potard est en train d'être tourné (la cible change)
    if (fabsf(delayTarget - lastTarget) > 0.1f) {
        lastTarget = delayTarget;
        standbyTimer = 10000; // Maintient le standby pendant ~220ms après le dernier mouvement
    }

    if (standbyTimer > 0) {
        standbyTimer--;
        
        // Fade-out ultra-rapide
        muteFade -= 0.01f;
        if (muteFade <= 0.0f) {
            muteFade = 0.0f;
            // Dès qu'on est sous silence, le pointeur saute instantanément (aucun pitch-shift)
            currentDelay = delayTarget;
        }
    } else {
        // Le potard ne bouge plus, on sort du standby (Fade-in)
        muteFade += 0.01f;
        if (muteFade > 1.0f) {
            muteFade = 1.0f;
        }
    }

    // Lecture du son retardé avec interpolation linéaire
    float read_idx_f = (float)write_idx - currentDelay;
    while (read_idx_f < 0.0f) read_idx_f += (float)buf_len;
    while (read_idx_f >= (float)buf_len) read_idx_f -= (float)buf_len;

    uint32_t r0 = (uint32_t)read_idx_f;
    uint32_t r1 = r0 + 1; 
    if (r1 >= buf_len) r1 = 0;
    float frac = read_idx_f - (float)r0;

    float del_read = buffer[r0] + (buffer[r1] - buffer[r0]) * frac;

    // Application du fade pour le mode standby
    del_read *= muteFade;

    // Application du filtre passe-bas
    float read = tone_a0 * del_read + tone_b1 * tone_z1;
    tone_z1 = read;

    // Écriture dans la ligne de delay (avec feedback)
    float write_val = feedback * read;
    if (active) {
        write_val += in;
    }
    
    // Limiteur de saturation interne
    if (write_val > 1.0f) write_val = 1.0f;
    if (write_val < -1.0f) write_val = -1.0f;
    
    buffer[write_idx] = write_val;

    write_idx++;
    if (write_idx >= buf_len) write_idx = 0;

    return read;
}

DelayEffect::DelayEffect() : AudioStream(1, inputQueueArray_) {
    setMix(0.5f);
    setDelayTime(0.5f);
    setFeedback(0.7f);
}

DelayEffect::~DelayEffect() {
    delay.Free();
}

bool DelayEffect::begin() {
    delay.Free();
    delay.Init(AUDIO_SAMPLE_RATE_EXACT, MAX_DELAY);
    return (delay.buffer != nullptr);
}

void DelayEffect::update() {
    audio_block_t* in = receiveReadOnly(0);
    if (!in) return;
    
    if (!active || !delay.buffer) {
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
        float input = (float)in->data[i] / 32768.0f;

        float delay_out = delay.Process(input);

        float output = ((input * dryMix) + (delay_out * wetMix)) * volume;

        // Saturation douce (clamp final)
        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;

        out->data[i] = (int16_t)(output * 32767.0f);
    }

    transmit(out, 0);
    release(out);
    release(in);
}

void DelayEffect::setMix(float mix) {
    wetMix = clampf(mix, 0.0f, 1.0f);
    dryMix = 1.0f - wetMix;
}

void DelayEffect::setVolume(float vol)
{
    volume = clampf(vol, 0.0f, 1.0f);
}

void DelayEffect::setDelayTime(float time) {
    vdelayTime = clampf(time, 0.0f, 1.0f);

    delay.active = (vdelayTime > 0.01f);

    // Mapping linéaire : 0.0 → delay min, 1.0 → delay max
    constexpr float MIN_DELAY_SAMPLES = 2400.0f;   // ~54 ms
    constexpr float MAX_DELAY_SAMPLES = (float)MAX_DELAY;

    delay.delayTarget = MIN_DELAY_SAMPLES + vdelayTime * (MAX_DELAY_SAMPLES - MIN_DELAY_SAMPLES);
}

void DelayEffect::setFeedback(float fdbk) {
    vdelayFDBK = clampf(fdbk, 0.0f, 0.99f);
    delay.feedback = vdelayFDBK;
}


void DelayEffect::setParameter(int param_id, float value) {
    switch (param_id) {
        case 0:
            setMix(value);
            break;
        case 1:
            setDelayTime(value);
            break;
        case 2:
            setFeedback(value);
            break;
        case 5: 
            setVolume(value);
            break;
        default:
            break;
    }
}

