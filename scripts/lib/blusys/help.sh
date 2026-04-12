# Sourced by repo-root `blusys` — help text for all subcommands.

blusys_help() {
    cat <<'HELP'
Usage: blusys <command> [args...]

All commands default to the current directory when no project is specified.

Commands:
  create [path]    Scaffold a new blusys project (default: cwd)
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
  build-examples   Build all examples for all targets
  host-build       Build the PC + SDL2 host harness (scripts/host/)
  qemu             Build and run project in QEMU emulator
  install-qemu     Download and install Espressif QEMU
  config-idf       Select default ESP-IDF version
  version          Show blusys and ESP-IDF version info
  update           Self-update blusys via git pull
  help             Show this help message

Run 'blusys help <command>' for command-specific help.
HELP
}

blusys_list_archetypes() {
    printf 'Canonical archetypes (primary selector for `blusys create`):\n\n'
    printf '  interactive-controller   Interactive, compact expressive UI; encoder-first; ST7735 family.\n'
    printf '  interactive-panel        Interactive operational density; dashboard-led; ILI9341/ILI9488 family.\n'
    printf '  edge-node                Headless-first connected device; telemetry, OTA, provisioning.\n'
    printf '  gateway-controller       Headless coordinator by default; optional local UI via --starter interactive.\n'
    printf '\nUsage: blusys create --archetype <name> [path]\n'
    printf 'Default when run with no options: --archetype interactive-controller\n\n'
    printf 'Legacy --starter (compatibility only): maps runtime mode to an archetype when --archetype is omitted:\n'
    printf '  --starter interactive  -> interactive-controller\n'
    printf '  --starter headless     -> edge-node\n'
    printf '(Prefer naming the archetype explicitly.)\n\n'
}

blusys_help_create() {
    printf 'Usage: blusys create [--archetype <name>] [--starter <headless|interactive>] [--list-archetypes] [path]\n'
    printf '\nScaffold a new blusys product project. The archetype picks starter content and defaults;\n'
    printf 'the directory layout stays the same for every archetype (main/core, main/integration, main/ui when used).\n'
    printf '\nOptions:\n'
    printf '  --archetype <type>  product family starter (default: interactive-controller)\n'
    printf '                       interactive-controller — compact expressive interactive app\n'
    printf '                       interactive-panel      — operational interactive panel\n'
    printf '                       edge-node               — headless-first connected app\n'
    printf '                       gateway-controller      — coordinator / optional local operator UI\n'
    printf '  --list-archetypes   print the four archetypes and exit\n'
    printf '  --starter <mode>    legacy alias when --archetype is omitted (interactive|headless)\n'
    printf '\nIf path is given, creates the project there; otherwise uses the current directory.\n'
    printf 'Asks before overwriting existing files.\n'
    printf '\nGenerates: top-level CMake + main/ with core/, integration/, and ui/ when the archetype ships UI.\n'
    printf 'Platform components are pulled via main/idf_component.yml.\n'
    printf '\nEnvironment:\n'
    printf '  BLUSYS_SCAFFOLD_PLATFORM_VERSION   git ref for blusys_* deps (default: current branch)\n'
}

blusys_help_build() {
    printf 'Usage: blusys build [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nDefaults to the current directory if no project is specified.\n'
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
    printf 'Usage: blusys erase [project] [port] [esp32|esp32c3|esp32s3]\n'
    printf '\nErase the entire flash of the connected device.\n'
}

blusys_help_menuconfig() {
    printf 'Usage: blusys menuconfig [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nAlias for config.\n'
}

blusys_help_fullclean() {
    printf 'Usage: blusys fullclean [project]\n'
    printf '\nRemove build directories for all targets (esp32, esp32c3, esp32s3).\n'
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
}

blusys_help_lint() {
    printf 'Usage: blusys lint\n'
    printf '\nRun platform lint checks.\n'
}

blusys_help_host_build() {
    printf 'Usage: blusys host-build [project]\n'
    printf '\nBuild a blusys product for the PC host (no hardware needed).\n'
    printf '\n  project   optional path to a blusys project created with\n'
    printf '            "blusys create". If omitted, uses the current directory\n'
    printf '            when it contains a host/ subdirectory, otherwise falls\n'
    printf '            back to the monorepo scripts/host/ harness.\n'
    printf '\nRuntime shape depends on the scaffolded archetype:\n'
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
    printf 'Usage: blusys qemu [project] [esp32|esp32c3|esp32s3]\n'
    printf '\nBuild and launch project in QEMU emulator. Defaults to cwd.\n'
    printf 'Networking enabled via OpenCores Ethernet (not WiFi).\n'
    printf 'Press Ctrl+A, X to exit.\n'
}

blusys_help_install_qemu() {
    printf 'Usage: blusys install-qemu\n'
    printf '\nDownload pre-built Espressif QEMU from GitHub releases.\n'
    printf 'Installs to ~/.espressif/tools/qemu-<version>/\n'
}
