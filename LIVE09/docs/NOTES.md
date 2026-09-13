# Research notes

## Title info
- Title: NBA Live 09 (Europe)
- Title ID: 4541087A
- Media ID: 6653832E
- Version: 0.0.0.2 (xex filetime 2008-08-18 23:11:06 UTC)
- Guest DLL modules: none (`--scan-dll` found 0)
- Disc: 56 files, 6,545,424,081 bytes; `default.xex` is 18,300,928 bytes

## Key addresses (default.xex)
| Address | Name | Notes |
| --- | --- | --- |
| | setjmp | |
| | longjmp | |
| | main / game loop | |

## Codegen issues
<!-- address, symptom, fix applied in config/ -->

### 2026-09-13 — 8 UnresolvedCall errors (fixed)
Validation failed on plain `b` branches (tail calls) to addresses no function
covered. Fixed by seeding them in `config/functions.toml` with no size, so
Discover walks each one:

| Target | Branch sites |
| --- | --- |
| 0x824BCCB0 | 0x824BC634 |
| 0x82CDD8C8 | 0x82586D68, 0x8256A68C |
| 0x82CDF0A0 | 0x8256B270 |
| 0x82CF5658 | 0x82CF69F4, 0x82CF6A74 |
| 0x82CF8EF0 | 0x82CD16E8 |

After the fix codegen succeeds in ~40s: 497 files, ~257 MB in `generated/default/`.

### Open warnings from codegen
- **26× `Unexpected float16_4 pack instruction`** — VMX128 `vpkd3d` packing to
  half-float that the translator doesn't handle. Sites: 0x8239B438–0x8239B480 (6)
  and 0x82EB1BAC–0x82EB2688 (20). Likely vertex/geometry packing; suspect these
  first if models or animation look corrupted at runtime.
- **Function 0x8307AF08 is 1,358,335 bytes** — larger than `max_file_size_bytes`
  (1 MB), so its generated file is oversized. Probably a huge generated table or
  switch-heavy function; may be slow to compile. Check whether analysis merged
  several functions into one.

## Build issues

### 2026-09-13 — unresolved kernel imports at link (stubbed)
`XUsbcamGetState` and `XUsbcamSetConfig` (Xbox Live Vision camera) are in the
SDK export table but not implemented. Stubbed in `src/hooks/hooks.cpp` to return
`X_ERROR_DEVICE_NOT_CONNECTED` (logs a warning each call). Game imports no other
XUsbcam functions.

Result: `out/build/win-amd64-release/nba_live_09.exe` (~85 MB) + `rexruntime.dll`.
The exe defaults `--game_data_root` to `LIVE09/assets` (set in CMakeLists.txt).

## Runtime issues
<!-- crash / hang / graphics bug, cause, fix -->

### 2026-09-13 — black window on first boot (fixed, retest)
Game booted (window, controller detected, file I/O working) but never drew:
`VdInitializeRingBuffer: no GPU emulation loaded (gpu_plugin not set)`.
Fix: `rexglue_setup_target(nba_live_09 GPU_PLUGINS xenos)` stages
`rexgpu-xenos.dll` next to the exe, and `OnPreSetup` defaults `gpu_plugin` to
`xenos`.

