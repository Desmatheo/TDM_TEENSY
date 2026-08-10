#pragma once

#include "main.h" 
#include "Utils.h"

// Nombre de types d'effets (indices 0..5 : DISTO, OCTAVER, TREMOLO, DELAY, NOISEGATE, EQUALIZER)
static constexpr int NUM_EFFECT_TYPES = 6;
// Indices dans allEffects[][] et effectUserEnabled[][]
enum EffectTypeIdx { EFX_DISTO = 0, EFX_OCTAVER = 1, EFX_TREMOLO = 2, EFX_DELAY = 3, EFX_NOISEGATE = 4, EFX_EQUALIZER = 5 };

// Tableau [type][corde] — initialisé dans setup() de main.cpp n'est pas possible car 
// les adresses sont connues à la compilation, on les initialise ici en static
static Effect* allEffects[NUM_EFFECT_TYPES][6] = {
    {&DistosObj[0],     &DistosObj[1],     &DistosObj[2],     &DistosObj[3],     &DistosObj[4],     &DistosObj[5]},
    {&OctaverObj[0],    &OctaverObj[1],    &OctaverObj[2],    &OctaverObj[3],    &OctaverObj[4],    &OctaverObj[5]},
    {&TremolosObj[0],   &TremolosObj[1],   &TremolosObj[2],   &TremolosObj[3],   &TremolosObj[4],   &TremolosObj[5]},
    {&DelaysObj[0],     &DelaysObj[1],     &DelaysObj[2],     &DelaysObj[3],     &DelaysObj[4],     &DelaysObj[5]},
    {&NoiseGatesObj[0], &NoiseGatesObj[1], &NoiseGatesObj[2], &NoiseGatesObj[3], &NoiseGatesObj[4], &NoiseGatesObj[5]},
    {&EqualizersObj[0], &EqualizersObj[1], &EqualizersObj[2], &EqualizersObj[3], &EqualizersObj[4], &EqualizersObj[5]},
};

// Sauvegarde l'état "configuré" par l'utilisateur (indépendant du mute/bypass global)
static bool effectUserEnabled[NUM_EFFECT_TYPES][6] = {
    {false, false, false, false, false, false}, // Disto
    {false, false, false, false, false, false}, // Octaver
    {false, false, false, false, false, false}, // Tremolo
    {false, false, false, false, false, false}, // Delay
    {false, false, false, false, false, false}, // NoiseGate
    {false, false, false, false, false, false}, // Equalizer
};

static bool stringBypass[6] = {false}; // Bypass individuel par corde (Mute)
static bool globalBypassState = false; // Bypass global

// Mapping CC de bypass individuel → index d'effet
struct BypassCCMapping {
    byte cc;
    int effectIdx;
};
static constexpr BypassCCMapping bypassCCMap[] = {
    {48,  EFX_DELAY},
    {75,  EFX_EQUALIZER},
    {88,  EFX_DISTO},
    {89,  EFX_OCTAVER},
    {118, EFX_TREMOLO},
    {119, EFX_NOISEGATE},
};
static constexpr int NUM_BYPASS_CC = sizeof(bypassCCMap) / sizeof(bypassCCMap[0]);

// Mapping CC ranges de paramètres → index d'effet + nom pour debug
struct ParamCCRange {
    byte ccMin;
    byte ccMax;
    int effectIdx;
    const char* name;
};
static constexpr ParamCCRange paramRanges[] = {
    {10,  16,  EFX_DELAY,     "DELAY"},
    {50,  56,  EFX_DISTO,     "DISTO"},
    {76,  81,  EFX_EQUALIZER, "EQUALIZER"},
    {90,  95,  EFX_OCTAVER,   "OCTAVER"},
    {110, 116, EFX_TREMOLO,   "TREMOLO"},
    {120, 122, EFX_NOISEGATE, "NOISE GATE"},
};
static constexpr int NUM_PARAM_RANGES = sizeof(paramRanges) / sizeof(paramRanges[0]);

// Helper : applique le bypass/unmute pour toutes les effets d'une corde
static inline void applyBypassState(int corde) {
    for (int t = 0; t < NUM_EFFECT_TYPES; t++) {
        if (stringBypass[corde] || globalBypassState) {
            allEffects[t][corde]->setEnabled(false);
        } else {
            allEffects[t][corde]->setEnabled(effectUserEnabled[t][corde]);
        }
    }
}

