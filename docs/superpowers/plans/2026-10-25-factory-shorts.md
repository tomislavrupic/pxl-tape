# Short factory sounds

User asks to choose short Zogg sounds and include them in factory sounds. Choose8 contrasting sources under2seconds; preserve source files, tape-machine graphics and existing presets. Redistribution permission is not documented, so public metadata identifies curated local Zogg factory presets and never bundles raw commercial-library audio. Presets load through existing bridge with source fingerprint validation before mutation; individual local .pxlsound files and bank prepared for this user.

- [x] Inspect/preview/prepare8 sources via Zogg; store provenance and receipts locally.
- [x] Shared factory manifest generated into native header; factory menu groups6 synth presets +8 Zogg sample patches. Loads reset sound-specific parameters deterministically while preserving mixer/sequence/tape/transport; failures leave current sound unchanged.
- [x] Browser integrates factory loader using existing client and prepared bytes. Native uses cancellable background loader with SafePointer callback, source identity guard and atomic sound load. Scope includes original sample, full envelope/sample settings and controlled effect.
- [x] Focused factory tests, native/browser/live checks; fresh review; package0.6 and bridge, publish, verify downloads. Raw sample bank remains local.

Verification: 15 web tests, six bridge HTTP tests, six native tests passed (AU registration skipped); all eight actual sample patches render audible native output. Live native bridge flow passed. Browser factory recall preserves running tape. Fresh review recheck clear. Original source and reel hashes match. Public package excludes private sample bank; publication hashes and Pages deployment checked separately.
