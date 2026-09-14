# Research notes

## Title info
- Title: NBA Live 10 (Europe, Asia)
- Title ID: 454108C1
- Guest DLL modules: none (`--scan-dll` found 0)
- Disc: 64 files, 6,267,027,825 bytes; `default.xex` is 20,299,776 bytes
- Code range: 822D0000-8329F8C8, image 82000000-83480000

## Codegen

### 2026-09-14: first pass
Validation failed on 4 plain `b` branches to addresses no function covered.
Seeded the targets:

| Target | Branch site |
| --- | --- |
| 0x822FBA68 | 0x82303F10 |
| 0x82938F98 | 0x8294406C |
| 0x82DF36F0 | 0x82DF3ABC |
| 0x82F6CA28 | 0x82F6CAFC |

That left one `Unresolved conditional branch to 0x82BAE36C from 0x82BAE354`.
The code at 0x82BAE338 is a frameless fragment that `sub_82BADB50` reaches with
`b 0x82bae338`; analysis attached it to that function but resolved the branch
before the fragment's labels existed. Seeding `0x82BAE338` makes the fragment
its own function and the parent's `b` a tail call, which is safe because the
fragment has no stack frame.

### 2026-09-14: runtime discovery
- First launch: `Call to invalid or unregistered function at 0x8231FD60`.
  An image dump (`LIVE_RECOMP_DUMP_IMAGE`) and
  `find_missing_functions.py` found 417 functions referenced from data;
  codegen flagged 8 split branches and 2 seeds were disabled.
- Second launch: `0x82A99490`, which follows `b 0x82a99464` and is only reached
  through a pointer computed at runtime. The code-gap scan added 433 seeds;
  84 split branches disabled 17 of them.
- Result: 836 seeds, 19 disabled, codegen reports no unresolved branches or
  calls. The game boots to menus and runs without fatal errors.

`scripts/analysis/stabilize_codegen.py --game LIVE10` now repeats the codegen,
seeding and pruning loop until codegen is clean.

### Open warnings
- **26x `Unexpected float16_4 pack instruction`** between 0x8237783C and
  0x8309C088, the same half-float vertex packing NBA LIVE 09 reports.
- **Function 0x83274BA0 is 1,357,334 bytes**, larger than
  `max_file_size_bytes`; its generated file is oversized but compiles.

## Kernel

- `XUsbcamGetState` and `XUsbcamSetConfig` (Xbox Live Vision camera) are
  imported, as in NBA LIVE 09. The stubs moved to `common/src/kernel` so every
  game shares them.
- Logged but harmless so far: `XNetLogonGetTitleID`,
  `XamBackgroundDownloadSetMode` and `IoDismountVolumeByFileHandle` stubs,
  missing `cache:\` and `UPDATE:\patch` devices, and `D:\data\...` loose files
  the game falls back to reading from the `.big` archives.

## Runtime issues

### Profile recreated on every launch (fixed)
Each launch wrote a new `SETTINGS <timestamp>` save. The game lists its saves
with `XamContentAggregateCreateEnumerator(xuid, ...)` and received
`00000000BABEBABE` instead of the profile XUID `B13EBABEBABEBABE`: ReXGlue read
every register argument as 32 bits. The SDK fork now reads 8-byte arguments in
full (`GetWideIntegerArgumentValue`), and the enumerator also lists the
signed-in profile when the xuid is zero. After the fix the enumerator returns
the existing saves and no new profile is created.

### Music sometimes stops (open)
Reported: menu music plays, then stops; picking a track afterwards plays
nothing. Two sessions showed no XMA or audio errors at the default log level,
and the audio worker kept submitting frames. Not reproduced under
`--log_level=debug` yet. When it happens, keep the game running and capture
per-thread CPU plus the debug log to tell a stalled XMA context apart from a
music streamer waiting on file I/O.
