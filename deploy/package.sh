#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p release
docker build --target backend -t erp-backend:2026-11-01-r3 .
docker build --target frontend -t erp-frontend:2026-11-01-r3 .
docker save -o release/erp-images.tar erp-backend:2026-11-01-r3 erp-frontend:2026-11-01-r3
gzip -f release/erp-images.tar
tar -czf release/erp-deploy.tar.gz compose.yaml .env.example deploy/README.md db
printf '%s\n' 'Created release/erp-images.tar.gz and release/erp-deploy.tar.gz'
