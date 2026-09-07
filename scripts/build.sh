#!/usr/bin/env bash
set -euo pipefail
# shellcheck source-path=SCRIPTDIR
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
cd "$root_dir"
for command_name in cmake pnpm uv; do
  command -v "$command_name" >/dev/null || { echo "Missing $command_name; run scripts/setup_env.sh." >&2; exit 1; }
done

# Install Python dependencies before CMake registers their tests.
pnpm --dir "$root_dir/web" install --frozen-lockfile
uv sync --project "$root_dir/ml" --frozen
args=(--preset full -B "$build_dir" -DCMAKE_BUILD_TYPE="${CHARGING_BUILD_TYPE:-RelWithDebInfo}")
if [[ -n "${CHARGING_QT_PREFIX:-}" ]]; then
  args+=(-DQT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES=ON)
  for module in "$CHARGING_QT_LIB_DIR"/cmake/Qt6*; do
    [[ -d "$module" ]] && args+=("-D$(basename "$module")_DIR=$module")
  done
fi
cmake "${args[@]}" "$@"
cmake --build "$build_dir" --parallel "${CHARGING_BUILD_JOBS:-4}"
pnpm --dir "$root_dir/web" build

# Qt WebEngine loads paths from qt.conf in each application and its helper.
if [[ -n "${CHARGING_QT_PREFIX:-}" && -x "$CHARGING_QT_PREFIX/lib/qt6/libexec/QtWebEngineProcess" ]]; then
  plugin_dir="$(qmake6 -query QT_INSTALL_PLUGINS)"
  for config_dir in "$build_dir/apps/user-app" "$build_dir/apps/admin-app" "$build_dir/apps/server" "$CHARGING_QT_PREFIX/lib/qt6/libexec"; do
    [[ -d "$config_dir" ]] || continue
    cat > "$config_dir/qt.conf" <<CONFIG
[Paths]
Prefix=$CHARGING_QT_PREFIX
Data=share/qt6
Translations=share/qt6/translations
LibraryExecutables=lib/qt6/libexec
Plugins=$plugin_dir
Qml2Imports=$CHARGING_QT_QML_DIR
CONFIG
  done
fi
printf 'Build ready: %s\n' "$build_dir"
