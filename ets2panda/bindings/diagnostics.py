#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import Iterable


SUPPORTED_SUFFIXES = (".ets", ".d.ets", ".ts", ".d.ts")
SEVERITY_NAMES = {1: "error", 2: "warning", 3: "information", 4: "hint"}
WARNING_SEVERITY = 2
DEFAULT_EXCLUDED_DIRS = ("arkts/builtin", "build-tools")
DEFAULT_WHITELIST: tuple[str, ...] = (
    "api/@ohos.app.ability.FenceExtensionAbility.d.ets",
    "api/@ohos.app.ability.FenceExtensionContext.d.ets",
    "api/@ohos.app.ability.VpnExtensionAbility.d.ets",
    "api/@ohos.app.ability.verticalPanelManager.d.ets",
    "api/@ohos.net.vpn.d.ets",
    "api/@ohos.arkui.advanced.ChipV2.d.ets",
    "api/@ohos.arkui.intelligence.imageGeneration.d.ets",
    "api/@ohos.arkui.theme.d.ets",
    "api/application/AccessibilityExtensionContext.d.ets",
    "api/arkui/component/common.d.ets"
)
RESULT_MARKER = "__ETS_DIAGNOSTICS_RESULT__"


class DiagnosticsError(RuntimeError):
    """Raised when syntactic or semantic diagnostics are found."""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run ets2panda LSP syntactic and semantic diagnostics."
    )
    parser.add_argument(
        "--sdk-path",
        dest="paths",
        nargs="+",
        required=True,
        help="ArkTS/TypeScript files or directories to check",
    )
    parser.add_argument(
        "--check-sdk", choices=("syntactic", "semantic", "both"), default="both",
        help="diagnostic pass to run (default: both)",
    )
    parser.add_argument(
        "--build-tools",
        help="build-tools directory; defaults to the parent of the script's bindings directory",
    )
    parser.add_argument("--project-root", help="module root used for import resolution")
    parser.add_argument(
        "--exclude",
        dest="excluded_dirs",
        action="append",
        default=list(DEFAULT_EXCLUDED_DIRS),
        help="directory to skip recursively; may be repeated (default: arkts/builtin)",
    )
    parser.add_argument(
        "--whitelist",
        dest="whitelist",
        action="append",
        default=list(DEFAULT_WHITELIST),
        help="known failing declaration file to suppress; matched as a path-suffix of the "
        "slash-normalized diagnostic file path, e.g. api/@ohos.xxx.d.ets or @ohos.xxx.d.ets; "
        "an omitted .d.ets suffix is appended automatically; may be repeated",
    )
    parser.add_argument("--node", default=os.environ.get("NODE", "node"), help="Node.js executable")
    parser.add_argument("--cache-dir", help="persistent LSP cache directory")
    parser.add_argument("--stamp", help="stamp file written only after successful diagnostics")
    parser.add_argument(
        "--init-ast-cache", action="store_true",
        help="initialize the full-project AST cache before diagnostics (high memory)",
    )
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    return parser.parse_args()


def is_supported(path: Path) -> bool:
    return any(path.name.endswith(suffix) for suffix in SUPPORTED_SUFFIXES)


def is_excluded(path: Path, excluded_dirs: Iterable[str]) -> bool:
    if path.is_file():
        path = path.parent
    path_parts = path.parts
    for value in excluded_dirs:
        excluded = Path(value)
        if excluded.is_absolute():
            excluded_path = excluded.resolve()
            if path == excluded_path or excluded_path in path.parents:
                return True
            continue
        excluded_parts = excluded.parts
        width = len(excluded_parts)
        if any(path_parts[index:index + width] == excluded_parts
               for index in range(len(path_parts) - width + 1)):
            return True
    return False


def is_whitelisted(file_name: str, whitelist: Iterable[str]) -> bool:
    normalized = file_name.replace("\\", "/")
    for entry in whitelist:
        if not entry:
            continue
        pattern = entry.replace("\\", "/")
        if (normalized == pattern or normalized.endswith("/" + pattern)
                or normalized.endswith("/" + pattern + ".d.ets")):
            return True
    return False


def collect_files(paths: Iterable[str], excluded_dirs: Iterable[str]) -> list[Path]:
    files: set[Path] = set()
    for value in paths:
        target = Path(value).resolve()
        if not target.exists():
            raise ValueError(f"Path does not exist: {target}")
        if target.is_file():
            if is_excluded(target.parent, excluded_dirs):
                continue
            if not is_supported(target):
                raise ValueError(f"Unsupported file type: {target}")
            files.add(target)
            continue
        for path in target.rglob("*"):
            if is_excluded(path, excluded_dirs):
                continue
            if path.is_file() and is_supported(path):
                files.add(path)
    return sorted(files)


def common_project_root(files: list[Path], explicit_root: str | None) -> Path:
    if explicit_root:
        return Path(explicit_root).resolve()
    parents = [str(path.parent) for path in files]
    return Path(os.path.commonpath(parents)).resolve()


