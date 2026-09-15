# liveRecomped

Unofficial native PC versions of EA's NBA LIVE games for Xbox 360, made by
statically recompiling the original game code with
[ReXGlue](https://github.com/rexglue/rexglue-sdk). Download the game's
executable, point it at your own Xbox 360 disc image, and play.

This repository and its releases contain no game data: no disc images, game
files or extracted assets. You must own the game.

| Game | Supported disc | Status | Download |
| --- | --- | --- | --- |
| [NBA LIVE 09](LIVE09/README.md) | 🇪🇺 Europe (`4541087A`) | Boots, menus, practice, Play Now matches | [Releases](https://github.com/furqanagwan/liveRecomped/releases?q=LIVE09) |
| [NBA LIVE 10](LIVE10/README.md) | 🇪🇺 🌏 Europe, Asia (`454108C1`) | Boots, menus, profiles save; music sometimes stops | [Releases](https://github.com/furqanagwan/liveRecomped/releases?q=LIVE10) |

## Playing

1. Check the [system requirements](#system-requirements) and install the
   [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).
2. Download the game's zip from [Releases](https://github.com/furqanagwan/liveRecomped/releases)
   and extract it to a folder you can write to (not Program Files).
3. Run the game's `.exe` and choose your Xbox 360 ISO when asked. The files are
   copied next to the executable once; the ISO isn't needed after that.

Check your disc against the game's supported regions first (see its README):
each release is recompiled from one regional executable.

### Controls

- Xbox, PlayStation, Switch and Steam Deck controllers work out of the box; all
  controllers drive player 1 unless *All controllers control player 1* is turned
  off in Settings.
- System menu (Resume, Settings, Exit Game): press **View + Menu** together, or
  **Esc**. The Guide button is reserved for Windows Game Bar, Xbox mode and Steam,
  and only opens the menu when `guide_button = true`.
- In menus: D-pad or left stick to move, A to select, B to go back.

### DLC

Put downloadable content packages (the `CON`, `LIVE` or `PIRS` files from an
Xbox 360 `Content\0000000000000000\<TitleID>\00000002` folder) in the `dlc`
folder next to the executable. Each package is checked against the game's title
ID and installed on the next start. Title updates are skipped: they replace game
code, so they need a new recompilation.

### Saves and settings

Saves, settings and logs go to your user folder; Settings > Game files shows
where. An empty `portable.txt` next to the executable keeps them beside it.

## System requirements

| | Required |
| --- | --- |
| OS | Windows 10 version 2004 (build 19041) or Windows 11, 64-bit |
| Processor | 64-bit x86 CPU with SSE4.1 |
| Graphics | DirectX 12 GPU (feature level 11_0) |
| Memory | 8 GB RAM recommended |
| Storage | NBA LIVE 09: 6.5 GB, NBA LIVE 10: 6.5 GB, plus room for the ISO while it is copied |
| Software | [Microsoft Visual C++ Redistributable 2015-2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) |
| Game | Your own Xbox 360 disc image of a supported region |

Tested on an Intel Core Ultra 9 275HX, NVIDIA GeForce RTX 5080 Laptop GPU and
32 GB RAM running Windows 11. Lower-end hardware hasn't been tested yet; reports
are welcome. Linux, macOS and Steam Deck builds compile but have no releases and
haven't been play-tested.

## Developing

The shared app framework (installer, menus, input, packaging scripts) lives in
[recomp-framework](https://github.com/furqanagwan/recomp-framework), included here
as the `framework` submodule together with the ReXGlue fork.

```
git clone --recursive https://github.com/furqanagwan/liveRecomped.git
cd liveRecomped
rexglue extract "<your disc>.iso" LIVE09\assets
.\framework\scripts\build.ps1 -Game LIVE09
```

```
framework/                  recomp-framework submodule (with thirdparty/rexglue-sdk)
LIVE09/                     NBA LIVE 09: descriptor, codegen config, settings, GDK/UWP metadata
LIVE10/                     NBA LIVE 10
<GAME>/docs/NOTES.md        Research notes: codegen, crashes and fixes
<GAME>/release.json         Supported disc and system requirements for release packaging
```

A game folder only holds what is unique to that title: a `GameDescriptor`,
game-specific kernel stubs or hooks, the codegen overrides in `config/`, runtime
defaults in `settings/`, and packaging metadata in `gdk/` and `uwp/`. Everything
else comes from the framework.

### Settings NBA LIVE needs

Defaults live in `<GAME>/settings/<project>.toml` and are copied next to the
executable. Changes made in the in-game Settings menu are saved to
`settings.toml` in the user data folder and override the defaults; command-line
flags override both.

| Setting | Why |
| --- | --- |
| `render_target_path_d3d12 = "rov"` / `render_target_path_vulkan = "fsi"` | Host render targets leave the practice court black after pausing |
| `d3d12_pipeline_creation_threads = 0`, `async_shader_compilation = false` | Background pipeline creation corrupted the heap when a match started |
| `vsync = true`, `video_mode_refresh_rate = 60` | Game and cutscene speed are tied to 60 Hz |

See [CONTRIBUTING.md](CONTRIBUTING.md) for setup, adding NBA LIVE 11 and making
releases.

## License

The code in this repository is BSD 3-Clause, see [LICENSE](LICENSE).

Not affiliated with or endorsed by Electronic Arts or Microsoft. NBA LIVE and
EA SPORTS are trademarks of Electronic Arts. Releases contain code recompiled
from the original games but no game data; you must own the game to play.
