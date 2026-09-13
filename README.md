# liveRecomped

Native PC static recompilations of EA's NBA LIVE games for Xbox 360, built on
[ReXGlue](https://github.com/rexglue/rexglue-sdk). Every game is recompiled from
the player's own disc image; this repository contains no game data, no generated
code and no extracted artwork.

| Game | Folder | Status |
| --- | --- | --- |
| NBA LIVE 09 (Europe, 4541087A) | `LIVE09/` | Boots, menus, practice, Play Now matches |

## Repository layout

```
cmake/LiveRecomp.cmake      live_recomp_add_game(): shared build setup for every game
common/                     live_common library shared by all games
  include/live/app          LiveRecompApp base class, GameDescriptor, GamePaths
  include/live/installer    DiscImageInstaller (Xbox 360 ISO extraction)
  include/live/input        ControllerMenuWatcher, GuestInputGate, ImGuiGamepadBridge
  include/live/platform     NativeFilePicker, GamingRuntimeSession (Xbox PC app)
  include/live/settings     UserSettingsStore
  include/live/ui           DiscInstallDialog, SystemMenuDialog, SettingsDialog, MonochromeTheme
  include/live/debug        GuestImageDump
LIVE09/                     NBA LIVE 09: descriptor, kernel stubs, codegen config, settings, GDK config
templates/game/             Starting point for the next game
scripts/                    build, packaging, new game, analysis tools
thirdparty/rexglue-sdk      ReXGlue fork with the fixes these games need
```

A game folder only holds what is unique to that title: a `GameDescriptor`,
game-specific kernel stubs or hooks, the codegen overrides in `config/`, runtime
defaults in `settings/`, and packaging metadata in `gdk/`. Everything else comes
from `common/`.

## Requirements

- CMake 3.25+, Ninja, Clang 18+ (Clang 20 on Linux)
- ReXGlue SDK: the `thirdparty/rexglue-sdk` submodule (branch `liverecomp-fixes`),
  either installed (`CMAKE_PREFIX_PATH`) or passed as `REXSDK_DIR`
- Windows: Visual Studio build tools and the Windows SDK; optional Microsoft GDK
  for Xbox PC app integration
- Linux / Steam Deck: Vulkan and GTK development packages as listed in the ReXGlue README

```
git clone --recursive https://github.com/furqanagwan/liveRecomped.git
```

## Building

The recompiled C++ is generated at build time from `<GAME>/assets/default.xex`,
so extract your disc into the game's `assets` folder first (the game's own
installer can do this, see below, or any Xbox 360 ISO extractor).

Windows (Developer PowerShell or plain PowerShell):

```
.\scripts\build.ps1 -Game LIVE09
.\scripts\build.ps1 -Game LIVE09 -Preset win-amd64-relwithdebinfo -SdkDir thirdparty\rexglue-sdk
```

Linux, macOS and Steam Deck:

```
./scripts/build.sh LIVE09
REXSDK_DIR=$PWD/thirdparty/rexglue-sdk ./scripts/build.sh LIVE09 linux-amd64-release
```

Linux, macOS and Steam Deck builds use the Vulkan renderer. They are expected to
compile with the presets in each game folder but have not been play-tested yet.

## First run

If `default.xex` is missing, the game opens a setup window: browse to (or type the
path of) your Xbox 360 ISO and the files are extracted once. The native file
picker is used on Windows, `zenity`/`kdialog` on Linux and Finder on macOS; on
Steam Deck Game Mode type the path. Unattended installs:
`LIVE_RECOMP_INSTALL_ISO=/path/to/game.iso`.

Game files go next to the executable, or into the user data folder when the
executable folder is read-only (packaged installs). An empty `portable.txt` next
to the executable keeps saves, cache and settings beside it.

## Controls

- Xbox, PlayStation, Switch and Steam Deck controllers work through SDL; all
  controllers drive player 1 unless `live_shared_controllers` is turned off.
- System menu (Resume, Settings, Exit Game): press **View + Menu** together, or
  **Esc**. The Guide button is reserved for Windows Game Bar, Xbox mode and Steam,
  and only opens the menu when `guide_button = true`.
- In menus: D-pad or left stick to move, A to select, B to go back.

## Settings

Defaults live in `<GAME>/settings/<project>.toml` and are copied next to the
executable. Changes made in the in-game Settings menu are saved to
`settings.toml` in the user data folder and override the defaults; command-line
flags override both. NBA LIVE 09 needs:

| Setting | Why |
| --- | --- |
| `render_target_path_d3d12 = "rov"` / `render_target_path_vulkan = "fsi"` | Host render targets leave the practice court black after pausing |
| `d3d12_pipeline_creation_threads = 0`, `async_shader_compilation = false` | Background pipeline creation corrupted the heap when a match started |
| `vsync = true`, `video_mode_refresh_rate = 60` | Game and cutscene speed are tied to 60 Hz |

## Xbox PC app (Microsoft GDK)

With the Microsoft GDK installed the build links the Gaming Runtime so the Xbox
app sees the game as running. For local testing (Developer Mode on):

```
.\scripts\package_gdk.ps1 -Game LIVE09 -Pack -Install
```

Shell images must be exactly 100, 150, 44 and 1920x1080 px and are read from
`<GAME>/metadata/gdk_hd`. The packaged identity is a local stand-in with no Store
IDs, so it is for personal testing only.

## Adding the next game

```
.\scripts\new_game.ps1 -Folder LIVE10 -ProjectName nba_live_10 -DisplayName "NBA LIVE 10"
```

Extract the disc into `LIVE10/assets` first. The script runs `rexglue init`,
renders `templates/game`, and wires the codegen config. Then iterate on codegen
until it reports no unresolved calls.

### Recompilation workflow

1. Build; fix `UnresolvedCall` errors by seeding the target addresses in
   `config/functions.toml`.
2. Runtime `Call to invalid or unregistered function`: dump the loaded image with
   `LIVE_RECOMP_DUMP_IMAGE=<GAME>/out/image_dump.bin`, then
   `python scripts/analysis/find_missing_functions.py --game <GAME> [--gaps] --write`.
3. After seeding, codegen must report no `Unresolved conditional branch` or
   `Jump target ... unresolved` lines; otherwise run
   `python scripts/analysis/prune_bad_seeds.py --game <GAME> <codegen log>`,
   which records removed seeds in `config/disabled_function_seeds.txt`.
4. Missing kernel imports at link time become stubs in `src/kernel/`.

Other codegen overrides (`switch_tables`, `midasm_hook`, `indirect_calls`,
`invalid_instructions`, `rexcrt`) follow `rex::codegen::RecompilerConfig`; add a
TOML file under `config/` and list it in the manifest `includes`.

Per-game research notes live in `<GAME>/docs/NOTES.md`.

## ReXGlue fork

`thirdparty/rexglue-sdk` tracks
[furqanagwan/rexglue-sdk@liverecomp-fixes](https://github.com/furqanagwan/rexglue-sdk/tree/liverecomp-fixes):

- codegen: `vpkuwus`/`vpkuhus` read aliased sources before writing (VP6 video colour)
- input: `InputSystem` entry points are serialized (concurrent polling crash)
- gpu/d3d12: issued draws feed the debug overlay counter

## Legal

Not affiliated with or endorsed by Electronic Arts or Microsoft. NBA LIVE is a
trademark of Electronic Arts. You must own the game; no copyrighted game content
is distributed here.
