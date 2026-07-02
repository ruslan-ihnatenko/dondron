#!/usr/bin/env bash
# Line 1 public-repo policy checks (fast grep gate for CI and agents).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WS_SRC="${ROOT}/ros2_ws/src"
FAIL=0

err() {
  echo "BOUNDARY FAIL: $*" >&2
  FAIL=1
}

echo "[*] DonDron public boundary check (root: ${ROOT})"

# flight_api must not subscribe to /detections (source only)
if grep -rE 'create_subscription.*"/detections"|subscribe.*"/detections"' \
  "${WS_SRC}/dondron_flight_api" \
  --include='*.cpp' --include='*.hpp' 2>/dev/null; then
  err "dondron_flight_api must not subscribe to /detections"
fi

# No private guidance package imports in executable source
if grep -rE '(import|from|find_package|IncludeLaunchDescription).*(dondron_guidance_private|dondron-guidance-private)' \
  "${WS_SRC}" \
  --include='*.py' --include='*.cpp' --include='*.hpp' --include='*.xml' \
  --include='CMakeLists.txt' --include='package.xml' 2>/dev/null; then
  err "private guidance package import/reference found in source"
fi

# mission.xml: no ENGAGE BT node (public Line 1 ends at TRACK)
if [[ -f "${WS_SRC}/dondron_state_machine/behavior_trees/mission.xml" ]]; then
  if grep -E '<(Action|Condition)[^>]*ID="(Engage|ENGAGE)' \
    "${WS_SRC}/dondron_state_machine/behavior_trees/mission.xml" 2>/dev/null; then
    err "ENGAGE actions are not allowed in public mission.xml"
  fi
fi

# No private repo URLs in CI/config (not policy docs)
if grep -rE 'github\.com/[^/]+/dondron-guidance-private|dondron_guidance_private\.git' \
  "${ROOT}/.github" "${ROOT}/ros2_ws" \
  --include='*.yaml' --include='*.yml' --include='CMakeLists.txt' --include='package.xml' \
  2>/dev/null; then
  err "private repo URL found in build/CI config"
fi

# Python launch syntax
while IFS= read -r -d '' f; do
  if ! python3 -m py_compile "${f}" 2>/dev/null; then
    err "launch file syntax error: ${f}"
  fi
done < <(find "${WS_SRC}" -path '*/launch/*.py' -print0 2>/dev/null)

if [[ "${FAIL}" -ne 0 ]]; then
  echo "[!] Public boundary check failed." >&2
  exit 1
fi

echo "[+] Public boundary check passed."
