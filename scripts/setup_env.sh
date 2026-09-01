#!/usr/bin/env bash
set -euo pipefail

echo "==> 更新软件源"
sudo apt update

echo "==> 安装编译工具链 + Qt5 + QtWebEngine + SQLite"
sudo apt install -y \
  build-essential \
  cmake \
  clangd \
  qtbase5-dev \
  qtwebengine5-dev \
  qttools5-dev-tools \
  libqt5sql5-sqlite

echo "==> 安装 Node.js latest LTS"
curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
sudo apt install -y nodejs

echo "==> 安装 uv (Python)"
curl -LsSf https://astral.sh/uv/install.sh | sh
