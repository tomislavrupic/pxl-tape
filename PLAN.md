# Web companion implementation

User requested a web app and chose GitHub Pages with the other plugins. Reuse native C++ tape/DSP, preserving PXL 0.2 design and tape-first controls. Native source and packages remain unchanged.

1. Compile the current native TapeEngine into an instance-owned 48 kHz WASM bridge. Verify audible synth/drums, tape actions, instance isolation and native/WASM numerical parity.
2. Encode/decode the existing PXLTAPE1 + SHA256 + JUCE ValueTree format. Test against First-Tape.pxltape, browser-generated roundtrip, native acceptance and corruption rejection. Validate all bounds before modifying audio state.
3. Implement AudioWorklet communication, bounded status/peak updates, tape/sound/drum/sequence/mix pages, save/open, WAV import and export, keyboard, optional microphone and MIDI. File operations stop transport explicitly. No fallback engine.
4. Exercise actual browser audio, recording/undo, project roundtrip, keyboard, tabs and dark/light mobile/desktop layout. Keep local preview running for the user.
5. Publish verified static assets to a new PXL Tape GitHub Pages repository. Verify the deployment and public app; report exact browser/device limits.