Harmless in that log: `cache:\` NtCreateFile failures (SDK intentionally leaves
the cache device unmapped), `IoDismountVolumeByFileHandle` stub,
missing `gamecontrollerdb.txt`.

### 2026-09-13 — crash after GPU init (fixed, retest)
With the plugin loaded the game ran on the RTX 5080, created render targets and
pipelines, then died with
`[FATAL] Call to invalid or unregistered function at guest address 0x828EF120`.
That address is a 0x20-byte function sitting between sub_828EF110 (a 0x10-byte
vtable thunk) and sub_828EF140; only reached through a pointer, so analysis
skipped it. Seeded in `config/functions.toml`. Expect more of these one at a
time — same fix each.

### 2026-09-13 — second missed function 0x82515078; bulk scan (retest)
Next run died the same way at 0x82515078. The SDK's VTableScanner only finds
vtables with MSVC RTTI, which EA code mostly lacks, so these would keep coming.
Bulk fix:
1. `NBA_LIVE_09_DUMP_IMAGE=out\image_dump.bin` makes the exe dump the loaded
   image (0x82000000–0x833D0000) and exit (`OnPostLoadXexImage` in the app header).
2. `python scripts/find_missing_functions.py --write` scans data sections for
   pointers into code that aren't generated functions/labels and follow a
   blr/bctr/b/padding. 47,166 code pointers → 167 candidates (included 0x82515078).
3. Codegen then flagged 3 tail-call stubs (0x824BC6B0/B8/C0) reached from the new
   functions; seeded those too. 176 seeds total, codegen and build clean, no
   split/outranked warnings.

Re-run steps 1–2 after big config changes; the scan only sees pointers stored in
data, not ones computed at runtime.

### 2026-09-13 — third missed function 0x82B80FF8; gap scan (retest)
0x82B80FF8 had no pointer in data (computed at runtime). It sits right after a
`bctr`: a 5-instruction thunk (mr r6,r5; mr r5,r4; li r4,-1; b ...).
`find_missing_functions.py --gaps` seeds code that follows blr/bctr/b and is not a
function, a branch label, or a conditional-branch target.

First attempt without the bc-target filter seeded loop bodies (0x82DF3F14,
0x82DF3F40: `b cond` then body reached by backward `bne`) and broke compile with
`use of undeclared label 'loc_82DF3F40'`. Removed those entries, regenerated, and
added the filter. **Always regenerate before scanning** — the script reads known
functions/labels from generated/, so stale output hides candidates.

Result: 310 gap seeds (250 after blr/bctr, 60 after b), 486 seeds total. Codegen
and build clean.

### 2026-09-13 — `Unresolved branch from 0x82D6E2F4 to 0x82D6E344` (fixed, retest)
Next run died in <1s. Some seeds pointed *inside* real functions (e.g. 0x82D6DEA0):
codegen started a new function there, ended it at the first `b __restgprlr_*`,
and cut the rest off. Codegen had warned — the original clean run had **0**
`Unresolved conditional branch` / `Jump target ... unresolved` lines; after the
pointer scan there were 17, after the gap scan 320 + 16.

`python scripts/prune_bad_seeds.py <codegen log>` comments out seeds that sit
between a warned branch and its target, or start the function containing it.
One round disabled 58 seeds → warnings back to 0. 428 active seeds; the three
runtime crash addresses (0x828EF120, 0x82515078, 0x82B80FF8) are still seeded.

**Rule: after any seeding, codegen must still show 0 of those warnings.**

### 2026-09-13 — missed function 0x824BC6C8 (fixed, retest)
Got further: audio system started (`RWAudioCore Dac` thread, NVIDIA HD Audio
endpoint), then `Call to invalid or unregistered function at 0x824BC6C8`.
It's a 4-instruction function (lbz r11,172(r3); li r3,0; stb r11,0(r4); blr)
sitting after a table of 8-byte adjustor thunks (`addi r3,r3,-16; b target`,
0x824BC5D0–0x824BC6C4). Thunk 0x824BC620 tail-calls it, so codegen inlined it as a
`loc_` label of that thunk — which also hid it from the gap scan. Seeded by hand;
codegen still 0 warnings.

### 2026-09-13 — BOOTS: intro plays, reaches "Press Start" (green artifacts)
Ran ~2m15s without crashing. Log (4,019 lines) is dominated by 3,746×
`Texture fetch constant (...) has "invalid" type!` — with
`gpu_allow_invalid_fetch_constants=false` (default) the texture cache drops the
binding, so those draws sample no texture. Prime suspect for the green artifacts.
Next test: `--gpu_allow_invalid_fetch_constants=true`. Second suspect: the 26
unhandled `float16_4` vpack instructions.

Other log noise, all expected: missing optional files (`movies\*.sub` subtitles,
`UPDATE:\patch\patch.big`, `fx-final\sideline_body.fx*`, `gobo.tex`), cache: device,
XUsbcamGetState stub called once.

### 2026-09-13 — green artifacts are video-only; SDK vpkuwus aliasing bug (retest)
`--gpu_allow_invalid_fetch_constants=true` cleared all 3,746 warnings but not the
green, so that wasn't it. Green appears only during videos (menus clean, sound ok).
Videos are EA VP6 (`movies\*.vp6` inside data4.big, `MVhd`/`vp60` header),
decoded on the CPU. Decoder strings at 0x82141014 ("VP6 Decode Job"); hot code
0x82B33FB8 (transform) and 0x82B41368 / 0x82B46658 / 0x82B46C20 (colour convert).

Bug found in rexglue-sdk codegen (`src/codegen/builders/vector.cpp`):
`build_vpkuwus` and `build_vpkuhus` wrote vD element-by-element while still
reading vA/vB, so `vpkuwus128 v63,v63,v61` read its own half-written output. All
9 vpkuwus uses in the game alias vD, all in the VP6 decoder/colour converters.
**Local SDK patch:** copy both sources to `pkA`/`pkB` before writing. SDK rebuilt
and reinstalled to C:\ReXGlue, codegen forced with `--ignore-stamp` (SDK changes
don't trigger regen), game rebuilt. Worth upstreaming.

Checked and fine (load-then-store): vcfux/vcuxwfp, vctuxs/vcfpuxws, vcfsx, vctsxs, vrfiz.

### 2026-09-13 — videos fixed; settings file + reference notes
Retest after the vpkuwus fix: **no green in videos**. Game boots, intro + sound ok.

Reference: `Downloads/skate3recomp` (EA title, playable). No LICENSE file, so use
it for approach only, don't copy code. It uses a fork of the SDK
(`mchughalex/rexglue-skate3`), so some of its cvars don't exist here.

Added `settings/nba_live_09.toml` (copied beside the exe on build; SDK loads
`<exe dir>/nba_live_09.toml`). Only cvars verified in our SDK:
- Controller: `input_backend = "sdl"` (default) maps DS4/DualSense/Switch/generic
  to the Xbox layout with rumble + hot-plug (src/input/sdl/sdl_input_driver.cpp);
  `guide_button`, `hid_mappings_file`, `mnk_mode`.
- Frame pacing: SDK "GPU VSync" thread fires guest vblank at
  `video_mode_refresh_rate` (clamped 24–240), or every 1 ms with `vsync=false`.
  **Untested for NBA Live:** if game logic is frame-locked, raising it speeds
  gameplay up and a proper unlock needs a game patch (find the timestep).
- Display/quality: `fullscreen`, `window_width/height`, `present_letterbox`,
  `d3d12_allow_variable_refresh_rate_and_tearing`, `resolution_scale` (1–7),
  `swap_post_effect` (none/fxaa/fxaa_extreme).
- Fork-only in skate3 (not available): `vblank_host_clock_pacing`,
  `d3d12_present_frame_limiter*`, `d3d12_max_frame_latency`.

Widescreen: the game already renders 16:9 (1280x720 video mode). Ultrawide
(21:9+) needs a game-side aspect/FOV patch; skate3 does it only in its native
renderer (skate3_ultrawide_guest.h, skate3_fov.cpp) — investigate later.

### 2026-09-13 — sticks don't move player; hang returning to shootaround
Bluetooth Xbox pad: menus work (Start, D-pad), but in the practice shootaround
the sticks don't move the player. Log showed **two** pads:
`XInput Controller #1` VID 0x0B05 PID 0x1C92 present at startup (ASUS, likely a
virtual pad from Armoury Crate) and `#2` PID 0x1C93 ~10s later. SDK default
`SlotAssignment` makes the real pad player 2, which menus accept but gameplay
ignores. Fix: `nba_live_09_shared_controllers` cvar (default true, defined in
src/patches/patches.cpp); `OnPreSetup` wraps `CreateDefaultInputSystem` and sets
`SharedAssignment` (all pads → user 0). **Retest.**