#if USE_MIDI_USB

static void OnControlChange(byte channel, byte control, byte value) {
    #pragma region Control MIDI
    float valNorm = value / 127.0f;
    
    Serial.print("Test MIDI => Channel : ");
    Serial.print(channel);
    Serial.print(", Control: ");
    Serial.print(control);
    Serial.print(", Value : ");
    Serial.println(value);

    // Selection de la corde
    int targetCorde = -1;
    if (channel >= 1 && channel <= 6) {
        targetCorde = channel - 1;
    } else if (channel >= 0 && channel < 6) {
        targetCorde = channel;
    }
    // Si correspond pas, corde 0 (Mi grave)
    if (targetCorde < 0 || targetCorde > 5) {
        targetCorde = 0;
    }

    // Bypass effets individuel (CC 48, 88, 89, 118, 119)
    bool handledBypassCC = false;
    for (int b = 0; b < NUM_BYPASS_CC; b++) {
        if (control == bypassCCMap[b].cc) {
            bool isBypassed = (value > 63);
            int efxIdx = bypassCCMap[b].effectIdx;
            effectUserEnabled[efxIdx][targetCorde] = !isBypassed;
            
            if (!stringBypass[targetCorde] && !globalBypassState) {
                allEffects[efxIdx][targetCorde]->setEnabled(!isBypassed);
            }

#if SerialUSB
            Serial.print("MIDI -> Effet Bypass via CC ");
            Serial.print(control);
            Serial.print(" | Corde: ");
            Serial.print(targetCorde);
            Serial.print(" | isBypass: ");
            Serial.println(isBypassed);
#endif
            handledBypassCC = true;
            break;
        }
    }
    if (handledBypassCC) { /* déjà traité */ }
    
    // Paramètres des effets (CC ranges : 10-16, 50-56, 90-95, 110-115, 120-122)
    else {
        bool handledParam = false;
        for (int r = 0; r < NUM_PARAM_RANGES; r++) {
            if (control >= paramRanges[r].ccMin && control <= paramRanges[r].ccMax) {
                int potard = control - paramRanges[r].ccMin;
                int efxIdx = paramRanges[r].effectIdx;

                effectUserEnabled[efxIdx][targetCorde] = true;
                if (!stringBypass[targetCorde] && !globalBypassState) {
                    allEffects[efxIdx][targetCorde]->setEnabled(true);
                }

                allEffects[efxIdx][targetCorde]->setParameter(potard, valNorm);

#if SerialUSB
                Serial.print("MIDI -> Effet: ");
                Serial.print(paramRanges[r].name);
                Serial.print(" | Corde: ");
                Serial.print(targetCorde);
                Serial.print(" | Potard: P");
                Serial.print(potard + 1);
                Serial.print(" | Valeur: ");
                Serial.println(value);
#endif
                handledParam = true;
                break;
            }
        }

        if (!handledParam) {
            // Bypass par corde (CC 0-5)
            if (control >= 0 && control <= 5) {
                int corde = control;
                bool isMuted = (value > 63);
                stringBypass[corde] = isMuted;

#if SerialUSB
                Serial.print("MIDI -> Mute Corde | Corde: ");
                Serial.print(corde);
                Serial.print(" | Etat: ");
                Serial.println(isMuted ? "MUTED" : "UNMUTED");
#endif
                applyBypassState(corde);
            } 

            // Bypass global (CC 126)
            else if (control == 126) {
                globalBypassState = (value > 63);

#if SerialUSB
                Serial.print("MIDI -> Global Bypass: ");
                Serial.println(globalBypassState ? "ON" : "OFF");
#endif

                for (int i = 0; i < 6; i++) {
                    applyBypassState(i);
                }
            }

            // Changement de l'ordre du chaînage via les 3 Slots (CC 20, 21, 22)
            // mapping : 0=None, 1=Delay, 2=Disto, 3=Earth, 4=Tremolo
            else if (control >= 20 && control <= 22) {
                int slotIdx = control - 20;
                BypassObj[targetCorde].setSlot(slotIdx, value);

#if SerialUSB
                Serial.print("MIDI -> Chain Order | Corde: ");
                Serial.print(targetCorde);
                Serial.print(" | Slot ");
                Serial.print(slotIdx + 1);
                Serial.print(" = Effet ID ");
                Serial.println(value);
#endif
            }
        }
    }
    #pragma endregion
}

#endif