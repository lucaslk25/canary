#!/usr/bin/env bash

set -euo pipefail

PRESET_DEFAULT="linux-release"
VCPKG_ROOT_DEFAULT="$HOME/repos/vcpkg"
JOBS_DEFAULT="$(nproc 2>/dev/null || echo 4)"

usage() {
	cat <<'EOF'
Usage: recompile.sh [--run-only|--build-only] [--reconfigure] [--jobs N] [--preset NAME] [--vcpkg-root PATH]

Default behavior (Linux):
  - Ensure build/ exists
  - Configure (if needed) using CMake preset (default: linux-release)
  - Build using the matching build preset
  - Copy the resulting executable to ./canary (keeping ./canary.old if present)
  - Run a smoke test: build/<preset>/bin/canary --help

Options:
  --run-only        Skip configure/build; only run the smoke test.
  --build-only      Only configure/build/copy; do not run the smoke test.
  --reconfigure     Force re-running configure step even if already configured.
  --jobs N          Parallel build jobs (default: nproc).
  --preset NAME     CMake preset to use (default: linux-release).
  --vcpkg-root PATH vcpkg root directory (default: $HOME/repos/vcpkg).

Legacy positional args (still supported):
  recompile.sh [VCPKG_ROOT] [PRESET]
EOF
}

info() {
	echo -e "\033[1;34m[INFO]\033[0m $1"
}

check_command() {
	if ! command -v "$1" >/dev/null; then
		echo "The command '$1' is not available. Please install it and try again." >&2
		exit 1
	fi
}

RUN_ONLY=0
BUILD_ONLY=0
RECONFIGURE=0
JOBS="$JOBS_DEFAULT"
PRESET="$PRESET_DEFAULT"
VCPKG_ROOT="$VCPKG_ROOT_DEFAULT"

# Legacy positional args: [VCPKG_ROOT] [PRESET]
if [[ ${1:-} != "" && ${1:-} != --* ]]; then
	VCPKG_ROOT="$1"
	shift
fi
if [[ ${1:-} != "" && ${1:-} != --* ]]; then
	PRESET="$1"
	shift
fi

while [[ $# -gt 0 ]]; do
	case "$1" in
		--run-only)
			RUN_ONLY=1
			shift
			;;
		--build-only)
			BUILD_ONLY=1
			shift
			;;
		--reconfigure)
			RECONFIGURE=1
			shift
			;;
		--jobs|-j)
			JOBS="${2:?missing value for --jobs}"
			shift 2
			;;
		--preset)
			PRESET="${2:?missing value for --preset}"
			shift 2
			;;
		--vcpkg-root)
			VCPKG_ROOT="${2:?missing value for --vcpkg-root}"
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown argument: $1" >&2
			usage >&2
			exit 2
			;;
	esac
done

if [[ $RUN_ONLY -eq 1 && $BUILD_ONLY -eq 1 ]]; then
	echo "Cannot use --run-only and --build-only together." >&2
	exit 2
fi

check_command "cmake"

# Ensure we run from the repo root (where CMakePresets.json lives).
REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}" )" && pwd)"
cd "$REPO_ROOT"

mkdir -p build

BIN_PATH="build/${PRESET}/bin/canary"
BUILD_SENTINEL="build/${PRESET}/build.ninja"

if [[ $RUN_ONLY -eq 0 ]]; then
	if [[ ! -d "$VCPKG_ROOT" ]]; then
		echo "VCPKG_ROOT does not exist: $VCPKG_ROOT" >&2
		echo "Pass --vcpkg-root PATH (or legacy: recompile.sh /path/to/vcpkg)" >&2
		exit 1
	fi

	if [[ $RECONFIGURE -eq 1 || ! -f "$BUILD_SENTINEL" ]]; then
		info "Configuring with preset '$PRESET' (VCPKG_ROOT=$VCPKG_ROOT)"
		VCPKG_ROOT="$VCPKG_ROOT" cmake --preset "$PRESET"
	fi

	info "Building preset '$PRESET' (-j$JOBS)"
	VCPKG_ROOT="$VCPKG_ROOT" cmake --build --preset "$PRESET" -j"$JOBS"

	# Keep previous behavior: copy built executable to ./canary for start.sh/start_gdb.sh
	if [[ -x "$BIN_PATH" ]]; then
		if [[ -e "./canary" ]]; then
			info "Saving old build as ./canary.old"
			mv ./canary ./canary.old
		fi
		info "Copying built executable to ./canary"
		cp "$BIN_PATH" ./canary
	fi
fi

if [[ $BUILD_ONLY -eq 0 ]]; then
	if [[ ! -x "$BIN_PATH" ]]; then
		echo "Executable not found or not executable: $BIN_PATH" >&2
		echo "Try running without --run-only to build it first." >&2
		exit 1
	fi

	# Canary currently doesn't print help for --help; it starts normally and may wait for Enter on failure.
	# Redirect stdin to avoid hanging in CI/automation.
	tmp_log="$(mktemp)"
	set +e
	"$BIN_PATH" --help </dev/null 2>&1 | tee "$tmp_log"
	canary_rc=${PIPESTATUS[0]}
	set -e

	# Until infra is configured, it is normal for canary to exit non-zero due to DB connection failure.
	if [[ $canary_rc -ne 0 ]]; then
		if grep -qE "Failed to connect to database|Can't connect to server on '127\\.0\\.0\\.1'" "$tmp_log"; then
			echo "Note: canary exited with code $canary_rc due to missing database/infra; treating as a successful smoke test." >&2
			rm -f "$tmp_log"
			exit 0
		fi
	fi

	rm -f "$tmp_log"
	exit "$canary_rc"
fi
