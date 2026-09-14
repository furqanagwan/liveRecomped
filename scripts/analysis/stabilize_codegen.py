import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from prune_bad_seeds import disable_seeds, seeds_splitting_functions, split_branches
from recomp_project import RecompProject

UNRESOLVED_CALL = re.compile(r"^\s+0x([0-9A-Fa-f]{8}) from 0x[0-9A-Fa-f]{8}: .*target not in any function",
                             re.MULTILINE)


def find_rexglue(explicit: str | None) -> str:
    if explicit:
        return explicit
    on_path = shutil.which("rexglue")
    if on_path:
        return on_path
    for prefix in filter(None, os.environ.get("CMAKE_PREFIX_PATH", "").split(os.pathsep)):
        candidate = Path(prefix) / "bin" / ("rexglue.exe" if os.name == "nt" else "rexglue")
        if candidate.exists():
            return str(candidate)
    raise SystemExit("rexglue not found; pass --rexglue or add the SDK bin folder to PATH")


def run_codegen(rexglue: str, project: RecompProject, log_path: Path) -> tuple[int, str]:
    manifest = next(project.root.glob("*_manifest.toml"), None)
    if manifest is None:
        raise SystemExit(f"No *_manifest.toml in {project.root}")
    result = subprocess.run([rexglue, "codegen", manifest.name], cwd=project.root,
                            capture_output=True, text=True, errors="replace")
    output = result.stdout + result.stderr
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text(output, encoding="utf-8")
    return result.returncode, output


def seed_unresolved_calls(project: RecompProject, output: str) -> int:
    existing = project.seeds() | project.disabled_seeds()
    targets = sorted({int(address, 16) for address in UNRESOLVED_CALL.findall(output)} - existing)
    if targets:
        text = project.functions_config.read_text().rstrip("\n")
        new_lines = "\n".join(f'"0x{target:08X}" = {{}}' for target in targets)
        project.functions_config.write_text(f"{text}\n{new_lines}\n", newline="\n")
    return len(targets)


def main():
    parser = argparse.ArgumentParser(
        description="Run codegen until it is clean: seed unresolved calls and disable seeds that split functions.")
    parser.add_argument("--game", required=True, help="game folder, e.g. LIVE10")
    parser.add_argument("--rexglue", help="path to the rexglue executable")
    parser.add_argument("--rounds", type=int, default=10, help="maximum codegen passes")
    args = parser.parse_args()

    project = RecompProject(args.game)
    rexglue = find_rexglue(args.rexglue)
    log_path = project.root / "out" / "codegen.log"

    for round_number in range(1, args.rounds + 1):
        exit_code, output = run_codegen(rexglue, project, log_path)
        branches = split_branches(output)
        if branches:
            blamed = seeds_splitting_functions(project, branches)
            disable_seeds(project, blamed)
            print(f"round {round_number}: {len(branches)} split branches, {len(blamed)} seeds disabled")
            if blamed:
                continue
        added = seed_unresolved_calls(project, output)
        if added:
            print(f"round {round_number}: seeded {added} unresolved call targets")
            continue
        if exit_code != 0:
            raise SystemExit(f"codegen failed without fixable errors; see {log_path}")
        print(f"round {round_number}: codegen clean ({len(project.seeds())} seeds); log {log_path}")
        return
    raise SystemExit(f"codegen still not clean after {args.rounds} rounds; see {log_path}")


if __name__ == "__main__":
    main()
