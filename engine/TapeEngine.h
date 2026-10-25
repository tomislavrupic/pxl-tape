#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>
#include "Devices.h"

namespace pxl {
constexpr double pi = 3.14159265358979323846;
class Engine {
public:
  using Stereo = std::array<std::vector<float>, 2>;
  std::array<Stereo, 4> tape;
  Stereo sample, clipboard;
  DeviceSettings device;
  float modulationCC=0, modulationValue=0;
  std::array<bool,128> heldNotes{};
  std::array<float, 4> trackGain{{.8f, .8f, .8f, .8f}}, trackPan{};
  std::array<bool, 4> trackMute{}, trackSolo{};
  std::array<int, 4> used{};
  std::array<bool, 128> sequence{};
  std::array<int, 128> seqNotes{
      {60, 62, 64, 67, 60, 62, 64, 69, 60, 62, 64, 67, 60, 62, 59, 55}};
  std::array<std::array<bool, 128>, 4> drumSequence{};
  bool playing = false, recording = false, monitor = true, seqEnabled = false,
       drumSeqEnabled = false;
  int armed = 0, sound = 0, loopStart = 0, loopEnd = 0, selectStart = 0,
      selectEnd = 0, capacity = 0, currentStep = -1;
  double rate = 48000, position = 0, speed = 1, bpm = 120, swing = 0;
  float tone = .5f, shape = .35f, attack = .006f, release = .4f,
        instrumentGain = .65f, drumGain = .7f;
  float drive = 0, delayMix = .12f, delayFeedback = .3f, reverbMix = .12f,
        master = .8f;
  int root = 0, scale = 0;
  bool sustain = false;
  float meterL = 0, meterR = 0, sourceMeter = 0;
  std::array<float, 256> scope{};
  size_t scopeHead = 0;

