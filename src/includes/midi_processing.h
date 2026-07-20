#pragma once

#include "main.h" 
#include "Utils.h"

// Variables d'état globales pour la gestion des effets et du bypass
static bool delayActif[6] = {false};   // État d'activation du Delay
static bool distoActif[6] = {false};   // État d'activation de la Disto
static bool octaverActif[6] = {false}; // État d'activation de l'Octaver
static bool stringBypass[6] = {false}; // Bypass individuel par corde (Mute)
static bool globalBypassState = false; // Bypass global

#if USE_MIDI_USB

static void OnControlChange(byte channel, byte control, byte value) {
    #pragma region Control MIDI
    float valNorm = value / 127.0f;

    // Sur Teensy usbMIDI, channel est indexé de 1 à 16.
    // Convertir en index de corde 0..5 si le canal MIDI est dans la plage 1..6 (ou 0..5).
    int corde = -1;
    if (channel >= 1 && channel <= 6) {
        corde = channel - 1;
    } else if (channel >= 0 && channel < 6) {
        corde = channel;
    }

    // --- A. CONTRÔLES SPÉCIFIQUES À UNE CORDE (via canal MIDI 1-6 / 0-5) ---

    // 1. DELAY (CC 10 à 16 ou plage 10 à 45)
    if (control >= 10 && control <= 45) {
        int potard = 0;
        int targetCorde = corde;

        if (control >= 10 && control <= 16 && corde >= 0 && corde < 6) {
            // Mapping via Canal MIDI (CC 10-16 sur le canal de la corde)
            potard = control - 10;
        } else {
            // Mapping via plage CC continue (CC 10-45 : 6 cordes x 6 potards)
            int ccRelatif = control - 10;
            targetCorde = ccRelatif / 6;
            potard = ccRelatif % 6;
        }

        if (targetCorde >= 0 && targetCorde < 6) {
            // Si la valeur est 0 sur le potard 0 (interrupteur d'activation de l'effet), désactiver l'effet
            if (value == 0 && potard == 0) {
                delayActif[targetCorde] = false;
                DelaysObj[targetCorde].setEnabled(false);
#if SerialUSB
                Serial.print("MIDI -> Effet: DELAY OFF | Corde: ");
                Serial.println(targetCorde);
#endif
            } else {
                delayActif[targetCorde] = true; // Mémorise que Delay est actif sur cette corde
                if (!stringBypass[targetCorde] && !globalBypassState) {
                    DelaysObj[targetCorde].setEnabled(true);
                }

                DelaysObj[targetCorde].setParameter(potard, valNorm);

#if SerialUSB
                Serial.print("MIDI -> Effet: DELAY | Corde: ");
                Serial.print(targetCorde);
                Serial.print(" | Potard: P");
                Serial.print(potard + 1);
                Serial.print(" | Valeur: ");
                Serial.println(value);
#endif
            }
        }
    }

    // 2. DISTORTION (CC 50 à 56 ou plage 50 à 85)
    else if (control >= 50 && control <= 85) {
        int potard = 0;
        int targetCorde = corde;

        if (control >= 50 && control <= 56 && corde >= 0 && corde < 6) {
            potard = control - 50;
        } else {
            int ccRelatif = control - 50;
            targetCorde = ccRelatif / 6;
            potard = ccRelatif % 6;
        }

        if (targetCorde >= 0 && targetCorde < 6) {
            if (value == 0 && potard == 0) {
                distoActif[targetCorde] = false;
                DistosObj[targetCorde].setEnabled(false);
#if SerialUSB
                Serial.print("MIDI -> Effet: DISTO OFF | Corde: ");
                Serial.println(targetCorde);
#endif
            } else {
                distoActif[targetCorde] = true; // Mémorise que Disto est actif sur cette corde
                if (!stringBypass[targetCorde] && !globalBypassState) {
                    DistosObj[targetCorde].setEnabled(true);
                }   

                DistosObj[targetCorde].setParameter(potard, valNorm);

#if SerialUSB
                Serial.print("MIDI -> Effet: DISTO | Corde: ");
                Serial.print(targetCorde);
                Serial.print(" | Potard: P");
                Serial.print(potard + 1);
                Serial.print(" | Valeur: ");
                Serial.println(value);
#endif
            }
        }
    }

    // 3. EARTH / OCTAVER (CC 90 à 96 ou plage 90 à 125)
    else if (control >= 90 && control <= 125) {
        int potard = 0;
        int targetCorde = corde;

        if (control >= 90 && control <= 96 && corde >= 0 && corde < 6) {
            potard = control - 90;
        } else {
            int ccRelatif = control - 90;
            targetCorde = ccRelatif / 6;
            potard = ccRelatif % 6;
        }

        if (targetCorde >= 0 && targetCorde < 6) {
            if (value == 0 && potard == 0) {
                octaverActif[targetCorde] = false;
                OctaverObj[targetCorde].setEnabled(false);
#if SerialUSB
                Serial.print("MIDI -> Effet: OCTAVER OFF | Corde: ");
                Serial.println(targetCorde);
#endif
            } else {
                octaverActif[targetCorde] = true; // Mémorise que Octaver est actif sur cette corde
                if (!stringBypass[targetCorde] && !globalBypassState) {
                    OctaverObj[targetCorde].setEnabled(true);
                }

                OctaverObj[targetCorde].setParameter(potard, valNorm);

#if SerialUSB
                Serial.print("MIDI -> Effet: Octaver | Corde: ");
                Serial.print(targetCorde);
                Serial.print(" | Potard: P");
                Serial.print(potard + 1);
                Serial.print(" | Valeur: ");
                Serial.println(value);
#endif
            }
        }
    }

    // --- B. CONTRÔLES GLOBAUX OU BYPASS ---

    // 4. MUTE / BYPASS INDIVIDUEL PAR CORDE (CC 0 à 5)
    else if (control >= 0 && control <= 5) {
        int targetCorde = control;
        bool isMuted = (value > 63);
        stringBypass[targetCorde] = isMuted;

#if SerialUSB
        Serial.print("MIDI -> Mute Corde | Corde: ");
        Serial.print(targetCorde);
        Serial.print(" | Etat: ");
        Serial.println(isMuted ? "MUTED" : "UNMUTED");
#endif

        if (isMuted || globalBypassState) {
            DistosObj[targetCorde].setEnabled(false);
            DelaysObj[targetCorde].setEnabled(false);
            OctaverObj[targetCorde].setEnabled(false);
        } else {
            // Sortie de mute : réactivation des effets qui étaient configurés
            if (delayActif[targetCorde])   DelaysObj[targetCorde].setEnabled(true);
            if (distoActif[targetCorde])   DistosObj[targetCorde].setEnabled(true);
            if (octaverActif[targetCorde]) OctaverObj[targetCorde].setEnabled(true);
        }
    }

    // 5. BYPASS PAR EFFET (CC 48, 88, 89)
    else if (control == 48 || control == 88 || control == 89) {
        int targetCorde = (corde >= 0 && corde < 6) ? corde : 0;
        bool isBypassed = (value > 63);

        if (isBypassed) {
            if (control == 48) {
                delayActif[targetCorde] = false;
                DelaysObj[targetCorde].setEnabled(false);
            } else if (control == 88) {
                distoActif[targetCorde] = false;
                DistosObj[targetCorde].setEnabled(false);
            } else if (control == 89) {
                octaverActif[targetCorde] = false;
                OctaverObj[targetCorde].setEnabled(false);
            }

#if SerialUSB
            Serial.print("MIDI -> Effet Bypass via CC ");
            Serial.print(control);
            Serial.print(" | Corde: ");
            Serial.println(targetCorde);
#endif
        } else {
            // Activation de l'effet correspondant
            if (control == 48) {
                delayActif[targetCorde] = true;
                if (!stringBypass[targetCorde] && !globalBypassState) DelaysObj[targetCorde].setEnabled(true);
            } else if (control == 88) {
                distoActif[targetCorde] = true;
                if (!stringBypass[targetCorde] && !globalBypassState) DistosObj[targetCorde].setEnabled(true);
            } else if (control == 89) {
                octaverActif[targetCorde] = true;
                if (!stringBypass[targetCorde] && !globalBypassState) OctaverObj[targetCorde].setEnabled(true);
            }

#if SerialUSB
            Serial.print("MIDI -> Effet Active via CC ");
            Serial.print(control);
            Serial.print(" | Corde: ");
            Serial.println(targetCorde);
#endif
        }
    }

    // 6. BYPASS GLOBAL (CC 126)
    else if (control == 126) {
        globalBypassState = (value > 63);

#if SerialUSB
        Serial.print("MIDI -> Global Bypass: ");
        Serial.println(globalBypassState ? "ON" : "OFF");
#endif

        for (int i = 0; i < 6; i++) {
            if (globalBypassState || stringBypass[i]) {
                DistosObj[i].setEnabled(false);
                DelaysObj[i].setEnabled(false);
                OctaverObj[i].setEnabled(false);
            } else {
                if (delayActif[i])   DelaysObj[i].setEnabled(true);
                if (distoActif[i])   DistosObj[i].setEnabled(true);
                if (octaverActif[i]) OctaverObj[i].setEnabled(true);
            }
        }
    }
    #pragma endregion
}

#endif