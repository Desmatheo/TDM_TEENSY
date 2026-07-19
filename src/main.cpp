#include <Audio.h>
#include <Wire.h>
#include "includes/main.h"

#if UtilEffet
DelayEffect              DelaysObj[6];
OctaverEffect            OctaverObj[6];
DistoEffect              DistosObj[6];
#endif
BypassEffect             BypassObj[6];

AudioMixer4              mixer_1a4;   
AudioMixer4              mixer_5et6;  
AudioMixer4              master;      

// 6 canaux hexa vers la Daisy : ports PAIRS 0..10 = slots 0..5
#if Osc || OscCodec
AudioConnection c0(osc[0], 0, BypassObj[0], 0);
AudioConnection c1(osc[1], 0, BypassObj[1], 0);
AudioConnection c2(osc[2], 0, BypassObj[2], 0);
AudioConnection c3(osc[3], 0, BypassObj[3], 0);
AudioConnection c4(osc[4], 0, BypassObj[4], 0);
AudioConnection c5(osc[5], 0, BypassObj[5], 0);
#elif USBIn
AudioConnection c0(usbIn, 0, BypassObj[0], 0);
AudioConnection c1(usbIn, 1, BypassObj[1], 0);
AudioConnection c2(usbIn, 2, BypassObj[2], 0);
AudioConnection c3(usbIn, 3, BypassObj[3], 0);
AudioConnection c4(usbIn, 4, BypassObj[4], 0);
AudioConnection c5(usbIn, 5, BypassObj[5], 0);
#elif GuitareCodec
AudioConnection c0(tdm_codec_in, 10, BypassObj[0], 0);
AudioConnection c1(tdm_codec_in,  8, BypassObj[1], 0);
AudioConnection c2(tdm_codec_in,  6, BypassObj[2], 0);
AudioConnection c3(tdm_codec_in,  4, BypassObj[3], 0);
AudioConnection c4(tdm_codec_in,  2, BypassObj[4], 0);
AudioConnection c5(tdm_codec_in,  0, BypassObj[5], 0);
#endif

#if UtilEffet
AudioConnection  c6(BypassObj[0], 0, DistosObj[0], 0);
AudioConnection  c7(BypassObj[1], 0, DistosObj[1], 0);
AudioConnection  c8(BypassObj[2], 0, DistosObj[2], 0);
AudioConnection  c9(BypassObj[3], 0, DistosObj[3], 0);
AudioConnection c10(BypassObj[4], 0, DistosObj[4], 0);
AudioConnection c11(BypassObj[5], 0, DistosObj[5], 0);

AudioConnection c12(DistosObj[0], 0, OctaverObj[0], 0);
AudioConnection c13(DistosObj[1], 0, OctaverObj[1], 0);
AudioConnection c14(DistosObj[2], 0, OctaverObj[2], 0);
AudioConnection c15(DistosObj[3], 0, OctaverObj[3], 0);
AudioConnection c16(DistosObj[4], 0, OctaverObj[4], 0);
AudioConnection c17(DistosObj[5], 0, OctaverObj[5], 0);

AudioConnection c18(OctaverObj[0], 0, DelaysObj[0], 0);
AudioConnection c19(OctaverObj[1], 0, DelaysObj[1], 0);
AudioConnection c20(OctaverObj[2], 0, DelaysObj[2], 0);
AudioConnection c21(OctaverObj[3], 0, DelaysObj[3], 0);
AudioConnection c22(OctaverObj[4], 0, DelaysObj[4], 0);
AudioConnection c23(OctaverObj[5], 0, DelaysObj[5], 0);

