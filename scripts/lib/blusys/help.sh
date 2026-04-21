# Sourced by repo-root `blusys` — help text for all subcommands.

blusys_help() {
    cat <<'HELP'
Usage: blusys <command> [args...]

All commands default to the current directory when no project is specified.

Commands:
  create [path]    Scaffold a new blusys project (default: cwd)
  gen-spec         Generate build/generated/blusys_app_spec.h from a manifest
  build            Build a project
  flash            Flash a project to a device
  monitor          Open serial monitor
  run              Build, flash, and monitor
  config           Open menuconfig
  menuconfig       Alias for config
  size             Show firmware size breakdown
  erase            Erase entire flash
  clean            Remove build directory for one target
  fullclean        Remove build directories for all targets
  example          Run a command on a bundled example
  lint             Run platform lint checks
  validate         Run inventory, layout, and lint checks
  build-examples   Build all examples for all targets
  build-inventory   Build inventory-filtered examples for one target
  host-build       Build the PC + SDL2 host harness (scripts/host/ or project host/)
  qemu             Build firmware, then run QEMU (UART default; --graphics for IDF framebuffer)
  install-qemu     Download and install Espressif QEMU
  config-idf       Select default ESP-IDF version
  version          Show blusys and ESP-IDF version info
  update           Self-update blusys via git pull
  help             Show this help message

Run 'blusys help <command>' for command-specific help.
HELP
}

blusys_help_create() {
    printf 'Usage: blusys create [--interface <interactive|headless>] [--with <cap1,cap2,...>] [--policy <policy1,...>] [--list] [path]\n'
    printf '\nScaffold a new blusys product project from the explicit product model:\n'
    printf 'interface + capabilities + profile + policies.\n'
    printf '\nOptions:\n'
    printf '  --interface <type>  product interface (default: interactive)\n'
    printf '                       interactive — local UI enabled\n'
    printf '                       headless    — no local UI\n'
    printf '  --with <caps>       comma-separated capabilities\n'
    printf '                       connectivity, bluetooth, usb, telemetry, ota, lan_control, storage, diagnostics\n'
    printf '  --policy <items>    comma-separated non-capability policy overlays\n'
    printf '                       low_power\n'
    printf '  --list              print interfaces, starter presets, capabilities, profiles, and policies\n'
    printf '\nIf path is given, creates the project there; otherwise uses the current directory.\n'
    printf 'Asks before overwriting existing files.\n'
    printf 'Interactive create uses a small preset menu; use --with for custom capability mixes.\n'
    printf '\nGenerates: top-level CMake, main/, host/, sdkconfig defaults, and schema-first blusys.project.yml.\n'
    printf 'ESP-IDF finds blusys_* via EXTRA_COMPONENT_DIRS (embedded path to components/) plus\n'
    printf 'main/idf_component.yml for registry deps (e.g. LVGL).\n'
    printf '\nEnvironment:\n'
    printf '  BLUSYS_SCAFFOLD_PLATFORM_VERSION   git ref for blusys_* deps (default: current branch)\n'
}

blusys_help_gen_spec() {
    printf 'Usage: blusys gen-spec [--manifest <blusys.project.yml>] [--output <build/generated/blusys_app_spec.h>]\n'
    printf '\nGenerate the manifest-derived app spec header used by app_main.cpp.\n'
    printf 'Defaults to ./blusys.project.yml and ./build/generated/blusys_app_spec.h when omitted.\n'
}

blusys_help_build() {
    printf 'Usage: blusys build [project] [esp32|esp32c3|esp32s3|host|qemu32|qemu32c3|qemu32s3]\n'
    printf '\n  esp32*     firmware in build-esp32*\n'
    printf '  host       SDL binary in build-host (same as host-build)\n'
    printf '  qemu32*    firmware in build-qemu* (optional sdkconfig.qemu for QEMU-only CONFIG)\n'
    printf '\nDefaults to cwd. Full matrix: docs/app/cli-host-qemu.md\n'
}

blusys_help_flash() {
    printf 'Usage: blusys flash [project] [port] [esp32|esp32c3|esp32s3]\n'
    printf '\nDefaults to the current directory if no project is specified.\n'
}

blusys_help_monitor() {
    printf 'Usage: blusys monitor [project] [port] [esp32|esp32c3|esp32s3]\n'
    printf '\nDefaults to the current directory if no project is specified.\n'
}

blusys_help_run() {
    printf 'Usage: blusys run [project] [port] [esp32|esp32c3|esp32s3]\n'
    printf '\nBuild, flash, and monitor. Defaults to the current directory.\n'
}

blusys_help_config() {
    printf 'Usage: blusys config [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nDefaults to the current directory if no project is specified.\n'
}

blusys_help_clean() {
    printf 'Usage: blusys clean [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nDefaults to the current directory if no project is specified.\n'
}

blusys_help_size() {
    printf 'Usage: blusys size [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nShow firmware size breakdown. Build the project first.\n'
}

