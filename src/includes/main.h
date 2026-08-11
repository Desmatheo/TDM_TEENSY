#pragma once

#include "../EffetTempBypass/Bypass.h"
#include "../EffetOctaver/Octaver.h"
#include "../EffetDelay/Delay.h"
#include "../EffetDisto/Disto.h"
#include "../EffetTremolo/Tremolo.h"
#include "../EffetNoiseGate/NoiseGate.h"
#include "../EffetEqualizer/Equalizer.h"
#include "../EffetCompresseur/Compresseur.h"
#include "Utils.h"

#if Osc || OscCodec
AudioSynthWaveform osc[6];
#endif    

#if OscCodec || GuitareCodec
DMAMEM AudioControlCS42448 cs42448_1;      // Contrôleur matériel CsS42448dd

AudioInputTDM       tdm_codec_in;
AudioOutputTDM       tdm_codec_out;

// A changer en fonction de l'entrée utilisé sur la carte
const int reset_p = 2; 
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
extern TremoloEffect            TremolosObj[6];
extern NoiseGateEffect          NoiseGatesObj[6];
extern EqualizerEffect          EqualizersObj[6];
extern CompresseurEffect        CompresseurObj[6];
extern BypassEffect             BypassObj[6];


inline void setupEffet(){
#if UtilEffet
#define TestMano 1
  for (int i = 0; i < 6; i++){
    
    OctaverObj[i].setEnabled(false);
    DelaysObj[i].setEnabled(false);
    DistosObj[i].setEnabled(false);
    TremolosObj[i].setEnabled(false);  
    NoiseGatesObj[i].setEnabled(false);
    EqualizersObj[i].setEnabled(false);
    CompresseurObj[i].setEnabled(false);
    
    BypassObj[i].setStringIndex(i);
    BypassObj[i].setEffect(BypassEffect::DISTO,     &DistosObj[i]);
    BypassObj[i].setEffect(BypassEffect::OCTAVER,   &OctaverObj[i]);
    BypassObj[i].setEffect(BypassEffect::TREMOLO,   &TremolosObj[i]);
    BypassObj[i].setEffect(BypassEffect::DELAY,     &DelaysObj[i]);
    BypassObj[i].setEffect(BypassEffect::NOISEGATE, &NoiseGatesObj[i]);
    BypassObj[i].setEffect(BypassEffect::EQUALIZER, &EqualizersObj[i]);
    BypassObj[i].setEffect(BypassEffect::COMPRESSEUR, &CompresseurObj[i]);

    OctaverObj[i].setMix(0.7f);
    OctaverObj[i].setVolume(1.0f);
    OctaverObj[i].setOctaveMode(1);

    DelaysObj[i].begin();
    DelaysObj[i].setMix(0.5f);
    DelaysObj[i].setFeedback(0.0f);
    DelaysObj[i].setDelayTime(0.6f);
    DelaysObj[i].setVolume(1.0f);

    DistosObj[i].setDistoMode(1);
    DistosObj[i].setVolume(0.01f);
         
    TremolosObj[i].setMix(1.0f);
    TremolosObj[i].setDepth(1.0f);
    TremolosObj[i].setWaveform(0);
    // TremolosObj[i].setPhaseOffset(0.0f);
    TremolosObj[i].setPhaseMode(TremoloEffect::SYNC);
    TremolosObj[i].setGlobalRate(5.0f);

    TremolosObj[i].setVolume(1.0f);
    
    NoiseGatesObj[i].setThreshold(0.01f);
    NoiseGatesObj[i].setAttack(1.0f);
    NoiseGatesObj[i].setRelease(50.0f);
    
    EqualizersObj[i].begin();
    // /*setBand(numéro de la bande {80.0f, 250.0f, 750.0f, 2200.0f, 6600.0f}
    //          , valeure normalisée [0 = -12dB, 1 = +12dB]
    //          )*/
    EqualizersObj[i].setBand(0, 0.0f); 
    EqualizersObj[i].setBand(1, 0.0f); 
    EqualizersObj[i].setBand(2, 0.0f); 
    EqualizersObj[i].setBand(3, 0.0f); 
    EqualizersObj[i].setBand(4, 0.0f); 
    EqualizersObj[i].setVolume(1.0f);

    CompresseurObj[i].setThreshold(-15.0f);
    CompresseurObj[i].setRatio(2.5f);
    CompresseurObj[i].setAttack(25.0f);
    CompresseurObj[i].setRelease(150.0f);
    CompresseurObj[i].setMakeupGain(2.0f);

#if TestMano
    EqualizersObj[i].setEnabled(false);
    CompresseurObj[i].setEnabled(true);
#endif
  }
#endif
}