  void prepare(double sr, double seconds = 360) {
    rate = std::clamp(sr, 1000.0, 192000.0);
    capacity = std::max(2, int(rate * std::clamp(seconds, 1.0, 360.0)));
    for (auto &t : tape)
      for (auto &c : t)
        c.assign(size_t(std::min(capacity,int(rate*16))), 0);
    for (auto &c : undoTape)c.clear();
    delay[0].assign(size_t(rate * 2 + 2), 0);
    delay[1].assign(size_t(rate * 2 + 2), 0);
    const double lengths[] = {.0297, .0371, .0411, .0437};
    for (int c = 0; c < 2; c++)
      for (int i = 0; i < 4; i++)
        reverbs[c][i].assign(
            size_t(std::max(2, int(rate * (lengths[i] + c * .0017)))), 0);
    loopEnd = std::min(capacity, int(rate * 16));
    loopStart = 0;
    position = 0;
    used.fill(0);
    undoTrack = -1;
    sourceDevice.prepare(rate);masterDevice.prepare(rate);
    for(int i=16;i<128;i++)seqNotes[size_t(i)]=60;
    panic();
  }
  void reserveTrack(int track,int frames){if(track<0||track>3)return;frames=std::clamp(frames,0,capacity);for(auto& c:tape[size_t(track)])if(int(c.size())<frames)c.resize(size_t(frames),0);}
  float readTape(int track,int channel,int frame)const{if(track<0||track>3||channel<0||channel>1||frame<0)return 0;const auto& data=tape[size_t(track)][size_t(channel)];return size_t(frame)<data.size()?data[size_t(frame)]:0.f;}
  void panic() {
    for (auto &v : voices)
      v.active = false;
    for (auto &d : drums)
      d.active = false;
    for (auto &c : delay)
      std::fill(c.begin(), c.end(), 0);
    delayIndex = 0;
    for (auto &c : reverbs)
      for (auto &r : c)
        std::fill(r.begin(), r.end(), 0);
    revIndex.fill(0);
    filterL = filterR = 0;
    seqCounter = 0;seqTick=0;gateCounter=0;seqMode=-1;ballY={};ballVelocity={};
    currentStep = -1;
    seqLastNote = -1;
    sustain = false;heldNotes.fill(false);sourceDevice.reset();masterDevice.reset();modPhase=0;modHeld=0;eqLowState={};eqHighState={};compressionEnv=0;
  }
  void rewind() {
    position = loopStart;
    seqCounter = 0;seqTick=0;gateCounter=0;seqMode=-1;ballY={};ballVelocity={};
    currentStep = -1;
  }
  void noteOn(int note, float velocity) {
    note=std::clamp(note,0,127);heldNotes[size_t(note)]=true;lastVelocity=velocity;
    if(int(device.seqType)==2)fingerBank=(note-48+128)%8;
    if(seqEnabled&&int(device.seqType)==5)return;
    if(device.playMode>.5f)for(auto& v:voices)v.active=false;
    triggerVoice(note,velocity);
  }
  void triggerVoice(int note, float velocity) {
    Voice *v = nullptr;
    for (auto &x : voices)
      if (!x.active) {
        v = &x;
        break;
      }
    if (!v)
      v = &voices[voiceSteal++ % voices.size()];
    *v = Voice{};
    v->active = true;
    v->note = std::clamp(note, 0, 127);
    v->velocity = std::clamp(velocity, 0.f, 1.f);
    v->dsp.seed=uint32_t(0x6d2b79f5+v->note*7919);
    int sampleFrames=int(std::min(sample[0].size(),sample[1].size()));int start=std::clamp(int(sampleFrames*device.sampleStart),0,std::max(0,sampleFrames-2)),end=std::clamp(int(sampleFrames*device.sampleEnd),start+2,std::max(2,sampleFrames));v->samplePos=device.sampleReverse>.5f?end-2:start;
    v->frequency = 440 * std::pow(2., (v->note - 69) / 12.);
    v->pluckLength =
        std::clamp(int(rate / v->frequency), 2, int(v->pluck.size()));
    for (int i = 0; i < v->pluckLength; i++)
      v->pluck[size_t(i)] = noise() * .7f;
  }
  void noteOff(int note) {
    if(note>=0&&note<128)heldNotes[size_t(note)]=false;
    if(device.playMode<1.5f)releaseVoice(note);
  }
  void releaseVoice(int note) {
    for (auto &v : voices)
      if (v.active && v.note == note) {
        v.held = false;
        if (!sustain)
          v.releasing = true;
      }
  }
  void setSustain(bool on) {
    sustain = on;
    if (!on)
      for (auto &v : voices)
        if (!v.held)
          v.releasing = true;
  }
  void allNotesOff() {
    heldNotes.fill(false);
    for (auto &v : voices) {
      v.held = false;
      v.releasing = true;
    }
  }
  void triggerDrum(int index, float velocity = 1) {
    auto &d = drums[size_t(std::clamp(index, 0, 7))];
    d.active = true;
    d.age = 0;
    d.phase = 0;
    d.velocity = std::clamp(velocity,0.f,1.f);d.samplePos=0;
  }
  std::pair<float, float> tick(float inL, float inR) {
    if (capacity < 2)
      return {};
    loopStart = std::clamp(loopStart, 0, capacity - 1);
    loopEnd = std::clamp(loopEnd, loopStart + 1, capacity);
    if (!std::isfinite(position) || position < loopStart || position >= loopEnd)
      position = loopStart;
    modPhase+=device.modRate/rate;
    if(modPhase>=1){modPhase-=std::floor(modPhase);modHeld=noise();}
    float mv=0;switch(int(device.modType)){case 1:mv=float(std::sin(2*pi*modPhase));break;case 2:mv=float(1-4*std::abs(modPhase-.5));break;case 3:mv=modHeld;break;case 4:mv=lastVelocity*2-1;break;case 5:mv=modulationCC*2-1;break;case 6:mv=std::min(1.f,sourceMeter*4)*2-1;break;}
    modulationValue=mv*device.modDepth;
    const float liveTone=std::clamp(tone+(int(device.modTarget)==0?modulationValue*.45f:0.f),0.f,1.f);
    const float liveShape=std::clamp(shape+(int(device.modTarget)==1?modulationValue*.45f:0.f),0.f,1.f);
    if (playing && (seqEnabled || drumSeqEnabled))
      advanceSequence();
    float sourceL = 0, sourceR = 0;
    for (auto &v : voices)
      if (v.active) {
        if (v.releasing)
          v.env *= float(std::exp(-6.9 / (std::max(.02f, release) * rate)));
        else if(!v.decaying){v.env=std::min(1.f,v.env+float(1./(std::max(.001f,attack)*rate)));if(v.env>=1)v.decaying=true;}
        else v.env += ((device.playMode>1.5f?0.f:device.sustainLevel)-v.env)*float(1-std::exp(-6.9/(std::max(.005f,device.decay)*rate)));
        if ((v.releasing||(device.playMode>1.5f&&v.decaying)) && v.env < .0001f) {
          v.active = false;
          continue;
        }
        v.phase += v.frequency * std::pow(2.,int(device.modTarget)==2?modulationValue:0.f) / rate;
        v.phase -= std::floor(v.phase);
        float l = 0, r = 0;
        double p = v.phase;
        switch (sound) {
        case 0: {
          float saw = float(2 * p - 1) - polyBlep(p, v.frequency / rate);
          double q = std::fmod(p + .5 + liveShape * .4, 1.);
          float pulse = (p < (.15 + liveShape * .7) ? 1.f : -1.f) +
                        polyBlep(p, v.frequency / rate) -
                        polyBlep(std::fmod(p + 1 - (.15 + liveShape * .7), 1.),
                                 v.frequency / rate);
          l = .65f * saw + .35f * pulse;
          (void)q;
          break;
        }
        case 1:{
          double op4=std::sin(2*pi*p*(1+std::floor(device.macro3*7)));
          double op3=std::sin(2*pi*p*2+op4*device.macro4*3);
          double op2=std::sin(2*pi*p*(liveTone<.5?2:3)+op3*device.macro3*3);
          l=float(std::sin(2*pi*p+liveShape*8*op2));break;
        }
        case 2:
          l = float((std::sin(2 * pi * p) + liveShape * .7 * std::sin(4 * pi * p) +
                     liveTone * .5 * std::sin(8 * pi * p)) *
                    .55);
          break;
        case 3: {
          auto i = v.pluckIndex;
          auto j = (i + 1) % v.pluckLength;
          l = v.pluck[size_t(i)];
          v.pluck[size_t(i)] =
              (l + v.pluck[size_t(j)]) * (.485f + liveTone * .014f);
          v.pluckIndex = j;
          break;
        }
        case 4: {
          int frames=int(std::min(sample[0].size(),sample[1].size()));
          if(frames<2){v.active=false;continue;}
          int start=std::clamp(int(frames*device.sampleStart),0,frames-2),end=std::clamp(int(frames*device.sampleEnd),start+2,frames);
          if(v.samplePos<start||v.samplePos>=end-1){if(device.sampleLoop>.5f){double length=end-start-1;v.samplePos=start+std::fmod(v.samplePos-start,length);if(v.samplePos<start)v.samplePos+=length;}else{v.active=false;continue;}}
          int n=std::clamp(int(v.samplePos),start,end-2);float f=float(v.samplePos-n);
          l=sample[0][size_t(n)]*(1-f)+sample[0][size_t(n+1)]*f;r=sample[1][size_t(n)]*(1-f)+sample[1][size_t(n+1)]*f;
          v.samplePos+=std::pow(2.,(v.note-60)/12.)*(device.sampleReverse>.5f?-1:1);
          break;
        }
        default:
          l=v.dsp.render(sound,p,v.frequency,rate,liveTone,liveShape,device.macro3,device.macro4);break;
        }
        if (sound != 4)
          r = l;
        float coef = std::clamp(
            float(1 - std::exp(-2 * pi * (250 + liveTone * 15000) / rate)), .01f,
            1.f);
        v.filterL += coef * (l - v.filterL);
        v.filterR += coef * (r - v.filterR);
        float gain = v.env * v.velocity * instrumentGain * .23f;
        sourceL += v.filterL * gain;
        sourceR += v.filterR * gain;
      }
    for (size_t k = 0; k < drums.size(); k++) {
      auto &d = drums[k];
      if (!d.active)
        continue;
      double t = d.age / rate / std::max(.1f,device.drumDecay);
      float x = 0;
      int kind = int(k) % 4;
      double variant = (k >= 4 ? 1.35 : 1.)*std::pow(2.,device.drumPitch/12.);
      if (kind == 0) {
        double f = (45 + 100 * std::exp(-t * 35)) * variant;
        d.phase += f / rate;
        x = float(std::sin(2 * pi * d.phase) * std::exp(-t * 13));
      }
      if (kind == 1)
        x = float((noise() * .75 + std::sin(2 * pi * 180 * variant * t) * .25) *
                  std::exp(-t * 20));
      if (kind == 2) {
        float n = noise();
        x = (n - d.previous) * .45f * float(std::exp(-t * (k >= 4 ? 12 : 65)));
        d.previous = n;
      }
      if (kind == 3)
        x = noise() * float(std::exp(-t * 17)) *
            (std::fmod(t, .018) < .009 ? 1.f : .35f);
      if(device.drumMode>.5f){int frames=int(std::min(sample[0].size(),sample[1].size()));int start=int(k)*frames/8,end=int(k+1)*frames/8;if(end-start<2){d.active=false;continue;}int n=start+int(d.samplePos);if(n>=end-1){d.active=false;continue;}float f=float(d.samplePos-std::floor(d.samplePos));x=(sample[0][size_t(n)]*(1-f)+sample[0][size_t(n+1)]*f+sample[1][size_t(n)]*(1-f)+sample[1][size_t(n+1)]*f)*.5f;d.samplePos+=std::pow(2.,device.drumPitch/12.);}
      else x=std::tanh(x*(.5f+device.drumColour*1.5f)+float(std::sin(2*pi*400*variant*t))*x*device.drumShape);
      sourceL += x * d.velocity * drumGain * .5f;
      sourceR += x * d.velocity * drumGain * .5f;
      d.age++;
      if (t > .9)
        d.active = false;
    }
    sourceL += std::isfinite(inL) ? inL : 0;
    sourceR += std::isfinite(inR) ? inR : 0;
    if(int(device.fxRoute)==0){auto settings=device;if(int(device.modTarget)==3)settings.fxA=std::clamp(settings.fxA+modulationValue*.4f,0.f,1.f);auto fx=sourceDevice.process(sourceL,sourceR,settings);sourceL=fx.first;sourceR=fx.second;}
    sourceMeter = std::max(std::abs(sourceL), std::abs(sourceR));
    float outL = monitor ? sourceL : 0, outR = monitor ? sourceR : 0;
    int index = int(position);
    float frac = float(position - index);
    int next = index + 1 < loopEnd ? index + 1 : loopStart;
    bool anySolo = std::any_of(trackSolo.begin(), trackSolo.end(),
                               [](bool x) { return x; });
    if (playing) {
      for (int t = 0; t < 4; t++)
        if (!trackMute[size_t(t)] && (!anySolo || trackSolo[size_t(t)])) {
          float l = readTape(t,0,index) * (1 - frac) +
                    readTape(t,0,next) * frac;
          float r = readTape(t,1,index) * (1 - frac) +
                    readTape(t,1,next) * frac;
          auto g = gains(t);
          outL += l * g.first;
          outR += r * g.second;
        }
      if (recording && std::abs(speed - 1) < .000001 && armed >= 0 &&
          armed < 4 && size_t(index)<tape[size_t(armed)][0].size()) {
        auto &t = tape[size_t(armed)];
        t[0][size_t(index)] =
            std::clamp(t[0][size_t(index)] + sourceL, -4.f, 4.f);
        t[1][size_t(index)] =
            std::clamp(t[1][size_t(index)] + sourceR, -4.f, 4.f);
        used[size_t(armed)] = std::max(used[size_t(armed)], index + 1);
      }
      const double s = std::isfinite(speed) ? std::clamp(speed, -2., 2.) : 1.;
      double len = loopEnd - loopStart;
      position = loopStart + std::fmod(position - loopStart + s + len * 2, len);
    }
    auto out = effects(outL, outR);
    scope[scopeHead++ % scope.size()] = out.first;
    meterL = std::max(std::abs(out.first), meterL * .9995f);
    meterR = std::max(std::abs(out.second), meterR * .9995f);
    return out;
  }
  std::pair<float, float> gains(int t) const {
    float pan = std::clamp(trackPan[size_t(t)], -1.f, 1.f),
          gain = trackGain[size_t(t)];
    return {gain * (pan > 0 ? 1 - pan : 1), gain * (pan < 0 ? 1 + pan : 1)};
  }
  std::pair<int, int> selection() const {
    int a = std::clamp(selectStart, loopStart, loopEnd),
        b = std::clamp(selectEnd, loopStart, loopEnd);
    if (b <= a)
      return {loopStart, loopEnd};
    return {a, b};
  }
  void checkpoint(int track) {
    if (track < 0 || track > 3)
      return;
    undoTrack = track;
    undoUsed = used[size_t(track)];
    for(int c=0;c<2;c++)undoTape[size_t(c)].assign(tape[size_t(track)][size_t(c)].begin(),tape[size_t(track)][size_t(c)].begin()+std::min(size_t(undoUsed),tape[size_t(track)][size_t(c)].size()));
  }
  void clearUndo() {
    undoTrack = -1;
    clipboard[0].clear();
    clipboard[1].clear();
  }
  bool undo() {
    if (undoTrack < 0)
      return false;
    auto &t = tape[size_t(undoTrack)];
    for (int c = 0; c < 2; c++)
      std::swap(t[size_t(c)], undoTape[size_t(c)]);
    reserveTrack(undoTrack,std::min(capacity,int(rate*16)));
    std::swap(used[size_t(undoTrack)], undoUsed);
    return true;
  }
  void copy(int track) {
    if (track < 0 || track > 3)
      return;
    auto s = selection();
    for(int c=0;c<2;c++){clipboard[size_t(c)].assign(size_t(s.second-s.first),0);for(int i=s.first;i<s.second;i++)clipboard[size_t(c)][size_t(i-s.first)]=readTape(track,c,i);}
  }
  void cut(int track) {
    copy(track);
    clear(track);
  }
  void clear(int track) {
    if (track < 0 || track > 3)
      return;
    checkpoint(track);
    auto s = selection();
    for(auto& c:tape[size_t(track)])if(size_t(s.first)<c.size())std::fill(c.begin()+s.first,c.begin()+std::min(size_t(s.second),c.size()),0);
    if (s.first == 0 && s.second >= used[size_t(track)])
      used[size_t(track)] = 0;
  }
  void paste(int track) {
    if (track < 0 || track > 3 || clipboard[0].empty())
      return;
    checkpoint(track);
    int dest = std::clamp(int(position), loopStart, loopEnd - 1);
    int n = std::min(int(clipboard[0].size()), loopEnd - dest);
    reserveTrack(track,dest+n);
    for (int c = 0; c < 2; c++)
      std::copy_n(clipboard[size_t(c)].begin(), n,
                  tape[size_t(track)][size_t(c)].begin() + dest);
    used[size_t(track)] = std::max(used[size_t(track)], dest + n);
  }
  void reverseSelection(int track) {
    if (track < 0 || track > 3)
      return;
    checkpoint(track);
    auto s = selection();reserveTrack(track,s.second);
    for (auto &c : tape[size_t(track)])
      std::reverse(c.begin() + s.first, c.begin() + s.second);
    used[size_t(track)] = std::max(used[size_t(track)], s.second);
  }
  void bounce() {
    checkpoint(3);reserveTrack(3,loopEnd);
    bool solo = std::any_of(trackSolo.begin(), trackSolo.end(),
                            [](bool x) { return x; });
    for (int i = loopStart; i < loopEnd; i++) {
      float l = 0, r = 0;
      for (int t = 0; t < 4; t++)
        if (!trackMute[size_t(t)] && (!solo || trackSolo[size_t(t)])) {
          auto g = gains(t);
          l += readTape(t,0,i) * g.first;
          r += readTape(t,1,i) * g.second;
        }
      tape[3][0][size_t(i)] = std::clamp(l, -4.f, 4.f);
      tape[3][1][size_t(i)] = std::clamp(r, -4.f, 4.f);
    }
    used[3] = std::max(used[3], loopEnd);
  }
  void clearAll() {
    for (auto &t : tape)
      for (auto &c : t)
        std::fill(c.begin(), c.end(), 0);
    used.fill(0);
    undoTrack = -1;
    rewind();
    panic();
  }
  int quantize(int note) const {
    if (scale == 0)
      return std::clamp(note, 0, 127);
    static const int major[] = {0, 2, 4, 5, 7, 9, 11},
                     minor[] = {0, 2, 3, 5, 7, 8, 10};
    auto *s = scale == 1 ? major : minor;
    int relative = (note - root + 120) % 12;
    int best = 0;
    for (int i = 0; i < 7; i++)
      if (s[i] <= relative)
        best = s[i];
    return std::clamp(note - relative + best, 0, 127);
  }

private:
  struct Voice {
    bool active = false, releasing = false, held = true, decaying=false;
    DeviceVoice dsp;
    int note = 60, pluckLength = 2, pluckIndex = 0;
    double phase = 0, frequency = 440, samplePos = 0;
    float env = 0, velocity = 1, filterL = 0, filterR = 0;
    std::array<float, 8192> pluck{};
  };
  struct Drum {
    bool active = false;
    int64_t age = 0;
    double phase = 0, samplePos=0;
    float velocity = 1, previous = 0;
  };
  std::array<Voice, 8> voices{};
  std::array<Drum, 8> drums{};
  size_t voiceSteal = 0;
  uint32_t random = 0x12345678;
  Stereo undoTape, delay;
  int undoTrack = -1, undoUsed = 0;
  size_t delayIndex = 0;
  std::array<std::array<std::vector<float>, 4>, 2> reverbs;
  std::array<size_t, 4> revIndex{};
  float filterL = 0, filterR = 0;
  double seqCounter = 0;
  int seqLastNote = -1,seqTick=0,seqMode=-1,fingerBank=0;
  double gateCounter=0,modPhase=0;float modHeld=0,lastVelocity=.8f;
  std::array<double,16> ballY{},ballVelocity{};
  DeviceEffect sourceDevice,masterDevice;
  std::array<float,2> eqLowState{},eqHighState{};float compressionEnv=0;
  float noise() {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return float(double(random) / 2147483648. - 1.);
  }
  static float polyBlep(double t, double dt) {
    if (dt <= 0 || dt >= .5)
      return 0;
    if (t < dt) {
      t /= dt;
      return float(t + t - t * t - 1);
    }
    if (t > 1 - dt) {
      t = (t - 1) / dt;
      return float(t * t + t + t + 1);
    }
    return 0;
  }
  void advanceSequence() {
    int mode=std::clamp(int(device.seqType),0,5),length=std::clamp(int(device.seqLength),1,128);
    if(mode!=seqMode){for(auto& v:voices){v.held=false;v.releasing=true;}seqCounter=0;seqTick=0;seqLastNote=-1;seqMode=mode;if(mode==3)for(int i=0;i<16;i++){ballY[i]=(i*13%17)/17.;ballVelocity[i]=-.08-i*.017;}}
    if(gateCounter>0&&--gateCounter<=0&&seqLastNote>=0){releaseVoice(seqLastNote);seqLastNote=-1;}
    if(seqCounter<=0){
      if(seqLastNote>=0)releaseVoice(seqLastNote);
      if(mode==0)length=std::min(16,length);int step=seqTick%length;
      if(int(device.seqDirection)==1)step=length-1-step;
      else if(int(device.seqDirection)==2)step=std::clamp(int((noise()+1)*.5f*length),0,length-1);
      if(mode==2){int local=seqTick%16;if(int(device.seqDirection)==1)local=15-local;else if(int(device.seqDirection)==2)local=std::clamp(int((noise()+1)*8),0,15);step=(fingerBank*16+local)%128;}
      if(mode==3){ // Bouncing note particles: gravity and restitution determine which note hits next.
        double best=-1;int chosen=0;
        for(int i=0;i<std::min(length,16);i++){ballVelocity[i]+=.02+device.gravity*.2;ballY[i]+=ballVelocity[i];if(ballY[i]>=1){ballY[i]=2-ballY[i];ballVelocity[i]=-std::abs(ballVelocity[i])*(.6+.35*device.macro4);if(ballVelocity[i]>-.04)ballVelocity[i]=-.1-i*.015;}if(ballY[i]>best){best=ballY[i];chosen=i;}}
        step=chosen;
      }
      if(mode==0||mode==3)step=(step%16)+std::clamp(int(device.seqBank),0,7)*16;
      currentStep=step;seqLastNote=-1;
      if(seqEnabled){int note=-1;
        if(mode==5){std::array<int,128> chord{};int count=0;for(int n=0;n<128;n++)if(heldNotes[size_t(n)])chord[size_t(count++)]=n;
          if(count){int total=count*std::clamp(int(device.seqOctaves),1,4),pos=seqTick%total;if(int(device.seqDirection)==1)pos=total-1-pos;else if(int(device.seqDirection)==2)pos=std::clamp(int((noise()+1)*.5f*total),0,total-1);note=chord[size_t(pos%count)]+12*(pos/count);}}
        else if(sequence[size_t(step)]||mode==4)note=seqNotes[size_t(step)];
        if(note>=0){seqLastNote=quantize(note);triggerVoice(seqLastNote,.8f);}
      }
      if(drumSeqEnabled)for(int d=0;d<4;d++)if(drumSequence[size_t(d)][size_t(step)])triggerDrum(d,.85f);
      double base=rate*60/(std::clamp(bpm,40.,240.)*4);
      seqCounter+=base*(seqTick%2==0?1+swing:1-swing);gateCounter=seqCounter*device.seqGate;seqTick=(seqTick+1)%16384;
    }seqCounter-=1;
  }
  std::pair<float, float> effects(float l, float r) {
    if (drive > .001f) {
      float g = 1 + drive * 12;
      l = std::tanh(l * g) / std::sqrt(g);
      r = std::tanh(r * g) / std::sqrt(g);
    }
    if (!delay[0].empty()) {
      int n = std::clamp(int(rate * 60 / std::clamp(bpm, 40., 240.) * .75), 1,
                         int(delay[0].size()) - 1);
      size_t read =
          (delayIndex + delay[0].size() - size_t(n)) % delay[0].size();
      float dl = delay[0][read], dr = delay[1][read];
      delay[0][delayIndex] = std::clamp(l + dr * delayFeedback, -4.f, 4.f);
      delay[1][delayIndex] = std::clamp(r + dl * delayFeedback, -4.f, 4.f);
      l += dl * delayMix;
      r += dr * delayMix;
      delayIndex = (delayIndex + 1) % delay[0].size();
    }
    float wet[2] = {0, 0};
    for (int i = 0; i < 4; i++) {
      for (int c = 0; c < 2; c++) {
        auto &v = reverbs[size_t(c)][size_t(i)];
        if (v.empty())
          continue;
        auto index = revIndex[size_t(i)] % v.size();
        float old = v[index];
        v[index] = std::clamp((c == 0 ? l : r) * .2f + old * .77f, -4.f, 4.f);
        wet[c] += old * .25f;
      }
      revIndex[size_t(i)]++;
    }
    l = l + wet[0] * reverbMix;r = r + wet[1] * reverbMix;
    if(int(device.fxRoute)==1){auto settings=device;if(int(device.modTarget)==3)settings.fxA=std::clamp(settings.fxA+modulationValue*.4f,0.f,1.f);auto fx=masterDevice.process(l,r,settings);l=fx.first;r=fx.second;}
    float values[2]={l,r};
    for(int ch=0;ch<2;ch++){float x=values[ch];eqLowState[ch]+=float(1-std::exp(-2*pi*200/rate))*(x-eqLowState[ch]);eqHighState[ch]+=float(1-std::exp(-2*pi*3500/rate))*(x-eqHighState[ch]);values[ch]=eqLowState[ch]*float(std::pow(10.,device.eqLow/20.))+(eqHighState[ch]-eqLowState[ch])*float(std::pow(10.,device.eqMid/20.))+(x-eqHighState[ch])*float(std::pow(10.,device.eqHigh/20.));}
    float peak=std::max(std::abs(values[0]),std::abs(values[1]));compressionEnv+=(peak-compressionEnv)*(peak>compressionEnv?.01f:.0002f);float cg=1;
    if(device.compressor>0){float threshold=.7f-device.compressor*.6f;if(compressionEnv>threshold)cg=std::pow(threshold/compressionEnv,device.compressor*.8f);}
    l=values[0]*cg*master;r=values[1]*cg*master;
    return {std::isfinite(l) ? std::clamp(l, -1.f, 1.f) : 0,
            std::isfinite(r) ? std::clamp(r, -1.f, 1.f) : 0};
  }
};
} // namespace pxl
