#!/usr/bin/env bash
set -euo pipefail
# shellcheck source-path=SCRIPTDIR
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
cd "$root_dir"
if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
  echo 'No configured build. Run scripts/build.sh before scripts/test.sh.' >&2
  exit 1
fi
"$root_dir/scripts/format-cpp.sh" --check
uv run --frozen --project "$root_dir/ml" ruff check "$root_dir/ml" "$root_dir/tests" "$root_dir/web/tests" "$root_dir/web/scripts/export_dashboard.py"
uv run --frozen --project "$root_dir/ml" ruff format --check "$root_dir/ml" "$root_dir/tests" "$root_dir/web/tests" "$root_dir/web/scripts/export_dashboard.py"
pnpm --dir "$root_dir/web" lint
pnpm --dir "$root_dir/web" format:check
shellcheck -x scripts/*.sh ml/download_datasets.sh
# Require all suites even if CMake skipped an optional dependency.
ctest --test-dir "$build_dir" --show-only=json-v1 | python3 -c '
import json, sys
registered = {test["name"] for test in json.load(sys.stdin)["tests"]}
required = {"service-integration", "forecast-unit", "admin-ui", "mobile-flow", "mobile-qml-smoke"}
missing = required - registered
if missing:
  sys.exit("Missing CTest entries; rerun scripts/build.sh: " + ", ".join(sorted(missing)))
print("Registered tests: " + ", ".join(sorted(registered)))
'
export UV_FROZEN=1
ctest --test-dir "$build_dir" --output-on-failure --no-tests=error --parallel "${CHARGING_TEST_JOBS:-1}" "$@"