blusys_help_erase() {
    printf 'Usage: blusys erase [port] [esp32|esp32c3|esp32s3]\n'
    printf '\nErase the entire flash of the connected device.\n'
    printf 'No project directory is required — erase is a device-only operation.\n'
    printf 'Port and target are both optional; port is auto-detected when only one\n'
    printf 'USB serial device is connected.\n'
    printf '\nExamples:\n'
    printf '  blusys erase\n'
    printf '  blusys erase esp32c3\n'
    printf '  blusys erase /dev/ttyACM0 esp32c3\n'
}

blusys_help_menuconfig() {
    printf 'Usage: blusys menuconfig [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nAlias for config.\n'
}

blusys_help_fullclean() {
    printf 'Usage: blusys fullclean [project]\n'
    printf '\nRemove build directories for all targets (esp32, esp32c3, esp32s3, host, qemu32, qemu32c3, qemu32s3).\n'
}

blusys_help_example() {
    printf 'Usage: blusys example [name] [command] [args...]\n'
    printf '\nRun a command on a bundled example.\n'
    printf '  blusys example                           list available examples\n'
    printf '  blusys example validation/smoke build esp32s3\n'
    printf '  blusys example validation/smoke run /dev/ttyACM0 esp32s3\n'
    printf '  blusys example validation/smoke qemu esp32\n'
    printf '\nIf no command is given, defaults to build.\n'
}

blusys_help_build_examples() {
    printf 'Usage: blusys build-examples\n'
    printf '\nBuilds every example for every target (esp32, esp32c3, esp32s3).\n'
    printf 'For inventory-filtered builds, use: blusys build-inventory <target> <ci_pr|ci_nightly>\n'
}

blusys_help_build_inventory() {
    printf 'Usage: blusys build-inventory <target> <ci_pr|ci_nightly>\n'
    printf '\nBuild examples selected from inventory.yml by the given CI flag.\n'
    printf 'Examples: blusys build-inventory esp32s3 ci_pr\n'
    printf '          blusys build-inventory esp32 ci_nightly\n'
}

blusys_help_lint() {
    printf 'Usage: blusys lint\n'
    printf '\nRun platform lint checks.\n'
}

blusys_help_validate() {
    printf 'Usage: blusys validate [path ...]\n'
    printf '\nValidate blusys.project.yml manifests with the schema contract.\n'
    printf 'With no path and no local manifest, runs inventory, product-layout, lint, and repository manifest scans in one pass.\n'
}

blusys_help_host_build() {
    printf 'Usage: blusys host-build [project]\n'
    printf '\nBuild a blusys product for the PC host (no hardware needed).\n'
    printf '\n  project   optional path to a blusys project created with\n'
    printf '            "blusys create". If omitted, uses the current directory\n'
    printf '            when it contains a host/ subdirectory, otherwise falls\n'
    printf '            back to the monorepo scripts/host/ harness.\n'
    printf '\nRuntime shape depends on the scaffolded interface:\n'
    printf '  interactive (SDL2)  same blusys::app UI as the device build\n'
    printf '  headless              terminal / headless harness; reducer runtime without display\n'
    printf '\nRequires: cmake, pkg-config.\n'
    printf 'Interactive apps additionally require libsdl2-dev (or equivalent).\n'
    printf 'See scripts/host/README.md for install instructions.\n'
}

blusys_help_version() {
    printf 'Usage: blusys version\n'
    printf '\nPrint blusys version, detected ESP-IDF version, and paths.\n'
}

blusys_help_update() {
    printf 'Usage: blusys update\n'
    printf '\nSelf-update blusys via git pull from the installation directory.\n'
}

blusys_help_config_idf() {
    printf 'Usage: blusys config-idf\n'
    printf '\nSelect the default ESP-IDF version from detected installations.\n'
    printf 'Saved to %s\n' "${XDG_CONFIG_HOME:-\$HOME/.config}/blusys/config"
}

blusys_help_qemu() {
    printf 'Usage: blusys qemu [--graphics|--serial-only] [project] [chip|qemu32|qemu32c3|qemu32s3]\n'
    printf '\nBuilds the project, then runs QEMU.\n'
    printf '  (default)      merge_bin + qemu-system (-nographic) — logs / CI-style UART (Ctrl+A, X)\n'
    printf '  --graphics     idf.py qemu --graphics monitor — SDL framebuffer (esp32s3/esp32c3; needs matching HAL + sdkconfig.qemu)\n'
    printf '  --serial-only  idf.py qemu monitor — no SDL window\n'
    printf 'qemu* uses the same build dir as `blusys build … qemu*`. See docs/app/cli-host-qemu.md\n'
}

blusys_help_install_qemu() {
    printf 'Usage: blusys install-qemu\n'
    printf '\nDownload pre-built Espressif QEMU from GitHub releases.\n'
    printf 'Installs to ~/.espressif/tools/qemu-<version>/\n'
}
