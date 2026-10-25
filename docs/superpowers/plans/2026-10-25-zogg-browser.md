# Zogg Sample Browser Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans inline; fresh whole-change review at end.

**Goal:** Search, audition and load Zogg sounds directly into PXL sampler/tape while preserving reel graphics.
**Architecture:** Python standard-library loopback bridge to Zogg MCP; local same-origin web browser; native JUCE asynchronous browser uses same API/discovery.
**Tech Stack:** Existing JS/C++/JUCE and Python3.11+.
**Spec:** docs/superpowers/specs/2026-10-25-zogg-browser-design.md

## Global Constraints
- No source audio modifications, private samples or absolute source paths in public assets.
- Keep graphics.js reels and native drawReels unchanged.
-30sec sampler,300sec prepared excerpt,360sec tape capacity; validated trim and bounded pagination.
- IO and preview decoding outside audio rendering; actual load follows existing undo/transport rules.

## Review Focus
- Offline bridge, missing disk/source drift: actionable errors; existing tape preserved.
- Stale selections/rapid searches and async close: no wrong-asset or wrong-track loading.
- Preview cleanup and import failures: no lingering audio or disabled controls.
- Loopback boundary: authentication, Origin/Host, no arbitrary files/tool calls.
- Tape replacement: destination explicit, undo available; reel art kept intact.

## Tasks
- [x] Bridge: failing fake-MCP HTTP tests for token/origin/host, bounds, file handles and MCP errors; implement; live status/search/prepare checks.
- [x] Web: standalone sample-browser module with search/filter/page/trim/waveform and lifecycle; tests async selection; integrate reusable import path and local connection link.
- [x] Native: browser dialog and worker to bridge, list/search/preview/trim and destination load through existing processor imports; safe close; build/tests.
- [x] Verify: real browser Zogg search/preview/sampler/tape load, unchanged originals/reel hashes; native bridge/render tests; fresh review; package0.5 and publish Pages with local bridge launcher.

Ruling: use the established tape-import operation (write at current playhead with Undo), rather than clearing the whole track. This preserves existing project behavior and avoids unnecessary data loss. Labels explicitly state the affected region. ZoggClient uses the macOS Application Support session path explicitly because JUCE userApplicationDataDirectory resolves to ~/Library.

Final: review clear after fixing stale result selection, pending native preview cancellation, interrupted JSON/WAV handling, authenticated cookie bootstrap and stale bearer override.13 browser tests,4 HTTP boundary tests and6 native checks pass; AU runtime registration skips. Real browser and native Zogg search/prepare/sampler/tape load verified. Both reel source hashes unchanged.0.5 native package includes bridge+launcher; private library/cache/session excluded.
