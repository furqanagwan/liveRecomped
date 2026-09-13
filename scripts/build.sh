#!/usr/bin/env bash
set -euo pipefail

game="${1:-LIVE09}"
case "$(uname -s)" in
    Darwin) default_preset="mac-$( [ "$(uname -m)" = "arm64" ] && echo arm64 || echo amd64 )-release" ;;
    *) default_preset="linux-$( [ "$(uname -m)" = "aarch64" ] && echo arm64 || echo amd64 )-release" ;;
esac
preset="${2:-$default_preset}"
sdk_dir="${REXSDK_DIR:-}"

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
game_root="$repository_root/$game"
[ -f "$game_root/CMakeLists.txt" ] || { echo "No game project at $game_root" >&2; exit 1; }

configure_arguments=(--preset "$preset")
[ -n "$sdk_dir" ] && configure_arguments+=("-DREXSDK_DIR=$sdk_dir")

cd "$game_root"
cmake "${configure_arguments[@]}"
cmake --build --preset "$preset"
