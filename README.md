# PXL Tape Web

[Play PXL Tape](https://tomislavrupic.github.io/pxl-tape/) — a tape-first browser workstation in the PXL instrument family. Click **Enable Audio → Demo → Play**, or record the keys onto an armed track.

Four stereo tape tracks, overdubbing, selections, cut/copy/paste, reverse, undo, bounce, 13 sound engines, a drum synth and eight-slice sampler, seven selectable effect devices, six sequencers with128 steps/eight banks, modulation, ADSR, sampler loops/reverse, EQ and compression. Drive/delay/reverb remain available. Dark and light interfaces adapt to desktop and phone widths. The native macOS AU, VST3 and standalone download is included in the app.

The browser uses the native C++ TapeEngine compiled to WebAssembly inside an AudioWorklet. No accounts or audio uploads are required. Projects are saved explicitly as local `.pxltape` files; keep them before closing the tab. The bounded binary codec reads and writes the native JUCE project format, including audio, all device parameters and128-step sequences. Older projects receive defaults for new devices; new patch-bank projects need0.4. Audio is resampled when opening at another device rate.

## Files and controls

- Tape capacity: six minutes per track. Sampler: 30 seconds. WAV/AIFF import: up to256 MB; project open: up to1.5 GB.
- Save/open, import and export stop transport. Press Play to resume.
- Mix WAV exports one loop forward at1x, with track levels/pans, mute/solo, master effects, EQ and compression. Source effects are printed when recording and are not reapplied during export. Stems ZIP contains four raw stereo WAVs. Exports are 24-bit PCM; effect tails beyond the loop are not appended.
- Drag waveform selections; drag dials vertically (Shift for fine control), double-click to reset, or enter exact values. A/W/S/E… play keys; Space toggles playback; R toggles record; Escape clears held notes and effects.
- Microphone and Web MIDI are optional and request browser permission only when clicked. Microphone monitoring starts off. Support depends on the browser; physical input devices have not been verified.
- Audio needs HTTPS/localhost and a user gesture. Recording a full six-minute stereo track requires substantial memory; desktop browsers are the primary target. Browser DAW host transport sync is unavailable.

Album sides A/B capture live post-master output independently of tape transport for up to30 minutes. Takes stream to local IndexedDB and survive reload; export24-bit WAV to keep portable copies. Album audio is separate from tape projects. Recording stops if storage cannot keep up, retaining the stored prefix. Eight named sound slots include sample, effects and modulation and travel with saved projects; recall preserves tape transport.

SAVE SOUND / OPEN SOUND exchange portable `.pxlsound` files with your sample while preserving tape. Device controls are grouped under Sound, Drums, Sequence and Mix. [Device guide](docs/DEVICE-GUIDE.md).

## Zogg sample browser

Open SAMPLES or ZOGG SAMPLES from Tape/Sound. Download the Zogg bridge ZIP from that page, unzip, then open Start Zogg.command. Requires Python3.11+ and your existing configured Zogg MCP (Codex configuration, or ZOGG_MCP_COMMAND/ZOGG_MCP_ARGS). The launcher opens the connected localhost workstation; keep its Terminal window open. Native0.6 uses the same running bridge from its SAMPLES button. No changes to Zogg's app or original audio.

Search filename/metadata, choose a mounted library and duration limit, sort and page through results. Select a row, set start/length, Preview to audition and inspect its waveform, then Load Sampler or choose a tape track and Load Tape. Sampler loads up to30seconds; tape loads up to300seconds per excerpt at its current playhead, overwriting only that region with Undo available. Loading stops transport. Native preview is excluded from tape and album recording; browser preview uses a separate audio element.

The loopback bridge serves this same UI locally. Public Pages provides the connection/download page; it does not upload your catalogue or access drives directly. Pairing uses a per-run connection link and an HttpOnly SameSiteStrict cookie. Native discovery is a private mode600 Application Support/Pixel Records/PXL Tape/ZoggBridge.json. Only approved API calls and generated audio handles are served; no arbitrary source paths. A disconnected drive/source drift yields an error. Album browser storage is per-origin, so localhost and GitHub Pages album takes are separate; keep portable WAV/project copies when switching.

## Factory Shorts

Sound Presets includes8 Zogg factory patches using sources under2seconds: Pocket Kalimba, Glass Relay, Glass Shard, Spring Coil, Tin Satellite, Wire Pluck, Vapor Sub and Dust Click. They require the connected local workstation and matching Zogg sources. Source fingerprints are checked before preparing/loading; sound-specific settings reset consistently while tape, mixer and sequence are preserved. Raw sample audio is not included in public downloads. Save the loaded sound or project to keep a private portable copy.

Factory metadata in dist/factory-sounds.json is shared with native through tools/generate_factory.py and Source/FactoryManifest.h. The user's prepared .pxlsound files and eight-slot bank remain outside this repository.

## Development

No npm dependencies. `npm test` runs engine, native-project codec and worklet tests, including native PCM parity, corruption rejection and worklet output at 44.1/48/96 kHz. The native-generated fixture and native PCM oracle are committed so CI can test the prebuilt engine without a C++ toolchain.

`python3 tools/build.py` rebuilds `dist/engine.wasm` with the official WASI SDK (`WASI_SDK_PATH`), regenerates the native oracle and records source/WASM hashes in `dist/engine-build.json`. The default SDK path points to the neighbouring PXL Dub toolchain. `npm run dev` serves `dist` on port 4175.

GitHub Actions tests the committed engine and deploys `dist` to GitHub Pages. Browser smoke checks covered audio initialization, demo playback, keys, drums, sequencer, mix page, recording, clear/undo, save and responsive themes. Native JUCE accepted a browser-written project. Native plugin installation, AU registration, hardware MIDI/microphone and cross-browser listening acceptance are separate checks.

This is an original PXL implementation inspired by a tape workstation workflow. It does not redistribute OP-1 firmware and is not affiliated with Teenage Engineering or Hundekraft.

## Factory Drums

Eight more local Zogg one-shot sampler presets: Kick / Circuit, Kick / Backroom, Snare / Berg, Clap / Copper, Hat / Closed, Hat / Open Dust, Cymbal / Cinch, and Perc / Molecule. All are under one second; delay and reverb start off. Select from Sound Presets, play from the instrument keys at C4 for original pitch, or record onto tape. These presets use the sampler; they do not replace the synthesized drum pads. The same local Zogg bridge and source fingerprint checks apply. Raw library audio is excluded from public downloads.

## Eight-pad factory kit (0.7)

On DRUMS, choose LOAD ZOGG KIT. The eight curated drum recordings load together with fingerprint validation before replacing the kit. Pad order: Circuit Kick, Berg Snare, Closed Hat, Copper Clap, Backroom Kick, Molecule Percussion, Open Hat, Cinch Cymbal. MIDI channel10 notes36–43 use the same order. Closed Hat chokes Open Hat. All eight pads have sequencer lanes across the existing eight banks. Enable DRUMS on SEQUENCE and Play; Record prints to the armed tape track.

The kit has independent stereo sample memory, so changing or recalling a melodic sound leaves the kit and drum settings intact. Loading a kit preserves melody timing/held notes, tape, transport and patterns. Existing D-BOX synth and melodic sample slices remain selectable. Save the project to embed the kit for offline recall. New projects use format3 and need PXL Tape0.7 or newer; formats1/2 still load with an empty kit and four extra empty drum lanes. Public downloads contain metadata only; initial factory loading requires the local Zogg bridge.
