#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace pxl {
constexpr double pi = 3.14159265358979323846;
class Engine {
public:
  using Stereo = std::array<std::vector<float>, 2>;
  std::array<Stereo, 4> tape;
  Stereo sample, clipboard;
  std::array<float, 4> trackGain{{.8f, .8f, .8f, .8f}}, trackPan{};
  std::array<bool, 4> trackMute{}, trackSolo{};
  std::array<int, 4> used{};
  std::array<bool, 16> sequence{};
  std::array<int, 16> seqNotes{
      {60, 62, 64, 67, 60, 62, 64, 69, 60, 62, 64, 67, 60, 62, 59, 55}};
  std::array<std::array<bool, 16>, 4> drumSequence{};
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

  void prepare(double sr, double seconds = 60) {
    rate = std::clamp(sr, 1000.0, 192000.0);
    capacity = std::max(2, int(rate * std::clamp(seconds, 1.0, 60.0)));
    for (auto &t : tape)
      for (auto &c : t)
        c.assign(size_t(capacity), 0);
    for (auto &c : undoTape)
      c.assign(size_t(capacity), 0);
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
    panic();
  }
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
    seqCounter = 0;
    currentStep = -1;
    seqLastNote = -1;
    sustain = false;
  }
  void rewind() {
    position = loopStart;
    seqCounter = 0;
    currentStep = -1;
  }
  void noteOn(int note, float velocity) {
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
    v->frequency = 440 * std::pow(2., (v->note - 69) / 12.);
    v->pluckLength =
        std::clamp(int(rate / v->frequency), 2, int(v->pluck.size()));
    for (int i = 0; i < v->pluckLength; i++)
      v->pluck[size_t(i)] = noise() * .7f;
  }
  void noteOff(int note) {
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
    d.velocity = velocity;
  }
  std::pair<float, float> tick(float inL, float inR) {
    if (capacity < 2)
      return {};
    loopStart = std::clamp(loopStart, 0, capacity - 1);
    loopEnd = std::clamp(loopEnd, loopStart + 1, capacity);
    if (!std::isfinite(position) || position < loopStart || position >= loopEnd)
      position = loopStart;
    if (playing && (seqEnabled || drumSeqEnabled))
      advanceSequence();
    float sourceL = 0, sourceR = 0;
    for (auto &v : voices)
      if (v.active) {
        if (v.releasing)
          v.env *= float(std::exp(-6.9 / (std::max(.02f, release) * rate)));
        else
          v.env = std::min(
              1.f, v.env + float(1. / (std::max(.001f, attack) * rate)));
        if (v.releasing && v.env < .0001f) {
          v.active = false;
          continue;
        }
        v.phase += v.frequency / rate;
        v.phase -= std::floor(v.phase);
        float l = 0, r = 0;
        double p = v.phase;
        switch (sound) {
        case 0: {
          float saw = float(2 * p - 1) - polyBlep(p, v.frequency / rate);
          double q = std::fmod(p + .5 + shape * .4, 1.);
          float pulse = (p < (.15 + shape * .7) ? 1.f : -1.f) +
                        polyBlep(p, v.frequency / rate) -
                        polyBlep(std::fmod(p + 1 - (.15 + shape * .7), 1.),
                                 v.frequency / rate);
          l = .65f * saw + .35f * pulse;
          (void)q;
          break;
        }
        case 1:
          l = float(
              std::sin(2 * pi * p +
                       shape * 8 * std::sin(2 * pi * p * (tone < .5 ? 2 : 3))));
          break;
        case 2:
          l = float((std::sin(2 * pi * p) + shape * .7 * std::sin(4 * pi * p) +
                     tone * .5 * std::sin(8 * pi * p)) *
                    .55);
          break;
        case 3: {
          auto i = v.pluckIndex;
          auto j = (i + 1) % v.pluckLength;
          l = v.pluck[size_t(i)];
          v.pluck[size_t(i)] =
              (l + v.pluck[size_t(j)]) * (.485f + tone * .014f);
          v.pluckIndex = j;
          break;
        }
        case 4: {
          if (sample[0].empty()) {
            l = 0;
            break;
          }
          auto n = int(v.samplePos);
          if (n >= int(sample[0].size()) - 1) {
            v.active = false;
            continue;
          }
          float f = float(v.samplePos - n);
          l = sample[0][size_t(n)] * (1 - f) + sample[0][size_t(n + 1)] * f;
          r = sample[1][size_t(n)] * (1 - f) + sample[1][size_t(n + 1)] * f;
          v.samplePos += std::pow(2., (v.note - 60) / 12.);
          break;
        }
        default:
          break;
        }
        if (sound != 4)
          r = l;
        float coef = std::clamp(
            float(1 - std::exp(-2 * pi * (250 + tone * 15000) / rate)), .01f,
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
      double t = d.age / rate;
      float x = 0;
      int kind = int(k) % 4;
      double variant = k >= 4 ? 1.35 : 1.;
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
      sourceL += x * d.velocity * drumGain * .5f;
      sourceR += x * d.velocity * drumGain * .5f;
      d.age++;
      if (t > .9)
        d.active = false;
    }
    sourceL += std::isfinite(inL) ? inL : 0;
    sourceR += std::isfinite(inR) ? inR : 0;
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
          float l = tape[size_t(t)][0][size_t(index)] * (1 - frac) +
                    tape[size_t(t)][0][size_t(next)] * frac;
          float r = tape[size_t(t)][1][size_t(index)] * (1 - frac) +
                    tape[size_t(t)][1][size_t(next)] * frac;
          auto g = gains(t);
          outL += l * g.first;
          outR += r * g.second;
        }
      if (recording && std::abs(speed - 1) < .000001 && armed >= 0 &&
          armed < 4) {
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
    for (int c = 0; c < 2; c++)
      std::copy(tape[size_t(track)][size_t(c)].begin(),
                tape[size_t(track)][size_t(c)].end(),
                undoTape[size_t(c)].begin());
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
    std::swap(used[size_t(undoTrack)], undoUsed);
    return true;
  }
  void copy(int track) {
    if (track < 0 || track > 3)
      return;
    auto s = selection();
    for (int c = 0; c < 2; c++)
      clipboard[size_t(c)].assign(
          tape[size_t(track)][size_t(c)].begin() + s.first,
          tape[size_t(track)][size_t(c)].begin() + s.second);
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
    for (auto &c : tape[size_t(track)])
      std::fill(c.begin() + s.first, c.begin() + s.second, 0);
    if (s.first == 0 && s.second >= used[size_t(track)])
      used[size_t(track)] = 0;
  }
  void paste(int track) {
    if (track < 0 || track > 3 || clipboard[0].empty())
      return;
    checkpoint(track);
    int dest = std::clamp(int(position), loopStart, loopEnd - 1);
    int n = std::min(int(clipboard[0].size()), loopEnd - dest);
    for (int c = 0; c < 2; c++)
      std::copy_n(clipboard[size_t(c)].begin(), n,
                  tape[size_t(track)][size_t(c)].begin() + dest);
    used[size_t(track)] = std::max(used[size_t(track)], dest + n);
  }
  void reverseSelection(int track) {
    if (track < 0 || track > 3)
      return;
    checkpoint(track);
    auto s = selection();
    for (auto &c : tape[size_t(track)])
      std::reverse(c.begin() + s.first, c.begin() + s.second);
    used[size_t(track)] = std::max(used[size_t(track)], s.second);
  }
  void bounce() {
    checkpoint(3);
    bool solo = std::any_of(trackSolo.begin(), trackSolo.end(),
                            [](bool x) { return x; });
    for (int i = loopStart; i < loopEnd; i++) {
      float l = 0, r = 0;
      for (int t = 0; t < 4; t++)
        if (!trackMute[size_t(t)] && (!solo || trackSolo[size_t(t)])) {
          auto g = gains(t);
          l += tape[size_t(t)][0][size_t(i)] * g.first;
          r += tape[size_t(t)][1][size_t(i)] * g.second;
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
    bool active = false, releasing = false, held = true;
    int note = 60, pluckLength = 2, pluckIndex = 0;
    double phase = 0, frequency = 440, samplePos = 0;
    float env = 0, velocity = 1, filterL = 0, filterR = 0;
    std::array<float, 8192> pluck{};
  };
  struct Drum {
    bool active = false;
    int64_t age = 0;
    double phase = 0;
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
  int seqLastNote = -1;
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
    if (seqCounter <= 0) {
      if (seqLastNote >= 0)
        noteOff(seqLastNote);
      currentStep = (currentStep + 1) % 16;
      if (seqEnabled && sequence[size_t(currentStep)]) {
        seqLastNote = quantize(seqNotes[size_t(currentStep)]);
        noteOn(seqLastNote, .8f);
      } else
        seqLastNote = -1;
      if (drumSeqEnabled)
        for (int d = 0; d < 4; d++)
          if (drumSequence[size_t(d)][size_t(currentStep)])
            triggerDrum(d, .85f);
      double base = rate * 60 / (std::clamp(bpm, 40., 240.) * 4);
      seqCounter += base * (currentStep % 2 == 0 ? 1 + swing : 1 - swing);
    }
    seqCounter -= 1;
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
    l = (l + wet[0] * reverbMix) * master;
    r = (r + wet[1] * reverbMix) * master;
    return {std::isfinite(l) ? std::clamp(l, -1.f, 1.f) : 0,
            std::isfinite(r) ? std::clamp(r, -1.f, 1.f) : 0};
  }
};
} // namespace pxl