AudioConnection c24(DelaysObj[0], 0, mixer_1a4,  0);
AudioConnection c25(DelaysObj[1], 0, mixer_1a4,  1);
AudioConnection c26(DelaysObj[2], 0, mixer_1a4,  2);
AudioConnection c27(DelaysObj[3], 0, mixer_1a4,  3);
AudioConnection c28(DelaysObj[4], 0, mixer_5et6, 0);
AudioConnection c29(DelaysObj[5], 0, mixer_5et6, 1);
#else
AudioConnection  c6(BypassObj[0],  0, mixer_1a4,  0);
AudioConnection  c7(BypassObj[1],  0, mixer_1a4,  1);
AudioConnection  c8(BypassObj[2],  0, mixer_1a4,  2);
AudioConnection  c9(BypassObj[3],  0, mixer_1a4,  3);
AudioConnection c10(BypassObj[4],  0, mixer_5et6, 0);
AudioConnection c11(BypassObj[5],  0, mixer_5et6, 1);
#endif

AudioConnection mast1(mixer_1a4 , 0, master, 0);
AudioConnection mast2(mixer_5et6, 0, master, 1);

#if USBOut
AudioConnection p_outL_usb(master, 0, usbOut, 0);
AudioConnection p_outR_usb(master, 0, usbOut, 1);
#endif
#if OscCodec || GuitareCodec
AudioConnection f1(master, 0, tdm_codec_out, 12);
AudioConnection f2(master, 0, tdm_codec_out, 14);
#endif

void setup()
{
  AudioMemory(1500);   // TDM exige >= 16 blocs ; 50 = confortable
#if SerialUSB
  Serial.begin(115200);
#endif

#if GuitareCodec || OscCodec
  pinMode(reset_p, OUTPUT);                                    
  //Power-Up Sequence
  digitalWrite(reset_p, LOW);
  delay(800);
  digitalWrite(reset_p, HIGH);
  delay(10);
#if SerialUSB
  if (cs42448_1.enable()) {
      Serial.println("configured CS42448");
  } else {
      Serial.println("failed to config CS42448");
  }
#else 
  cs42448_1.enable();
#endif
  cs42448_1.inputLevel(1);
  cs42448_1.volume(1.0);
#endif

#if Osc || OscCodec
// test accord sympa :)
  osc[0].begin(0.06f, 110.00f, WAVEFORM_SINE); // 110..660 Hz
  osc[1].begin(0.04f, 329.63f, WAVEFORM_SINE); // 110..660 Hz
  osc[2].begin(0.06f, 440.00f, WAVEFORM_SINE); // 110..660 Hz
  osc[3].begin(0.04f, 554.37f, WAVEFORM_SINE); // 110..660 Hz
  osc[4].begin(0.04f, 659.26f, WAVEFORM_SINE); // 110..660 Hz
  osc[5].begin(0.06f, 880.00f, WAVEFORM_SINE); // 110..660 Hz
#endif

#if UtilEffet
#if 1
  for (int i = 0; i < 6; i++){
    OctaverObj[i].setEnabled(true);
    OctaverObj[i].setMix(0.7f);
    OctaverObj[i].setVolume(1.0f);
    OctaverObj[i].setOctaveMode(1);

    DelaysObj[i].begin();
    DelaysObj[i].setEnabled(false);
    DelaysObj[i].setMix(1.0f);
    DelaysObj[i].setFeedback(0.8f);   
    DelaysObj[i].setDelayTime(1.0f);
    DelaysObj[i].setVolume(1.0f);

    DistosObj[i].setEnabled(false);
    DistosObj[i].setDistoMode(1);
    DistosObj[i].setMix(1.0f);
    DistosObj[i].setVolume(0.01f);
  }
#endif
#endif

#if !SerialUSB
    Serial.end();
#endif
}

void loop()
{

#if SerialUSB
  unsigned long tempsActuel = millis();
  static unsigned long lastHeartbeat = 0;
  bool led = false;
  
  if (tempsActuel - lastHeartbeat >= 500) { 
    led = !led;
    lastHeartbeat = tempsActuel;
    
    
    // Affichage Charge CPU
    Serial.print("Charge CPU Audio Actuelle : ");
    Serial.print(AudioProcessorUsage());
    Serial.println(" %");

    Serial.print("Charge CPU Audio Max : ");
    Serial.print(AudioProcessorUsageMax());
    Serial.println(" %");
  }
#endif
}
