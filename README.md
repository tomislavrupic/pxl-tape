# PXL Tape Web

[Play PXL Tape](https://tomislavrupic.github.io/pxl-tape/) — a tape-first browser workstation in the PXL instrument family. Click **Enable Audio → Demo → Play**, or record the keys onto an armed track.

Four stereo tape tracks, overdubbing, selections, cut/copy/paste, reverse, undo, bounce, five sound engines (Analog, FM, Organ, Pluck and Sampler), eight drum voices, a 16-step melody/drum sequencer, and drive/delay/reverb. Dark and light interfaces adapt to desktop and phone widths. The native macOS AU, VST3 and standalone download is included in the app.

The browser uses the native C++ TapeEngine compiled to WebAssembly inside an AudioWorklet. No accounts or audio uploads are required. Projects are saved explicitly as local `.pxltape` files; keep them before closing the tab. The bounded binary codec reads and writes the native JUCE project format, including audio, parameters and sequences. Audio is resampled when opening at another device rate.

## Files and controls

- Tape capacity: 60 seconds per track. Sampler: 30 seconds. WAV/AIFF import: up to 64 MB; project open: up to 256 MB.
- Save/open, import and export stop transport. Press Play to resume.
- Mix WAV exports one loop forward at 1x, with track levels/pans, mute/solo and master effects. Stems ZIP contains four raw stereo WAVs. Exports are 24-bit PCM; effect tails beyond the loop are not appended.
- Drag waveform selections; drag dials vertically (Shift for fine control), double-click to reset, or enter exact values. A/W/S/E… play keys; Space toggles playback; R toggles record; Escape clears held notes and effects.
- Microphone and Web MIDI are optional and request browser permission only when clicked. Microphone monitoring starts off. Support depends on the browser; physical input devices have not been verified.
- Audio needs HTTPS/localhost and a user gesture. The 60-second engine allocates substantial memory; desktop browsers are the primary target. Browser DAW host transport sync is unavailable.

## Development

No npm dependencies. `npm test` runs engine, native-project codec and worklet tests, including native PCM parity, corruption rejection and worklet output at 44.1/48/96 kHz. The native-generated fixture and native PCM oracle are committed so CI can test the prebuilt engine without a C++ toolchain.

`python3 tools/build.py` rebuilds `dist/engine.wasm` with the official WASI SDK (`WASI_SDK_PATH`), regenerates the native oracle and records source/compiler hashes in `dist/engine-build.json`. The default SDK path points to the neighbouring PXL Dub toolchain. `npm run dev` serves `dist` on port 4175.

GitHub Actions tests the committed engine and deploys `dist` to GitHub Pages. Browser smoke checks covered audio initialization, demo playback, keys, drums, sequencer, mix page, recording, clear/undo, save and responsive themes. Native JUCE accepted a browser-written project. Native plugin installation, AU registration, hardware MIDI/microphone and cross-browser listening acceptance are separate checks.

This is an original PXL implementation inspired by a tape workstation workflow. It does not redistribute OP-1 firmware and is not affiliated with Teenage Engineering or Hundekraft.
