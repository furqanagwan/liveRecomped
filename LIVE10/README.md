<p align="center">
  <img src="metadata/gdk_hd/title_1024.png" alt="NBA LIVE 10" width="320">
</p>

<h1 align="center">NBA LIVE 10</h1>

<p align="center">
  Native PC static recompilation of the Xbox 360 version, built on the
  <a href="../README.md">liveRecomped</a> framework and ReXGlue.
</p>

> The title artwork above is extracted from your own `default.xex` and is not
> committed; it appears after the artwork step (see [Artwork](#artwork)).

## Game

| | |
| --- | --- |
| Developer | EA Canada |
| Publisher | Electronic Arts (EA SPORTS) |
| Series | NBA LIVE |
| Platform recompiled | Xbox 360 |
| Released | October 2009 |
| Genre | Sports, basketball |
| Modes | Single-player, local multiplayer, online (servers offline) |
| Achievements | 33, 1000 Gamerscore |

## Disc

| | |
| --- | --- |
| Region | Europe, Asia |
| Title ID | `454108C1` |
| Contents | 64 files, 6,267,027,825 bytes |
| Executable | `default.xex`, 20,299,776 bytes |
| Audio languages | English, French, Spanish |
| Archives | `data1.big` to `data7.big` |
| DLL modules | None |

## Status

| Area | State |
| --- | --- |
| Boot, menus | Working |
| Profile creation and loading | Working (needed the SDK 64-bit argument fix) |
| Music | Plays, but sometimes stops; under investigation |
| Controllers | Working through SDL |
| Matches | Not yet tested |
| Xbox PC app, UWP builds | Configured, not yet tested |
| Linux, macOS, Steam Deck | Builds expected, not play-tested |

## Getting started

1. Build from the repository root:
   `.\scripts\build.ps1 -Game LIVE10` or `./scripts/build.sh LIVE10`.
2. Launch `NBA LIVE 10`. On first run choose your Xbox 360 ISO; the files are
   extracted once.
3. Open the system menu with **View + Menu** (or **Esc**) for Settings and Exit.

While the music issue is open, start the game with extra logging so a failure
can be diagnosed:

```powershell
& ".\LIVE10\out\build\win-amd64-release\NBA LIVE 10.exe" --log_level=debug
```

## Default settings

`settings/nba_live_10.toml` starts from the NBA LIVE 09 settings, which fixed a
black screen after pausing (`rov` / `fsi` render target paths) and heap
corruption at match start (no background pipeline creation). Whether LIVE 10
needs them has not been tested separately.

## Recompilation notes

| | |
| --- | --- |
| Generated sources | 570 files, about 291 MB |
| Function seeds | 836 in `config/functions.toml` |
| Disabled seeds | 19 in `config/disabled_function_seeds.txt` (they split real functions) |
| Kernel stubs | Xbox Live Vision camera, shared from `common/src/kernel` |
| Known codegen warnings | 26 unhandled `vpkd3d128` float16 packs, one 1.36 MB function |

The full log of what was found and fixed is in [docs/NOTES.md](docs/NOTES.md).

## Xbox Developer Mode (UWP)

Same flow as NBA LIVE 09, using `uwp/AppxManifest.xml`:

```powershell
.\scripts\build.ps1 -Game LIVE10 -Preset win-amd64-uwp-release
.\scripts\package_uwp.ps1 -Game LIVE10 -Register
.\scripts\package_uwp.ps1 -Game LIVE10 -Pack
```

## Artwork

1. Extract achievements and the title image:
   `rexglue init --project-name nba_live_10 --xex-path assets\default.xex achievements assets\default.xex metadata`
2. Upscale `metadata/icons/title.png` 4x twice with Real-ESRGAN
   (`realesrgan-x4plus`) to `metadata/gdk_hd/title_1024.png`.
3. `.\scripts\generate_artwork.ps1 -Game LIVE10 -ProjectName nba_live_10`
   writes the exe icon and the Xbox app images.

## Legal

Not affiliated with or endorsed by Electronic Arts or Microsoft. NBA LIVE and
EA SPORTS are trademarks of Electronic Arts. You must own the game.
