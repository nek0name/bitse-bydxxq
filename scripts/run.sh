#!/usr/bin/env bash
set -euo pipefail
# shellcheck source-path=SCRIPTDIR
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
target="${1:-server}"
if (( $# > 0 )); then shift; fi
case "$target" in
  server)
    exec "$build_dir/apps/server/charging-server" --source-root "$root_dir" --web-root "$root_dir/web/dist" "$@"
    ;;
  user)
    exec "$build_dir/apps/user-app/charging-user" "$@"
    ;;
  admin)
    exec "$build_dir/apps/admin-app/charging-admin" "$@"
    ;;
  *)
    echo 'Usage: scripts/run.sh [server|user|admin] [arguments]' >&2
    exit 2
    ;;
esac
