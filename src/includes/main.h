#pragma once

#include "../EffetTempBypass/Bypass.h"
#include "../EffetOctaver/Octaver.h"
#include "../EffetDelay/Delay.h"
#include "../EffetDisto/Disto.h"
#include "Utils.h"

#if Osc || OscCodec
AudioSynthWaveform osc[6];
#endif    

#if OscCodec || GuitareCodec
DMAMEM AudioControlCS42448 cs42448_1;      // Contrôleur matériel CsS42448dd

AudioInputTDM       tdm_codec_in;
AudioOutputTDM       tdm_codec_out;

// A changer en fonction de l'entrée utilisé sur la carte
const int reset_p = 34; 
#endif

#if Usb
#if USBIn
AudioInputUSB            usbIn;
#endif
#if USBOut
AudioOutputUSB           usbOut;           // Sortie audio de la teensy
#endif
#endif

extern OctaverEffect            OctaverObj[6];
extern DelayEffect              DelaysObj[6];
extern DistoEffect              DistosObj[6];
extern BypassEffect             BypassObj[6];