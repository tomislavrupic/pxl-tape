# Factory drum kit pads implementation plan

Goal: Load the eight curated Zogg drum hits into an independent kit, play all eight pads/MIDI channel10 notes36-43, and sequence every pad. Save the kit inside projects for offline recall. Existing synth drums and melodic sampler slicing remain available.

Architecture: Add bounded stereo kit memory packed into eight equal padded slices, independent of the melodic sampler. Extend drumMode with Kit and sequence storage/display to eight drum rows. Add optional DrumKit audio/name to native/web project codec; old projects default to empty kit and four extra empty lanes. Sound-only recalls preserve drum settings and kit. Fetch all eight fingerprint-checked sources before atomically installing a kit; failed or superseded requests preserve current state. Keep tape reels unchanged and raw commercial samples local.

- [x] Core: independent stereo kit rendering, hat choke, all eight sequence lanes; tests for isolation, pad identity, velocity, tail boundaries and sequencing pad8.
- [x] Persistence: native/web project roundtrip, resampling and legacy defaults; sound-only recall preserves kit; reject malformed kit payload before mutation.
- [x] Controls: Load Zogg Kit button on Drums, kit pad/sequence labels, existing synth/slice modes retained; cancellable atomic loader in web/native.
- [x] Verify: engine/WASM/worklet/native regressions, actual eight-source kit rendering and save/reload, browser pad/sequence flow, fresh code review, native packaging and Pages publication with hosted hashes.

Kit pad order preserves existing rhythm positions: Circuit kick, Berg snare, closed hat, Copper clap, Backroom kick, Molecule percussion, open hat, Cinch cymbal. Kit install preserves melody, tape, mixer, patterns and transport. No source edits or raw-sample public distribution.

Verification receipt: native Preview-0.7/VERIFICATION.md. Kit worklet/native tests pin melody phase preservation during installation. Fresh review recheck clear.
