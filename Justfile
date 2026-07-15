set shell := ["bash", "-euo", "pipefail", "-c"]
set export

ROOT_DIR := justfile_directory()
CPU_COUNT := num_cpus()

default:
    @just --list

bootstrap:
    apt update
    apt install git gcc-15 g++-15 clang-21 clang-format clang-tidy ninja-build ccache cmake just python3-pip pipx -y
    pipx ensurepath
    pipx install conan

# todo(dgorbunov): Create prerequisites check script & move to separate area
apply-core-isol-and-perf:
    cp /etc/default/grub "/etc/default/grub.bak_$(date +%Y-%m-%d_%H-%M-%S)"
    sed -i 's/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT="isolcpus=domain,managed_irq,2-7 nohz_full=2-7 rcu_nocbs=2-7 irqaffinity=0-1,8-23 processor.max_cstate=1 intel_idle.max_cstate=0 default_hugepagesz=1G hugepagesz=1G hugepages=8"/' /etc/default/grub
    update-grub

    apt install linux-cpupower -y
    cpupower frequency-set -g performance

    echo 1 | tee /sys/devices/system/cpu/intel_pstate/no_turbo

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
    @just configure "{{ preset }}"
    cmake --build --preset "{{ preset }}" --parallel "{{ CPU_COUNT }}"

build-from-scratch preset='clang-release':
    @just clean "{{ preset }}"
    @just build "{{ preset }}"

test-from-scratch preset='clang-debug-asan':
    @just build-from-scratch "{{ preset }}"
    ctest --parallel "{{ CPU_COUNT }}" --test-dir "cmake-build-{{ preset }}" --output-on-failure --verbose

format:
    git ls-files -z -- '*.cpp' '*.h' | xargs -0 -r clang-format --style=file -i

format-check:
    git ls-files -z -- '*.cpp' '*.h' | xargs -0 -r clang-format --style=file --dry-run --Werror

lint preset:
    test -f "cmake-build-{{ preset }}/compile_commands.json"
    git ls-files -z -- '*.cpp' | xargs -0 -r clang-tidy -p "./cmake-build-{{ preset }}/" --quiet

lint-from-scratch preset='clang-debug':
    @just clean {{ preset }}
    @just build {{ preset }}
    @just lint {{ preset }}

run *args:
    python3 ./scripts/run_isolated.py {{ args }} || true

act:
    gh act --container-architecture linux/amd64

patch config_file:
    ./scripts/patch_config.py "{{ config_file }}"

benchmark application config_file preset='clang-release':
    @just build-from-scratch "{{ preset }}"
    @just patch "{{ config_file }}"
    @just run timeout -k 5s 60s "./cmake-build-{{ preset }}/exchange/application/{{ application }}" "{{ config_file }}"
    "./cmake-build-{{ preset }}/exchange/application/calc_stats" "{{ config_file }}"
    ./scripts/calc_stats.py "{{ config_file }}"
