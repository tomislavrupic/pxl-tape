# Original OP-1 coverage audit

2026-10-25. User requests all original OP-1 algorithms, devices and effects in PXL style, in both the native plugin and browser workstation. This is a reference inventory and gap audit, not a claim of implemented parity.

Sources checked:
- https://teenage.engineering/guides/op-1/original/reference
- https://teenage.engineering/products/op-1/original/modules
- https://teenage.engineering/guides/op-1/original
- https://hundekraft.github.io/op1-workshop/

The generic /guides/op-1 URL now describes OP-1 Field; use /original routes for this project.

| Area | Original target | Current PXL Tape gap |
| --- | --- | --- |
| Synth engines | Cluster, Digital, String, Pulse, FM, Phase, Dr. Wave, D-Synth, Voltage, DNA, synth sampler | Analog, FM, Organ, Pluck and Sampler are original PXL implementations. Similar synthesis families do not establish original algorithm or sound parity. |
| Drum devices | D-Box and sliced drum sampler | Eight generated drum voices; no full original D-Box control model or drum sample slicing device. |
| Effects | Delay, Grid, Nitro, Phone, Punch, Spring, CWO | Generic drive, stereo delay and reverb; not original effect algorithms. |
| Sequencers | Pattern, Finger (synth/drum), Endless, Tombola, Sketch, Arpeggio | One 16-step melody/four-row drum grid. Other sequencers absent. |
| Sound architecture | Engine, envelope, effect and modulation per sound; preset slots and play modes | Shared global synth controls, attack/release and master effects. Needs richer per-slot architecture and versioned project migration. |
| Modulation | Inventory all original modulation modes and input mappings from detailed guide | No complete selectable OP-1 modulation system. |
| Tape | Four tracks, six minutes at normal speed, low-speed extension, editable regions, lift/split/drop, reverse recording, tape tricks | Four stereo 60-second tracks, selection editing, overdub, reverse playback, bounce and undo. Longer tape needs new storage design; record currently forces forward 1x. |
| Mixer/album | Track mixer, EQ, master effect/compressor, album recording | Track gain/pan/mute/solo and master effects; no full original EQ, compressor or album workflow. |
| Inputs/tools | Audio sampling, MIDI, radio and motion/accessory behavior | Browser audio import, optional mic/MIDI. Physical radio and hardware sensors need explicit software substitutes or hardware integration. |
| Workshop extensions | Separate additional engines/effects and tools | Not part of original baseline; audit separately after original scope is defined. |

## Fidelity choices requiring direction

Recommended: independently recreate every documented original device and interaction, preserving PXL visuals and shared native/WASM DSP. Track functional completeness and audible fidelity separately. Each named device needs an actual implementation, not a renamed generic oscillator or effect. Validate against hardware reference recordings across parameter settings when available.

Alternative: investigate running original firmware through emulation. No portable original DSP source or usable emulator has been established in this session. Workshop requires an unmodified R246 firmware input and generates hardware firmware; that does not establish desktop/browser execution. This path needs a bounded feasibility investigation before making implementation or exact-sound promises.

The existing live 0.2.0 app remains the working baseline. This audit does not change or publish product code.