Hang: menu → B back to shootaround gives black screen, audio continues. No FATAL;
log ends with optional `fxsimple\hair\*.rx2` loose-file probes (normal), then
silence for 70s until the window was closed → a guest/GPU thread stall, not a
missed function. Needs a `--log_level=debug` repro.

Update: controls now work in the shootaround (shared assignment fix confirmed).
Real repro: game boots into the shootaround fine; Start → pause menu; B → black
screen with arena audio; Start again shows the menu. Only the first arena view
works. Debug log (nba_live_09_017.log): no warnings/GPU errors at all, no render
targets created/lost on return. Pause at 19:00:07 = `XGIUserSetContextEx` ×4 +
new `VideoDecodeThread`; more VideoDecodeThreads at 19:00:20 / 19:00:31.
Theory: pause menu plays a VP6 background video; on close the game waits for the
video system to report stopped before handing the screen back to 3D, and that
never completes, so 3D is never drawn again while UI still is.

Diagnostic SDK change: `kDrawCalls` perf counter was declared but never
incremented; added `IncrementCounter` in `D3D12CommandProcessor::IssueDraw`, so the
**F3 debug overlay** shows real draws/frame. (`perf_log_csv` cvar exists but
`SetCsvLogPath` is never called, so no CSV.) Note: SDK DLLs are only copied next
to the exe when the exe relinks — copy `C:\ReXGlue\bin\*.dll` by hand after
SDK-only changes.

