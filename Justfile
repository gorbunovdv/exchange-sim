set shell := ["bash", "-euo", "pipefail", "-c"]
set export

ROOT_DIR := justfile_directory()
PATH := ROOT_DIR + "/.venv/bin:" + env_var("PATH")
CPU_COUNT := num_cpus()

default:
    @just --list

venv:
    python3 -m venv .venv
    pip install --upgrade pip
    pip install -r requirements.txt

bootstrap:
    apt update
    apt install git gcc-15 g++-15 clang-21 clang-format clang-tidy ninja-build ccache cmake just python3-venv -y
    just venv

# todo(dgorbunov): Create prerequisites check script & move to separate area
apply-core-isol-and-perf:
    cp /etc/default/grub "/etc/default/grub.bak_$(date +%Y-%m-%d_%H-%M-%S)"
    sed -i 's/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT="isolcpus=domain,managed_irq,2-7 nohz_full=2-7 rcu_nocbs=2-7 irqaffinity=0-1,8-23 processor.max_cstate=1 intel_idle.max_cstate=0 default_hugepagesz=1G hugepagesz=1G hugepages=8"/' /etc/default/grub
    update-grub

    apt install linux-cpupower -y
    cpupower frequency-set -g performance

    echo 0 | tee /sys/devices/system/cpu/intel_pstate/no_turbo

    apt install linux-tools-generic -y
    echo 'kernel.perf_event_paranoid=1' | tee /etc/sysctl.d/99-perf.conf
    sysctl --system

    sudo systemctl set-property --runtime system.slice AllowedCPUs=0-1,8-23
    sudo systemctl set-property --runtime user.slice   AllowedCPUs=0-1,8-23

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
    ctest --parallel "{{ CPU_COUNT }}" --test-dir "cmake-build-{{ preset }}" --output-on-failure --verbose

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

act:
    gh act --container-architecture linux/amd64

benchmark-one application config_file preset='clang-release':
    just build-from-scratch "{{ preset }}"
    python3 ./scripts/patch_config.py "{{ config_file }}" --quiet
    python3 ./scripts/write_metadata.py --quiet "{{ config_file }}" --preset "{{ preset }}" --application "{{ application }}"
    just run timeout --preserve-status -k 5s 60s "./cmake-build-{{ preset }}/exchange/application/{{ application }}" "{{ config_file }}"
    just run "./cmake-build-{{ preset }}/exchange/application/calc_stats" "{{ config_file }}"
    python3 ./scripts/calc_stats.py --quiet "{{ config_file }}"

benchmark *args:
    python3 ./scripts/benchmark.py {{ args }}

evaluate *args:
    python3 ./scripts/evaluate_runs.py {{ args }}
