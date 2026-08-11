#pragma once

#define TEENSY 1

#define Osc 0
#if Osc
#define OscCodec 1
#else 
#define GuitareCodec 1
#endif

#define Usb 1

#if Usb
#define SerialUSB 0
#define CPU_Serial 1
#define CPU_MIDI 1

#define USE_MIDI_USB 1

#define USBIn 0
#define USBOut 1
#endif

#define UtilEffet 1
#define PeakAnalysage 0

#if TEENSY
#define UtilBypassRoutage 1
#endif

static inline float clampf(float value, float min, float max){
    return (value < min) ? min : (value > max) ? max : value;
}