### 2026-09-13 — crash when starting a match: host heap corruption (testing)
nba_live_09_018.log just stops mid-load (hair .rx2 probes), no FATAL/shutdown.
Windows Event Log: `0xc0000374 STATUS_HEAP_CORRUPTION` in ntdll. Full dump in
`%LOCALAPPDATA%\CrashDumps\nba_live_09.exe.<pid>.dmp` (pid 32836 for this one).
No cdb installed and LLVM's lldb needs python311.dll, so the dump was read with the
pure-Python `minidump` + `pefile` packages (scratch venv), mapping stack values to
module+offset / nearest export (no PDBs: Release build).

Faulting thread = guest render thread: nba_live_09.exe → rexgpu-xenos.dll
(+0x61260, +0x1D48E0, +0x60EC0, +0x60DB1) → D3D12Core / D3DSCache / dxilconv →
nvwgf2umx (NVIDIA UMD). i.e. heap corruption *detected* while creating a new
pipeline (match start = many new shaders). ~12 other threads idle in the same
rexgpu-xenos worker routine (+0x24762C / +0xF4B86) = "D3D12 Pipelines" creation
threads (`d3d12_pipeline_creation_threads = -1` → 3/4 of logical CPUs).
First test: `--d3d12_pipeline_creation_threads=0 --async_shader_compilation=false`
to see if it's a race in async pipeline creation. If it still crashes, next step is
a RelWithDebInfo SDK build (PDBs) + page heap to catch the actual bad write.

Result: with `d3d12_pipeline_creation_threads=0` + `async_shader_compilation=false`
a **Play Now match starts and plays** → race in the async pipeline creation path.
Both are now defaults in `settings/nba_live_09.toml`. Root cause not fixed yet.

F3 overlay showed no counters: SDK CMakeLists.txt only defines
`REXGLUE_ENABLE_PERF_COUNTERS` for non-Release configs
(`$<$<NOT:$<CONFIG:Release>>:...>`), and C:\ReXGlue is a Release install, so the
overlay's counter section (and the new IssueDraw increment) is compiled out.
Needs a RelWithDebInfo SDK install — which would also give PDBs for crash dumps.

Pause-menu black screen is **inconsistent**: after playing a match and quitting
back, the shootaround pause/resume works. So it's state/timing dependent (first
arena session only so far), not a permanent render break.

### 2026-09-13 — RelWithDebInfo build for diagnostics
SDK built + installed in RelWithDebInfo next to Release in C:\ReXGlue (files use
the `rd` postfix: rexruntimerd.dll, rexgpu-xenosrd.dll, TracyClientrd.dll;
rexglueTargets-relwithdebinfo.cmake). The GPU plugin loader appends the same
postfix, so configs can't mix. Perf counters (F3 overlay) are compiled in.
Game: `scripts\build.ps1 -Preset win-amd64-relwithdebinfo` →
`out\build\win-amd64-relwithdebinfo\nba_live_09.exe` + nba_live_09.pdb (~94 MB).
GPU preference (High performance) set for that exe path too.

### 2026-09-13 — pause-menu black screen: fixed by ROV render-target path
F3 overlay (RelWithDebInfo): main menu Draw 2478 / Verts 1.40M; black screen
Draw 1496 / Verts 1.43M → the game still renders the arena, output is black.
Not fixed by `d3d12_readback_resolve` (→ readback_resolve "fast") +
`d3d12_readback_memexport`. **Fixed by `--render_target_path_d3d12=rov`**.
Isolation: user had also created a profile/favourite team that run; Test A
(profile present, default rtv path) was black again → ROV is the fix.
Now default in settings/nba_live_09.toml (both build folders updated).
Likely root cause in the RTV (host render target) path for this game's float/HDR
render targets (NaN/Inf persisting in a feedback buffer until a match load
resets it) — not investigated further; ROV costs GPU time.

