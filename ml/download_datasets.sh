#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
data_dir="${DATASETS_DIR:-$root_dir/reference/datasets}"

figshare_base_url=https://ndownloader.figshare.com/files

usage() {
  cat <<'EOF'
Usage: ml/download_datasets.sh [all|beijing|jiaxing]

Downloads the retained research datasets into reference/datasets/ and verifies
each file with the checksum published by its repository.
EOF
}

verify_md5() {
  local checksum="$1"
  local target="$2"

  printf '%s  %s\n' "$checksum" "$target" | md5sum -c - >/dev/null
}

download_figshare_file() {
  local file_id="$1"
  local target="$2"
  local checksum="$3"
  local url="$figshare_base_url/$file_id"
  local temp_target="${target}.part"

  mkdir -p "$(dirname "$target")"

  if [[ -f "$target" ]] && verify_md5 "$checksum" "$target"; then
    printf 'skip  %s\n' "$target"
    return
  fi

  printf 'get   %s\n' "$target"
  rm -f "$temp_target"
  curl --fail --location --retry 8 --retry-delay 5 \
    --connect-timeout 20 --output "$temp_target" "$url"

  if ! verify_md5 "$checksum" "$temp_target"; then
    printf 'checksum mismatch: %s\n' "$target" >&2
    rm -f "$temp_target"
    return 1
  fi

  mv "$temp_target" "$target"
  printf 'ok    %s\n' "$target"
}

extract_figshare_zip() {
  local file_id="$1"
  local archive_name="$2"
  local checksum="$3"
  local output_dir="$4"
  local expected_file="$5"
  local archive="$output_dir/$archive_name"

  if [[ -f "$output_dir/$expected_file" ]]; then
    printf 'skip  %s\n' "$output_dir/$expected_file"
    return
  fi

  download_figshare_file "$file_id" "$archive" "$checksum"
  python3 -m zipfile -e "$archive" "$output_dir"
  rm "$archive"
  printf 'ok    %s\n' "$output_dir/$expected_file"
}

download_beijing() {
  local dir="$data_dir/beijing_2026"

  download_figshare_file 63526980 "$dir/code.ipynb" \
    2374b6348675f71b3f9e3857eee5dafd
  download_figshare_file 65342043 "$dir/stations_public.parquet" \
    449a2adaa391895d4c29944a6e400794
  download_figshare_file 65342046 "$dir/orders_2025-01_public.parquet" \
    c66ee6b6149fba215bc6b88667af2d18
  download_figshare_file 65342049 "$dir/orders_2025-07_public.parquet" \
    64a4e6df4e5dbcc06040e650737f7b0a
}

download_jiaxing() {
  local dir="$data_dir/jiaxing_2025"

  extract_figshare_zip 52886138 Dataset.zip \
    d2f43f19a3cfd5d364f97225e5f5d3ab "$dir" \
    Dataset/Charging_Data.csv
  extract_figshare_zip 52886141 Code.zip \
    b95c6a24a1c5e91a51efea1043c6ebce "$dir" \
    Code/failure_analysis.py
}

target="${1:-all}"
case "$target" in
  all)
    download_beijing
    download_jiaxing
    ;;
  beijing)
    download_beijing
    ;;
  jiaxing)
    download_jiaxing
    ;;
  -h|--help)
    usage
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac
