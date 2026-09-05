#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p certs

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout certs/mallory.key \
    -out certs/mallory.crt \
    -days 365 \
    -subj "/CN=leaky-server" \
    -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"

echo
printf 'Generated:\n'
printf '  %s\n' "$(pwd)/certs/mallory.key"
printf '  %s\n' "$(pwd)/certs/mallory.crt"
