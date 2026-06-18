set shell := ["bash", "-eu", "-c"]
set export

ROOT_DIR := justfile_directory()
DEFAULT_BUILD_DIRECTORY := ROOT_DIR + "/build"

default:
    @just --list

bootstrap:
    apt update
    apt install gcc clang cmake just python3-pip pipx -y
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

test-from-scratch sanitizer="none" build_type="Debug":
    rm -Rf build/
    conan install . --build=missing --output-folder "{{ DEFAULT_BUILD_DIRECTORY }}" -s build_type="{{build_type}}"
    cmake -S . -B "{{ DEFAULT_BUILD_DIRECTORY }}" -DSANITIZER=address -DCMAKE_TOOLCHAIN_FILE="{{ DEFAULT_BUILD_DIRECTORY }}/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE="{{ build_type }}"
    cmake --build "{{ DEFAULT_BUILD_DIRECTORY }}"
    ctest --test-dir "{{ DEFAULT_BUILD_DIRECTORY }}" --output-on-failure --verbose
