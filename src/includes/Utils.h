#pragma once


// Ce fichier contient une serie de define qui permettent de configurer le projet pour l'utilisation sur Teensy
// Il n'y a pas besoins de toucher a platformio.ini, sauf en cas de changement de fréquence d'échantillonnage ou de nombre de canaux USB

#define TEENSY 1

// Mettre le define a 1 si on veut ecouter une Oscillation
// Mettre le define a 0 si on veut ecouter le signal de la guitare
#define Osc 0
#if Osc
// soit les oscillations classiques, soit un sweep de fréquences (20 => 20kHz)
#define SweepCodec 0

#define OscCodec 1
#if OscCodec 
// 1 pour avoir un accord cool, 0 pour avoir la meme fréquence sur tous les cannaux commme un looser
#define AccordCool 1
#endif
#else 
#define GuitareCodec 1
#endif


// Mettre le define a 0 si on ne veut rien sur le canal USB (pas de MIDI, pas de Serial, pas de signal audio)
#define Usb 1
#if Usb
#define SerialUSB 1

// Mettre les defines a 1 si on veut afficher la charge CPU dans le moniteur série ou en MIDI 
// (les 2 sont compatibles en simultanés)
#define CPU_Serial 1
#define CPU_MIDI 1

// Mettre le define a 1 si on veut utiliser le MIDI USB (pour envoyer le CPU_MIDI par exemple)
#define USE_MIDI_USB 1

// Mettre le define a 1 si on veut envoyer un signal depuis l'USB
// /!\ Attension a bien configurer le define Hex/Stereo
#define USBIn 0
#if USBIn
// 1 si Hexa 0 si Stereo
#define USBHex 0
#endif

// Mettre le define a 1 si on veut recevoir le signal en USB
// Compatible avec la sortie CODEC TDM, on aura donc 2 sorties 
#define USBOut 0
#endif

// Mettre le define a 1 si on veut que les effets soient appliqués
// 0 pour tester juste en bypass 
// /!\ ça sert vraiment juste a debugger, sinon ça sert a rien :)
#define UtilEffet 1

// Mettre le define a 1 si on veut "voire" le volume de chaques cordes
#define PeakAnalysage 0

#if TEENSY
// Mettre le define a 0 si on veut que le chainage se fasse de maniere "classique"
// Mettre le define a 1 si on veut que les effets soient lancés depuis l"effet" bypass 
// (pour le chainage et l'utilisation sans passer par les fonctions Teensy)
#define UtilBypassRoutage 1
#endif

#define TESTPREAMP 1


static inline float clampf(float value, float min, float max){
    return (value < min) ? min : (value > max) ? max : value;
}
