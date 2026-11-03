#!/bin/sh
set -eu

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build python3 python-is-python3 \
    qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools \
    qt6-translations-l10n libkf6archive-dev libicu-dev \
    libboost-dev libssl-dev libcurl4-openssl-dev ragel zlib1g-dev \
    git pkg-config ca-certificates file xz-utils
rm -rf /var/lib/apt/lists/*
