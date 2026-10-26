#pragma once
#include "TapeEngine.h"
namespace pxl {
inline constexpr double demoTempo=112.;
inline constexpr int demoBeats=64;
inline constexpr const char* demoName="Neon Transit";
inline float demoNoise(uint32_t n){n^=n>>16;n*=0x7feb352du;n^=n>>15;n*=0x846ca68bu;n^=n>>16;return float(n&65535)/32767.5f-1.f;}
inline double demoFrequency(int note){return 440.*std::pow(2.,(note-69)/12.);}
inline bool loadDemo(Engine& e){
 if(e.capacity<2||std::any_of(e.used.begin(),e.used.end(),[](int n){return n>0;}))return false;
 const double beatSeconds=60./demoTempo;const int n=std::min(e.capacity,int(std::round(e.rate*beatSeconds*demoBeats)));
 for(int t=0;t<4;t++)e.reserveTrack(t,n);
 const int roots[]={33,29,36,31};const int chords[4][4]={{0,3,7,14},{0,4,7,11},{0,4,7,14},{0,4,7,9}};
 for(int i=0;i<n;i++){
  double time=i/e.rate,beat=time/beatSeconds,barBeat=std::fmod(beat,4.);int bar=int(beat/4),chord=(bar/2)%4,root=roots[chord];
  bool breakdown=bar==8||bar==9;double lastKick=barBeat>=3.5?3.5:barBeat>=2?2:barBeat>=1.5?1.5:0;
  if(bar%8==7&&barBeat>=3.75)lastKick=3.75;
  double kt=(barBeat-lastKick)*beatSeconds,st=std::fmod(beat+1,2.)*beatSeconds,ht=std::fmod(beat,.5)*beatSeconds;
  float noise=demoNoise(uint32_t(i)+0x13579u),hatNoise=demoNoise(uint32_t(i)*3+0x97531u);
  double kick=std::sin(2*pi*(48*kt+2.7*(1-std::exp(-kt*35))))*std::exp(-kt*16)*.42;
  double snare=(noise*.75+std::sin(2*pi*184*st)*.25)*std::exp(-st*24)*.22;
  if(bar==0)snare=0;
  double hats=hatNoise*std::exp(-ht*85)*.064*(int(beat*2)%2?.68:1.);
  double ot=std::fmod(beat+.5,4.)*beatSeconds;
  double open=hatNoise*std::exp(-ot*15)*.04;
  if(bar%8==7&&barBeat>3){double ft=std::fmod(beat,.25)*beatSeconds;snare+=noise*std::exp(-ft*45)*.10;}
  if(breakdown){kick=0;snare*=.1;hats*=.4;}
  double drumL=kick+snare+hats+open*.55,drumR=kick+snare*.94+hats*.72+open;
  const double bassHits[]={0,1.5,2.5,3.5};double bassAt=0;int bassHit=0;for(int k=0;k<4;k++)if(barBeat>=bassHits[k]){bassAt=bassHits[k];bassHit=k;}
  double bt=(barBeat-bassAt)*beatSeconds;int bassNote=root+(bassHit==3&&bar%2?12:0);double bf=demoFrequency(bassNote);
  double bass=(std::sin(2*pi*bf*time)+.22*std::sin(2*pi*bf*2*time)+.1*std::sin(2*pi*bf*3*time))*std::exp(-bt*8)*(1-std::exp(-bt*220))*.18;
  if(bar<2)bass=0;if(breakdown)bass*=.6;
  double chordAge=std::fmod(beat,8.)*beatSeconds,chordRemain=(8-std::fmod(beat,8.))*beatSeconds;
  double chordEnv=(1-std::exp(-chordAge*4))*std::exp(-chordAge*.16)*std::min(1.,chordRemain*4)*(1-.25*std::exp(-kt*20));
  double padL=0,padR=0;for(int k=0;k<4;k++){double f=demoFrequency(root+24+chords[chord][k]);padL+=(std::sin(2*pi*f*time)+.16*std::sin(2*pi*f*2*time))*.034;padR+=(std::sin(2*pi*f*1.0015*time)+.16*std::sin(2*pi*f*1.998*time))*.034;}
  padL*=chordEnv;padR*=chordEnv;
  double leadL=0,leadR=0;const int motif[]={7,14,12,15,7,9,3,0};
  if(bar>=4)for(int echo=0;echo<3;echo++){
   double shifted=beat-echo*.75;if(shifted<16)continue;int eighth=int(std::floor(shifted*2)),hit=eighth;
   for(int j=0;j<8;j++){int q=(hit%8+8)%8;if(q==0||q==3||q==4||q==6)break;--hit;}
   double age=(shifted-hit*.5)*beatSeconds;int hc=(int(hit*.5/4)/2)%4,step=hit%8,interval=motif[step];if(hc!=0&&(interval==3||interval==15))++interval;
   double f=demoFrequency(roots[hc]+36+interval),tone=std::sin(2*pi*f*age)+.32*std::sin(2*pi*f*2.003*age);
   double v=tone*std::exp(-age*12)*(1-std::exp(-age*600))*(echo==0?.105:echo==1?.039:.017);
   double pan=(hit%2?-.35:.35)*(echo==1?-1:1);leadL+=v*(1-pan);leadR+=v*(1+pan);
  }
  double fade=std::min(1.,i/(e.rate*.006))*std::min(1.,(n-1-i)/(e.rate*.025));
  double left[]={drumL,bass,padL,leadL},right[]={drumR,bass*.96,padR,leadR};
  for(int t=0;t<4;t++){e.tape[size_t(t)][0][size_t(i)]=float(left[t]*fade);e.tape[size_t(t)][1][size_t(i)]=float(right[t]*fade);}
 }
 e.bpm=demoTempo;e.loopStart=0;e.loopEnd=n;e.used.fill(n);e.rewind();return true;
}
}