def resolve_runner(build_tools_arg: str | None) -> tuple[Path, Path]:
    script_dir = Path(__file__).resolve().parent
    build_tools = Path(build_tools_arg).resolve() if build_tools_arg else script_dir.parent
    runner = build_tools / "bindings" / "dist" / "diagnostics.js"
    if not runner.is_file():
        raise ValueError(f"LSP diagnostics runner not found: {runner}")
    return build_tools, runner


def build_request(args: argparse.Namespace, build_tools: Path, files: list[Path]) -> dict:
    checks = [args.check_sdk] if args.check_sdk != "both" else ["syntactic", "semantic"]
    request = {
        "buildTools": str(build_tools),
        "projectRoot": str(common_project_root(files, args.project_root)),
        "files": [str(path) for path in files],
        "checks": checks,
        "initAstCache": args.init_ast_cache,
    }
    if args.cache_dir:
        request["cacheDir"] = str(Path(args.cache_dir).resolve())
    return request


def build_env(build_tools: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["BINDINGS_PATH"] = str(build_tools / "bindings")
    lib_dir = build_tools / "ets2panda" / "lib"
    env["LD_LIBRARY_PATH"] = os.pathsep.join(
        value for value in (str(lib_dir), env.get("LD_LIBRARY_PATH", "")) if value
    )
    return env


def invoke_runner(node: str, runner: Path, request: dict, build_tools: Path) -> dict | None:
    completed = subprocess.run(
        [node, str(runner)], input=json.dumps(request), text=True,
        capture_output=True, env=build_env(build_tools), check=False,
    )
    if completed.returncode != 0:
        sys.stderr.write(completed.stderr or completed.stdout)
        return None
    marker_index = completed.stdout.rfind(RESULT_MARKER)
    if marker_index < 0:
        sys.stderr.write(completed.stderr or completed.stdout)
        return None
    return json.loads(completed.stdout[marker_index + len(RESULT_MARKER):])


def partition_diagnostics(result: dict, whitelist: Iterable[str]) -> tuple[list[dict], list[dict]]:
    # Warnings are informational for SDK validation and should not affect
    # the error count or fail the build.
    reported = [
        diagnostic for diagnostic in result.get("diagnostics", [])
        if diagnostic.get("severity") != WARNING_SEVERITY
    ]
    diagnostics = []
    whitelisted = []
    for diagnostic in reported:
        if is_whitelisted(diagnostic.get("file", ""), whitelist):
            whitelisted.append(diagnostic)
        else:
            diagnostics.append(diagnostic)
    return diagnostics, whitelisted


def format_summary(file_count: int, diagnostics: list[dict], whitelisted: list[dict]) -> str:
    suppressed = f", suppressed {len(whitelisted)} whitelisted diagnostic(s)" if whitelisted else ""
    return (
        f"[unpublished directory] Checked {file_count} file(s), "
        f"found {len(diagnostics)} diagnostic(s){suppressed}."
    )


def print_diagnostic_line(diagnostic: dict, label: str) -> None:
    start = diagnostic["range"]["start"]
    print(
        f'{diagnostic["file"]}:{start["line"]}:{start["character"]}: '
        f'{label} [{diagnostic["kind"]}] {diagnostic["message"]}'
    )


def emit_report(result: dict, diagnostics: list[dict], whitelisted: list[dict], as_json: bool) -> None:
    if as_json:
        json.dump(result, sys.stdout, indent=2)
        sys.stdout.write("\n")
        return
    for diagnostic in diagnostics:
        severity = SEVERITY_NAMES.get(diagnostic["severity"], str(diagnostic["severity"]))
        print_diagnostic_line(diagnostic, severity)
    for diagnostic in whitelisted:
        print_diagnostic_line(diagnostic, "whitelisted")


def write_stamp(stamp_arg: str | None) -> None:
    if not stamp_arg:
        return
    stamp = Path(stamp_arg).resolve()
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.touch()


def main() -> int:
    start_time = time.perf_counter()
    try:
        args = parse_args()
        files = collect_files(args.paths, args.excluded_dirs)
        if not files:
            raise ValueError("No .ets or .ts files found")

        build_tools, runner = resolve_runner(args.build_tools)
        request = build_request(args, build_tools, files)
        result = invoke_runner(args.node, runner, request, build_tools)
        if result is None:
            return 2

        diagnostics, whitelisted = partition_diagnostics(result, args.whitelist)
        summary = format_summary(len(files), diagnostics, whitelisted)
        emit_report(result, diagnostics, whitelisted, args.json)
        if diagnostics:
            raise DiagnosticsError(summary)
        if not args.json:
            print(summary)
        write_stamp(args.stamp)
        return 0
    except (OSError, ValueError) as error:
        print(f"diagnostics.py: {error}", file=sys.stderr)
        return 2
    finally:
        elapsed = time.perf_counter() - start_time
        print(f"diagnostics.py: elapsed time: {elapsed:.3f}s", file=sys.stderr)


if __name__ == "__main__":
    sys.exit(main())
