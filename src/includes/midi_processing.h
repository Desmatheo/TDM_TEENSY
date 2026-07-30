#pragma once

#include "main.h" 
#include "Utils.h"

// Variables d'état globales pour la gestion des effets et du bypass
static bool delayActif[6] = {false};   // État d'activation du Delay
static bool distoActif[6] = {false};   // État d'activation de la Disto
static bool octaverActif[6] = {false}; // État d'activation de l'Octaver
static bool tremoloActif[6] = {false}; // État d'activation du Tremolo
static bool noiseGateActif[6] = {false}; // État d'activation du Noise Gate
static bool stringBypass[6] = {false}; // Bypass individuel par corde (Mute)
static bool globalBypassState = false; // Bypass global

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

    // Bypass effets individuel
    if (control == 48 || control == 88 || control == 89 || control == 118 || control == 119) {
        bool isBypassed = (value > 63);

        if (control == 48) {
            delayActif[targetCorde] = !isBypassed;
            DelaysObj[targetCorde].setEnabled(!isBypassed);
        } else if (control == 88) {
            distoActif[targetCorde] = !isBypassed;
            DistosObj[targetCorde].setEnabled(!isBypassed);
        } else if (control == 89) {
            octaverActif[targetCorde] = !isBypassed;
            OctaverObj[targetCorde].setEnabled(!isBypassed);
        } else if (control == 118) {
            tremoloActif[targetCorde] = !isBypassed;
            TremolosObj[targetCorde].setEnabled(!isBypassed);
        } else if (control == 119) {
            noiseGateActif[targetCorde] = !isBypassed;
            NoiseGatesObj[targetCorde].setEnabled(!isBypassed);
        }
#if SerialUSB
        Serial.print("MIDI -> Effet Bypass via CC ");
        Serial.print(control);
        Serial.print(" | Corde: ");
        Serial.print(targetCorde);
        Serial.print(" | isBypass: ");
        Serial.println(isBypassed);
#endif
    } 
    
    // Delay
    else if (control >= 10 && control <= 16) {
        int potard = control - 10;
        
        delayActif[targetCorde] = true; 
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

    // Distortion
    else if (control >= 50 && control <= 56) {
        int potard = control - 50;

        distoActif[targetCorde] = true; 
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

    // Octaver
    else if (control >= 90 && control <= 95) {
        int potard = control - 90;

        octaverActif[targetCorde] = true; 
        if (!stringBypass[targetCorde] && !globalBypassState) {
            OctaverObj[targetCorde].setEnabled(true);
        }

        OctaverObj[targetCorde].setParameter(potard, valNorm);

#if SerialUSB
        Serial.print("MIDI -> Effet: OCTAVER | Corde: ");
        Serial.print(targetCorde);
        Serial.print(" | Potard: P");
        Serial.print(potard + 1);
        Serial.print(" | Valeur: ");
        Serial.println(value);
#endif
    }

    // Tremolo
    else if (control >= 110 && control <= 115) {
        int potard = control - 110;

        tremoloActif[targetCorde] = true; 
        if (!stringBypass[targetCorde] && !globalBypassState) {
            TremolosObj[targetCorde].setEnabled(true);
        }

        TremolosObj[targetCorde].setParameter(potard, valNorm);

#if SerialUSB
        Serial.print("MIDI -> Effet: TREMOLO | Corde: ");
        Serial.print(targetCorde);
        Serial.print(" | Potard: P");
        Serial.print(potard + 1);
        Serial.print(" | Valeur: ");
        Serial.println(value);
#endif
    }

    // Noise Gate
    else if (control >= 120 && control <= 122) {
        int potard = control - 120;

        noiseGateActif[targetCorde] = true; 
        if (!stringBypass[targetCorde] && !globalBypassState) {
            NoiseGatesObj[targetCorde].setEnabled(true);
        }

        NoiseGatesObj[targetCorde].setParameter(potard, valNorm);

#if SerialUSB
        Serial.print("MIDI -> Effet: NOISE GATE | Corde: ");
        Serial.print(targetCorde);
        Serial.print(" | Potard: P");
        Serial.print(potard + 1);
        Serial.print(" | Valeur: ");
        Serial.println(value);
#endif
    }

    // Bypass par corde
    else if (control >= 0 && control <= 5) {
        int corde = control; // CC de 0 à 5 correspondent à la corde
        bool isMuted = (value > 63);
        stringBypass[corde] = isMuted;

#if SerialUSB
        Serial.print("MIDI -> Mute Corde | Corde: ");
        Serial.print(corde);
        Serial.print(" | Etat: ");
        Serial.println(isMuted ? "MUTED" : "UNMUTED");
#endif

        if (isMuted || globalBypassState) {
            DistosObj[corde].setEnabled(false);
            DelaysObj[corde].setEnabled(false);
            OctaverObj[corde].setEnabled(false);
            TremolosObj[corde].setEnabled(false);
            NoiseGatesObj[corde].setEnabled(false);
        } else {
            // Sortie de mute : réactivation des effets qui étaient configurés
            if (delayActif[corde])   DelaysObj[corde].setEnabled(true);
            if (distoActif[corde])   DistosObj[corde].setEnabled(true);
            if (octaverActif[corde]) OctaverObj[corde].setEnabled(true);
            if (tremoloActif[corde]) TremolosObj[corde].setEnabled(true);
            if (noiseGateActif[corde]) NoiseGatesObj[corde].setEnabled(true);
        }
    } 

    // Bypass global
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
                TremolosObj[i].setEnabled(false);
                NoiseGatesObj[i].setEnabled(false);
            } else {
                if (delayActif[i])   DelaysObj[i].setEnabled(true);
                if (distoActif[i])   DistosObj[i].setEnabled(true);
                if (octaverActif[i]) OctaverObj[i].setEnabled(true);
                if (tremoloActif[i]) TremolosObj[i].setEnabled(true);
                if (noiseGateActif[i]) NoiseGatesObj[i].setEnabled(true);
            }
        }
    }
    #pragma endregion
}

#endif