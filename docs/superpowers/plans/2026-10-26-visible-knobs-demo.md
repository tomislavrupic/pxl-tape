# Visible controls and startup demo

Goal: expose device controls as tactile rotary controls without collapsible panels, and open the browser with a more musical original demo ready to play.

Use the existing PXL dial component for continuous parameters and detented selectors with readable value labels. Keep all envelope/sample/effect/modulation sections visible, responsive on phones and in light mode. Native already exposes rotary controls; use one shared original demo generator for native Demo and WebAssembly. Browser shows a generated waveform preview before audio is enabled, then materializes the same demo on the first audio gesture. New and project-open suppress startup demo so personal tapes are never replaced. Preserve Safari playback session handling and reel graphics.

- [x] Refactor existing knob builder for use in open device sections, with integer choices, value synchronization and touch/keyboard/fine-adjust controls.
- [x] Shared deterministic 112 BPM, 16-bar original four-track arrangement: drums, syncopated bass, wide chords, bell lead, variations and a breakdown. Test non-silence/stereo/structure/loop bounds and non-destructive loading.
- [x] Fresh browser sessions show demo waveform/name and load it on audio activation; New and Open preserve explicit intent. Test lifecycle and check desktop/mobile layouts.
- [x] Build/test web/native, review, update downloads, deploy and verify live assets.

- [x] Standalone Mac app opens the shared demo on its first audio preparation; restored state takes priority. Dedicated Apple Silicon download, install notes, ad-hoc signature and source included.