Xenia Canary report for this title (xenia-canary/game-compatibility#286):
state-gameplay, `vsync-off-speedup`, cutscene speed tied to FPS → framerate
unlock needs a game timing patch, not just video_mode_refresh_rate.

### 2026-09-13 — exe renamed to "NBA LIVE 09.exe" + icon
`set_target_properties(nba_live_09 PROPERTIES OUTPUT_NAME "NBA LIVE 09")`; the
CMake target and app name stay `nba_live_09`, so the settings file is still
`nba_live_09.toml` and logs are unchanged. Old nba_live_09.exe/.pdb removed from
both build dirs; GPU preference registry entries moved to the new paths.

Icon comes from the user's own default.xex (no artwork in the repo):
`rexglue init --project-name nba_live_09 --xex-path assets\default.xex achievements assets\default.xex metadata`
(the parent `init` options are required even for the subcommand) → 23
achievements + metadata/icons/title.png (64x64) → upscaled to 256 and saved as
metadata/nba_live_09.ico (16–256) with Pillow. `src/nba_live_09.rc` embeds it as
icon ID 1 plus VERSIONINFO; CMake adds the .rc only if the .ico exists. All
extracted artwork is git-ignored.

Window title: the SDK sets `GetName() + " " + REXGLUE_BUILD_TITLE` right after
creating the window (rex_app.cpp SetupPresentation). The app overrides
`OnCreateDialogs` (runs later, UI thread) to `window()->SetTitle("NBA LIVE 09")`.
Not done by renaming the app, since GetName() also names the settings file,
user/save folder (Documents\nba_live_09) and logs.

### 2026-09-13 — ISO installer, settings menu, black UI
Modelled on skate3recomp's flow (read for approach only — no license, no code
copied; its `InstallWizardDialog`/`SimpleSettingsDialog` come from its SDK fork
and don't exist in ours). Built on our SDK instead:
- `OnFinalizePaths` (async): game folder = `--game_data_root`, else dev
  `assets/` if it has default.xex, else `<exe dir>\game`. Missing default.xex →
  `src/installer/iso_installer.*` setup window: Select ISO (Win32
  GetOpenFileNameW) → worker thread walks the SDK's `DiscImageDevice` and copies
  every entry (4 MB chunks, rejects unsafe names) with a progress bar → startup
  resumes via `CallInUIThreadDeferred(resume)`. Quit button exits.
  Unattended: `NBA_LIVE_09_INSTALL_ISO=<iso>`.
  Verified: unattended install to an empty folder = 56 files / 6,545,424,081
  bytes in 18 s, all SHA-256 identical to the extract-xiso copy.
- Portable mode: `portable.txt` next to the exe → user data in `<exe dir>\user`.
- Settings menu (Esc, `src/ui/settings_dialog.*`): Video (fullscreen live, vsync,
  render scale, FXAA, render-target path), Controls (backend, shared
  controllers, guide button), Game Files (paths, portable). Saved to
  `<user data>\settings.toml` (`src/ui/user_settings.*`), loaded in
  `OnPostInitLogging` so it overrides nba_live_09.toml; command line still wins.
  Restart-only changes are flagged.
- Theme (`src/ui/theme.*` via `OnConfigureStyle`): opaque black windows, grey
  controls, white text/borders/tab overline/checkmarks/progress; ImGui 1.92.5
  colour names (TabSelected, NavCursor). The "blur" was ImGui's translucent
  default windows, not a shader.
Not done vs skate3: RB+Start controller chord for settings, profiles UI, DLC,
title update wizard, Linux/macOS file pickers.

### 2026-09-13 — controller system menu (Resume / Settings / Exit Game)
`src/ui/system_menu.*`. Opens on Guide/PS button (edge), holding Back+Start 0.5 s,
or Esc. Esc now opens this menu; Settings is reached from it.
- `ControllerMenuWatcher`: background thread polls user 0 every 16 ms; opens the
  menu via `CallInUIThreadDeferred`. No permanent ImGui dialog, because the
  drawer attaches input + repaints continuously while any dialog exists.
- Input gate: replaces the SDK's `InputSystem::SetActiveCallback` in
  `OnPostSetup` (SDK sets its own in ConstructRuntime just before). Returns true
  only for the menu's own reads (thread_local flag) or when no menu is visible
  and all buttons have been released since one closed — so the game never sees
  the A/B that operated the menu. When inactive, the SDL driver zeroes the pad
  for every caller, hence the thread-local bypass.
- `FeedControllerToImGui`: D-pad/left stick/A/B/LB/RB → ImGui gamepad nav
  (the SDK doesn't feed gamepads to ImGui). Also used by the settings menu.
- Exit Game (with confirm) → `window()->RequestClose()`, same path as the X.
- `guide_button = true` default (Xbox 360 games never read Guide). Windows Game
  Bar may take the Xbox button; Back+Start is the fallback.
Known: holding Back+Start lets the game see Start during the hold.

### GDK / Xbox — PC GDK local package (in progress)
Project Helix = next-gen Xbox announced GDC 2026; dev kits 2027; unified GDK
across PC / Series X|S / Helix. First target: PC GDK sideload for the Xbox PC app.
- Installed Microsoft GDK April 2026 Update 4 (winget `Microsoft.Gaming.GDK`,
  machine scope) → `C:\Program Files (x86)\Microsoft GDK\bin` (makepkg, wdapp).
  Gaming Services 38.117 and Xbox app (Microsoft.GamingApp) already present.
- `gdk/MicrosoftGame.config`: only `<Identity>` is required by the schema
  (GameConfigSchema.xsd); no TitleId/MSAAppId/StoreId → local placeholder identity
  `NBALive09.Recomp` / `CN=NBALive09Recomp`, no Xbox network features. Alias must
  end in `.exe` (ST_ExecutableNoPath). KnownDependency VC14, x64.
- Shell images from the user's title icon → `metadata/gdk/` (StoreLogo 100,
  Logo 150, SmallLogo 44, SplashScreen 1920x1080), git-ignored.
- `scripts/package_gdk.ps1`: stages `out/gdk/layout` (exe, DLLs, toml, config,
  images); `-Register` / `-Unregister` (loose), `-Pack` (makepkg genmap + pack
  /lt /nogameos /pc → out/gdk/package), `-Install` (wdapp install). Register and
  Install refuse to run without Developer Mode.
- Result: `-Pack` succeeded → NBALive09.Recomp_0.1.0.0_x64__vtrtzaze888yy.msixvc
  (~101 MB), passed makepkg validation / non-retail dependency check.
- Blocked on: Developer Mode is off (AppModelUnlock
  AllowDevelopmentWithoutDevLicense not 1). User to enable, then `-Install`.
- Unverified: whether a sideloaded test package appears in the Xbox app library.

Update (same day):
- Shortcut changed per user: the Xbox button belongs to Game Bar / Xbox mode, so
  the system menu now opens Series X|S-style by pressing View (Back) + Menu
  (Start) together (second press within 250 ms, no hold). Guide only opens it
  when `guide_button = true`, now off by default for every build.
- Metadata from the Xbox 360 release (sources: Wikipedia "NBA Live 09",
  retailer listings): developer EA Canada, publisher Electronic Arts / EA SPORTS,
  released 2008-10-07 NA, 2008-10-09 AU, 2008-10-10 EU; sports/basketball;
  single-player + multiplayer; cover athlete Tony Parker; ESRB Everyone.
  MicrosoftGame.config: Identity `ElectronicArts.NBALIVE09` /
  `CN=Electronic Arts Inc.` v1.0.0.0, PublisherDisplayName Electronic Arts,
  languages en-US/en-GB/fr-FR/es-ES (EU disc audio), own-words description.
  Exe VERSIONINFO: CompanyName Electronic Arts, LegalCopyright (c) 2008
  Electronic Arts Inc., version 1.0.0.0. Local only, not for distribution.
- Repacked: `ElectronicArts.NBALIVE09_1.0.0.0_x64__zwks512sysnyr.msixvc`.

### 2026-09-13 — packaged install crashed after splash (fixed, retest)
Installed to `C:\Program Files\WindowsApps\ElectronicArts.NBALIVE09_...` (read-only,
not a flat XboxGames folder). Event log: 0xc0000005 in rexruntime.dll+0x115602 =
`std::vector<rex::input::DeviceInfo>::_Emplace_reallocate` → `InputSystem::
RefreshDevices`. **InputSystem had no locking**: every GetState rebuilds
`devices_`, and the guest's polling plus the system-menu watcher thread call it
concurrently. Dev runs got lucky on timing; the slower packaged start didn't.
SDK patch: `std::mutex lock_` in InputSystem, locked in SetDeviceAssignment,
GetCapabilities, GetState, SetState, GetKeystroke (drivers keep their own locks).
Rebuilt/installed both SDK configs, both game configs, repacked, reinstalled.

Also for read-only installs (`OnConfigurePaths` write probe of the exe dir):
`log_file` → `<user data>\logs\nba_live_09.log` (SDK otherwise does an unguarded
create_directories(<exe dir>\logs)), and the default game folder →
`<user data>\game` so the first-run ISO installer can extract.
`package_gdk.ps1 -Install` now uninstalls an existing ElectronicArts.NBALIVE09 first.

### 2026-09-13 — Xbox app shows "Launching" forever; blurry art (retest)
Game ran fine from the Xbox app, but the button stayed "Launching". The app tracks
running titles via the Gaming Runtime; the exe never called
`XGameRuntimeInitialize`. Added `src/platform/gdk_runtime.*`: called from
`OnPostInitLogging`, uninitialized in `OnShutdown`. CMake detects the PC GDK
(`NBA_LIVE_09_GDK_ROOT`, default `...\Microsoft GDK\260404\windows`), defines
`NBA_LIVE_09_HAS_GDK`, links `xgameruntime.lib` + `delayimp` with
`/DELAYLOAD:xgameruntime.dll`; code probes `LoadLibraryW` first so PCs without
Gaming Services still start. Log line: "Gaming Runtime initialized".
Unverified that this flips the Xbox app to "Running".

Art: only source is the 64x64 XDBF title icon. Upscaled 16x with Real-ESRGAN
ncnn-vulkan (xinntao, v0.2.5.0 Windows build, `realesrgan-x4plus`, 4x twice →
1024x1024, kept in scratch). Lettering/roundel/background are much sharper; the tiny
"SPORTS" wordmark inside the EA circle is unrecoverable (garbled by both models).
GDK validator requires exact sizes (ILI_LogoFileBadDimensions for 400/600/176 and
3840x2160), so `metadata/gdk_hd/` holds exact 100/150/44 and 1920x1080 rendered
down from the 1024 art; `package_gdk.ps1 -VisualsDir` defaults to it. Exe ICO
(16–256) also regenerated from the 1024 art. Validator: 0 failures, Success.
Reinstalled.

Xbox app tile still soft: it draws the package's 150x150 Square150x150Logo at ~380 px
on a 4K screen (and had cached the old image). Tried to make a sharp tile the
default — did NOT work, reverted:
- `*.scale-200/400.png` variants: validator accepts them (0 failures), but
  `makepkg localize` only indexes base names + language folders, and a
  `makepri new` Resources.pri still had a single Logo.png candidate. makepkg
  localize also needs Windows SDK MrmSupport.dll on PATH.
So no higher-res tile via MicrosoftGame.config for a sideloaded package. Store
titles get large library art from their Partner Center catalog, which a local
identity doesn't have. Working route: Xbox app → Edit → image →
`metadata/gdk_hd/title_1024.png`.

Build: settings copy moved from POST_BUILD (only ran on relink) to its own
custom target so TOML-only edits are copied.

Tried and rejected as bulk heuristics (too noisy):
- `addi r3,r3,X; b` pattern: 133 non-function hits, 130 are mid-function code.
- labels outside their function's start..next-start range: 1,727 hits, many from
  normal discontiguous code.

Hardware: Intel iGPU + NVIDIA RTX 5080 Laptop; Windows GPU preference for the
exe set to High performance.

## Progress
- [x] ISO extracted
- [x] `rexglue init` run
- [x] Codegen completes without errors
- [x] Builds and links (2026-09-13)
- [x] Reaches EA logo / intro video (2026-09-13, green artifacts)
- [x] "Press Start" screen (2026-09-13)
- [ ] Main menu
- [ ] In-game
