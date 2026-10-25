# PXL Device Workstation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Expand PXL Tape into a complete family of playable synth, drum, effect, modulation and sequencing devices using our own algorithms.
**Architecture:** Shared native C++ engine with focused device DSP, appended parameter definitions and compatible project migration; JUCE native editor and browser controls expose the same state.
**Tech Stack:** C++17, JUCE 8, WASM/AudioWorklet, vanilla JS.
**Spec:** docs/superpowers/specs/2026-10-25-device-workstation.md

## Global Constraints
- Preserve sound IDs 0–4 and existing project files; new parameters default on older files.
- Own algorithms and samples; no firmware or factory content.
- Keep 60-second tape and explicitly describe this capacity.
- No allocations in audio rendering; source/device state owned per instance.

## Review Focus
- Load a legacy project after a new session: omitted new parameters must reset to defaults.
- Sample reverse/loop regions with tiny or empty samples must remain bounded.
- Sequencer switching and chord release must not leave stuck notes.
- Every exported mix must use the same master device/EQ/compressor as live playback.
- User sample/preset data must remain local and survive project roundtrip.

### Task 1: Shared devices
Files: PXLTape/Source/Devices.h, TapeEngine.h; Tests/DevicesTests.cpp.
Interfaces: Engine::device controls, noteOn/noteOff, tick(); named engines and effects are distinct algorithms, all deterministic per instance.
- [x] Write/run failing render tests for all new synth/effect families, modulation, sampler bounds and sequencer modes.
- [x] Implement distinct DSP, modulation, sample slicing and sequencer behavior; test finite output, control response, release and instance isolation.

### Task 2: State and host integration
Files: Source/DeviceParameters.h, PluginProcessor.*, browser engine/WebBridge.cpp, dist/project.js and audio-worklet.js.
Interfaces: append-only parameter table; legacy projects acquire defaults; 128-step melodic/drum storage.
- [x] Add failing legacy/new-state codec and bridge tests.
- [x] Wire all device parameters into host automation, portable projects, worklet and mix export. Compile WASM and native tests.

### Task 3: PXL controls
Files: PluginEditor.*, dist/index.html, app.js, styles.css, devices.js.
Interfaces: grouped device controls with page-specific macros; sequence page banks/editing; own preset file import/export.
- [x] Expose every control in native/browser UI; preserve existing controls and instrument keyboard.
- [x] Browser-test synth selection, effects, sequencing, import/save and responsive layout; build native editors and render previews.

### Task 4: Verify and release
- [x] Run native and browser suites; fresh reviewer checks combined implementation and review focus.
- [x] Package versioned native binaries, update docs and hosted download, deploy Pages and verify public audio/assets.
