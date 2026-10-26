# PXL Tape landing page and AU release

Goal: Public PXL landing page on existing GitHub Pages, direct playable workstation and AU download.

Use index.html for the landing page and play.html for the unchanged workstation. Keep all workstation assets at their existing paths. The local Zogg helper serves play.html at / and marks it as the connected local workstation, preserving existing pairing links. Public landing page uses PXL typography/panels/colours and the existing reel renderer, a responsive workstation preview and plain feature/install descriptions. No external fonts, trackers or dependencies. Browser play is functional without Zogg; curated commercial sources remain local.

Create an AU-only Apple Silicon macOS12+ archive from the verified 0.7.0 component, include install instructions/source, verify signature/ZIP and AU instantiate/MIDI render outside sandbox. Explain development signing/notarization accurately. Retain existing full AU/VST3/Standalone bundle.

- [x] Workstation route and helper regression test; preserve reel graphics and existing project/audio behavior.
- [x] Landing page, responsive design, working Play/download/install links; desktop/mobile browser checks.
- [x] Dedicated AU archive and release receipt; existing audio/bridge checks, publish and verify hosted hashes.

Verification: desktop and mobile layout (no horizontal overflow), Play link, Enable Audio/Demo/Play/Stop; 21 web tests and seven bridge HTTP tests. Dedicated AU archive CRC/signature and actual AU instantiate/MIDI render passed. All eight native tests passed outside sandbox. Image generation prompt in docs/artwork/pxl-tape-banner.md. Physical iPhone/iPad speaker confirmation remains user-side.
