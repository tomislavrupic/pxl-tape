#pragma once
#include <cmath>
#include <algorithm>
// Append-only portable parameter order follows the original 37 controls.
#define PXL_DEVICE_PARAMETERS(X) \
 X(macro3,"Motion",0,1,.35,0) \
 X(macro4,"Colour",0,1,.5,0) \
 X(decay,"Decay",.005,4,.35,0) \
 X(sustainLevel,"Sustain",0,1,1,0) \
 X(fxType,"Effect device",0,7,0,1) \
 X(fxA,"FX time / frequency",0,1,.35,0) \
 X(fxB,"FX feedback / resonance",0,1,.4,0) \
 X(fxC,"FX colour",0,1,.5,0) \
 X(fxMix,"Device mix",0,1,.3,0) \
 X(fxRoute,"FX route: source / master",0,1,0,1) \
 X(modType,"Modulation",0,6,0,1) \
 X(modRate,"Modulation rate",.05,20,1,0) \
 X(modDepth,"Modulation depth",0,1,0,0) \
 X(modTarget,"Modulation target",0,3,0,1) \
 X(seqType,"Sequencer device",0,5,0,1) \
 X(seqLength,"Sequence length",1,128,16,1) \
 X(seqDirection,"Sequence direction",0,2,0,1) \
 X(seqGate,"Note gate",.05,.95,.65,0) \
 X(seqOctaves,"Arpeggio octaves",1,4,1,1) \
 X(gravity,"Tombola gravity",0,1,.5,0) \
 X(playMode,"Play mode",0,2,0,1) \
 X(sampleStart,"Sample start",0,.99,0,0) \
 X(sampleEnd,"Sample end",.01,1,1,0) \
 X(sampleLoop,"Sample loop",0,1,0,1) \
 X(sampleReverse,"Sample reverse",0,1,0,1) \
 X(drumMode,"Drum synth / slices",0,1,0,1) \
 X(drumPitch,"Drum pitch",-24,24,0,0) \
 X(drumDecay,"Drum decay",.1,3,1,0) \
 X(drumColour,"Drum colour",0,1,.5,0) \
 X(drumShape,"Drum cross-modulation",0,1,.25,0) \
 X(eqLow,"Low EQ dB",-12,12,0,0) \
 X(eqMid,"Mid EQ dB",-12,12,0,0) \
 X(eqHigh,"High EQ dB",-12,12,0,0) \
 X(compressor,"Master compression",0,1,0,0) \
 X(seqBank,"Pattern bank",0,7,0,1)
namespace pxl {
struct DeviceSettings {
#define DECLARE(id,label,lo,hi,def,integer) float id=float(def);
 PXL_DEVICE_PARAMETERS(DECLARE)
#undef DECLARE
};
inline bool setDeviceParameter(DeviceSettings& settings,int index,float value){
 if(!std::isfinite(value))return false;int n=0;
#define SET(id,label,lo,hi,def,integer) if(index==n++){settings.id=std::clamp(float(integer?std::round(value):value),float(lo),float(hi));return true;}
 PXL_DEVICE_PARAMETERS(SET)
#undef SET
 return false;
}
constexpr int deviceParameterCount=0
#define COUNT(id,label,lo,hi,def,integer) +1
 PXL_DEVICE_PARAMETERS(COUNT)
#undef COUNT
;
}
