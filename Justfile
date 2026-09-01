set shell := ["bash", "-euo", "pipefail", "-c"]
set export

ROOT_DIR := justfile_directory()
PATH := ROOT_DIR + "/.venv/bin:" + env_var("PATH")
CPU_COUNT := num_cpus()

import? 'extra/Justfile'

default:
    @just --list

venv:
    python3 -m venv .venv
    pip install --upgrade pip
    pip install -r requirements.txt

bootstrap:
    apt update
    apt install linux-tools-generic linux-cpupower jq git gcc-15 g++-15 clang-21 llvm-21 clang-format clang-tidy ninja-build ccache cmake just python3-venv -y
    just venv

patch-grub *args:
    python3 ./scripts/apply_os_settings.py patch-grub {{ args }}

apply-runtime *args:
    python3 ./scripts/apply_os_settings.py apply-runtime {{ args }}

clean preset:
    rm -Rf "cmake-build-{{ preset }}"

configure preset='clang-release':
    cmake --preset "{{ preset }}"

build preset='clang-release':
    just configure "{{ preset }}"
    cmake --build --preset "{{ preset }}" --parallel "{{ CPU_COUNT }}"

verify-headers preset='clang-debug':
    just configure "{{ preset }}"
    cmake --build --preset "{{ preset }}" --parallel "{{ CPU_COUNT }}" --target all_verify_interface_header_sets

build-from-scratch preset='clang-release':
    just clean "{{ preset }}"
    just build "{{ preset }}"

test preset='clang-debug-asan':
    just build "{{ preset }}"
    TSAN_OPTIONS="symbolize=0:halt_on_error=1" ctest --parallel "{{ CPU_COUNT }}" --test-dir "cmake-build-{{ preset }}" --output-on-failure --verbose

test-from-scratch preset='clang-debug-asan':
    just clean "{{ preset }}"
    just test "{{ preset }}"

format:
    git ls-files -z -- '*.cpp' '*.h' | xargs -0 -r clang-format --style=file -i
    git ls-files -z -- '*.py' | xargs -0 -r ruff format

format-check:
    git ls-files -z -- '*.cpp' '*.h' | xargs -0 -r clang-format --style=file --dry-run --Werror
    git ls-files -z -- '*.py' | xargs -0 -r ruff format --check

pylint *args:
    git ls-files -z -- '*.py' | xargs -0 -r ruff check {{ args }}
    mypy

lint preset:
    just build "{{ preset }}"
    test -f "cmake-build-{{ preset }}/compile_commands.json"
    run-clang-tidy -p "./cmake-build-{{ preset }}/" -quiet -j "{{ CPU_COUNT }}" $(git ls-files -- '*.cpp')

lint-from-scratch preset='clang-debug':
    just clean "{{ preset }}"
    just lint "{{ preset }}"

run *args:
    python3 ./scripts/run_isolated.py {{ args }}

run-one *args:
    python3 ./scripts/run_one.py {{ args }}

act:
    gh act --container-architecture linux/amd64

benchmark-one *args:
    python3 ./scripts/benchmark_one.py {{ args }}

benchmark *args:
    python3 ./scripts/benchmark.py {{ args }}

evaluate *args:
    python3 ./scripts/evaluate_runs.py {{ args }}

_remove-label old_label benchmark_path:
    #!/usr/bin/env bash
    set -euo pipefail
    label="{{ old_label }}"; bp="{{ benchmark_path }}"
    [[ -n "$label" && "$label" != *"/"* && "$label" != "." && "$label" != ".." ]] \
        || { echo "ERROR: bad label '$label'"; exit 1; }
    [[ -d "$bp" ]] || { echo "ERROR: benchmark_path '$bp' is not a directory"; exit 1; }
    shopt -s nullglob
    matches=("$bp"/*/"$label")
    (( ${#matches[@]} )) || { echo "ERROR: label '$label' not found under $bp"; exit 1; }
    for d in "${matches[@]}"; do
        echo "INFO: removing: $d"
        rm -rf -- "$d"
    done
    echo "INFO: removed ${#matches[@]} dir(s) for label '$label'"

_rename-label old_label new_label benchmark_path:
    #!/usr/bin/env bash
    set -euo pipefail
    old="{{ old_label }}"; new="{{ new_label }}"; bp="{{ benchmark_path }}"
    for l in "$old" "$new"; do
        [[ -n "$l" && "$l" != *"/"* && "$l" != "." && "$l" != ".." ]] \
            || { echo "ERROR: bad label '$l'"; exit 1; }
    done
    [[ "$old" != "$new" ]] || { echo "ERROR: labels are identical"; exit 1; }
    [[ -d "$bp" ]] || { echo "ERROR: benchmark_path '$bp' is not a directory"; exit 1; }
    shopt -s nullglob
    matches=("$bp"/*/"$old")
    (( ${#matches[@]} )) || { echo "ERROR: label '$old' not found under $bp"; exit 1; }
    for d in "${matches[@]}"; do
        dst="$(dirname "$d")/$new"
        [[ ! -e "$dst" ]] || { echo "ERROR: '$dst' already exists, aborting before any mv"; exit 1; }
    done
    for d in "${matches[@]}"; do
        dst="$(dirname "$d")/$new"
        echo "INFO: renaming: $d -> $dst"
        mv -- "$d" "$dst"
    done
    echo "INFO: renamed ${#matches[@]} dir(s): '$old' -> '$new'"

remove-label old_label config='./configs/dmitrys-ubuntu.json':
    just _remove-label "{{ old_label }}" "{{ ROOT_DIR }}/$(jq -r '.benchmark_path' {{ config }})"

rename-label old_label new_label config='./configs/dmitrys-ubuntu.json':
    just _rename-label "{{ old_label }}" "{{ new_label }}" "{{ ROOT_DIR }}/$(jq -r '.benchmark_path' {{ config }})"
