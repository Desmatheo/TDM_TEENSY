#pragma once

#define Osc 0
#if Osc
#define OscCodec 0
#else 
#define GuitareCodec 1
#endif
#define Usb 1

#if Usb
#define SerialUSB 1

#define USE_MIDI_USB 1

#define USBIn 0
#define USBOut 1
#endif

#define UtilEffet 1

static inline float clampf(float value, float min, float max){
    return (value < min) ? min : (value > max) ? max : value;
}
