#pragma once
#include "DeviceParameters.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace pxl {
constexpr double tauD=6.2831853071795864769;
inline double wrapD(double x){return x-std::floor(x);}
struct DeviceVoice {
 std::array<double,7> phases{};uint32_t seed=0x6d2b79f5;float memory=0;
 float noise(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return float(double(seed)/2147483648.-1);}
 static float saw(double p,double dt){p=wrapD(p);float v=float(2*p-1);if(dt>0&&dt<.5){if(p<dt){auto t=p/dt;v-=float(t+t-t*t-1);}else if(p>1-dt){auto t=(p-1)/dt;v-=float(t*t+t+t+1);}}return v;}
 float render(int type,double phase,double frequency,double rate,float tone,float shape,float a,float b){
  double dt=std::min(.45,frequency/rate);float x=0;
  switch(type){
   case 5: // Seven independently detuned, band-limited oscillators.
    for(int i=0;i<7;i++){double cents=(i-3)*(1+shape*28),step=dt*std::pow(2.,cents/1200.);phases[i]=wrapD(phases[i]+step);x+=saw(phases[i],step)*float((i%2?1-a:a+.2));}return x/4;
   case 6:{ // Ring modulation + wave folding + continuously variable digital resolution.
    phases[0]=wrapD(phases[0]+dt*(1+std::floor(a*7)));double v=std::sin(tauD*phase)*(1-shape)+std::sin(tauD*phase)*std::sin(tauD*phases[0])*shape;v=std::sin(v*(1+b*12));double levels=4+std::pow(2.,tone*10);return float(std::round(v*levels)/levels);}
   case 7:{ // Two pulse trains with cross modulation and independent ratios.
    phases[0]=wrapD(phases[0]+dt*(.5+std::floor(a*4)*.5));double width=.08+shape*.84;auto pulse=[&](double p){return .5f*(saw(p,dt)-saw(p+width,dt));};return pulse(phase)+pulse(phases[0]+std::sin(tauD*phase)*b*.1)*(.2f+a*.5f);}
   case 8:{ // Phase distortion maps the two half cycles at a movable breakpoint.
    double split=.03+shape*.94,p=phase<split?phase/split*.5:.5+(phase-split)/(1-split)*.5;
    return float(std::sin(tauD*p+a*std::sin(tauD*p*(1+std::floor(b*5)))));}
   case 9: // Additive spectral wavetable; distinct wave morph and spectral tilt.
    for(int h=1;h<=16;h++){if(frequency*h>rate*.44)break;double odd=(h%2?1.:1.-shape),weight=odd*std::pow(double(h),-1.-b*2);double shift=a*h*h*.13; x+=float(std::sin(tauD*phase*h+shift)*weight*std::cos(shape*h*.3));}return x*.65f;
   case 10:{ // Dual oscillators with an audio-rate envelope cross modulator.
    phases[0]=wrapD(phases[0]+dt*std::pow(2.,(a-.5)*24/12.));memory+=float((.0002+b*.01)*(shape-memory));double v=std::sin(tauD*phase+std::sin(tauD*phases[0])*shape*6);return float(v*(1-a)+saw(phases[0],dt)*a)*(1-memory*.3f);}
   case 11:{ // Feedback phase oscillator and inductive waveshaper.
    double v=std::sin(tauD*phase+memory*(.2+shape*5)+std::sin(tauD*phase*2)*a*3);memory=float(v);return float(std::tanh(v*(1+b*8))*(.6+.4*std::sin(tauD*phase+a)));}
   case 12:{ // Clocked deterministic noise excites a pitched resonator.
    phases[0]+=dt*(1+a*48);if(phases[0]>=1){phases[0]=wrapD(phases[0]);memory=noise();}return float(std::sin(tauD*phase+memory*shape*6)*(1-b*.5)+memory*b*.5);}
  }return 0;
 }
};
// Prepared delay storage; process() never allocates. Each source/master owns its state.
class DeviceEffect {
 std::array<std::vector<float>,2> line;size_t head=0;double rate=48000,phase=0;int activeType=-1;
 std::array<float,2> low{},band{},hold{},damp{},env{};int holdCounter=0;
 float read(int c,double samples)const{double p=double(head)-std::clamp(samples,1.,double(line[c].size()-2));while(p<0)p+=line[c].size();auto i=size_t(p);float f=float(p-i);return line[c][i]*(1-f)+line[c][(i+1)%line[c].size()]*f;}
 public:
 void prepare(double sr){rate=sr;for(auto&c:line)c.assign(size_t(sr*2+4),0);reset();}
 void reset(){for(auto&c:line)std::fill(c.begin(),c.end(),0);head=0;phase=0;low={};band={};hold={};damp={};env={};holdCounter=0;activeType=-1;}
 std::pair<float,float> process(float l,float r,const DeviceSettings& p){int type=int(p.fxType);if(type<=0||line[0].empty())return {l,r};
  // State remains bounded on device changes; clear just the small filter state on the audio thread.
  if(type!=activeType){low={};band={};hold={};damp={};env={};activeType=type;}
  float input[2]={l,r},wet[2]={};float a=p.fxA,b=p.fxB,c=p.fxC;phase=wrapD(phase+(.05+a*18)/rate);++holdCounter;
  for(int ch=0;ch<2;ch++){
   float x=input[ch],y=x;env[ch]+=float((std::abs(x)-env[ch])*(std::abs(x)>env[ch]?.01:.0005));
   if(type==1){float z=read(ch,rate*(.008+a*.95));damp[ch]+=(.03f+c*.9f)*(z-damp[ch]);line[ch][head]=std::tanh(x+damp[ch]*b*.91f);y=damp[ch];}
   if(type==2){float z=(read(ch,rate*(.013+a*.31))+read(1-ch,rate*(.019+a*.47))+read(ch,rate*(.031+a*.71)))*.333f;line[ch][head]=std::tanh(x+z*b*.9f);y=z;}
   if(type==3){double cutoff=60*std::pow(220.,std::clamp(double(a+env[ch]*c*.8f),0.,1.));float f=float(2*std::sin(3.141592653589793*std::min(cutoff,rate*.12)/rate));low[ch]+=f*band[ch];float high=x-low[ch]-(2-b*1.85f)*band[ch];band[ch]=std::tanh(band[ch]+f*high);y=low[ch]*(1-c)+high*c;}
   if(type==4){float k=float(1-std::exp(-tauD*(200+a*2400)/rate));low[ch]+=k*(x-low[ch]);float hp=x-low[ch];band[ch]+=.25f*(hp-band[ch]);int period=1+int(b*80);if(holdCounter%period==1%period){float steps=4+c*60;hold[ch]=std::round(band[ch]*steps)/steps;}y=hold[ch]*float(.6+.4*std::sin(tauD*phase*(1+c*12)));}
   if(type==5){float k=.01f+a*.65f;low[ch]+=k*(x-low[ch]);float z=std::tanh(low[ch]*(1+b*18));y=z/(1+env[ch]*c*8)*(1+b);}
   if(type==6){float z=(read(ch,rate*(.023+a*.027))+read(1-ch,rate*(.037+a*.041)))*.5f;damp[ch]+=(.03f+c*.6f)*(z-damp[ch]);line[ch][head]=std::tanh(x*.6f+damp[ch]*(.45f+b*.48f));y=z;}
   if(type==7){float z=read(ch,rate*(.015+b*.48));line[ch][head]=std::tanh(x+z*c*.85f); // SSB frequency shift using a quadrature all-pass approximation.
    float ap=-.6f*z+low[ch];low[ch]=z+.6f*ap;double angle=tauD*wrapD(phase*(1+a*12));y=float(z*std::cos(angle)-ap*std::sin(angle));}
   wet[ch]=std::isfinite(y)?std::clamp(y,-4.f,4.f):0;
  }head=(head+1)%line[0].size();return {l+(wet[0]-l)*p.fxMix,r+(wet[1]-r)*p.fxMix};
 }
};
}
