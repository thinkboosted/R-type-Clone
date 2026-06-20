#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PORT=1234
DURATION_SEC=10
SIMULATE_LAG_MS="${SIMULATE_LAG_MS:-300}"
PRINT_BANDWIDTH="${PRINT_BANDWIDTH:-1}"

SERVER_BIN="${BUILD_DIR}/r-type_server"
CLIENT_BIN="${BUILD_DIR}/r-type_client"
SERVER_LOG="${ROOT_DIR}/server.log"
CLIENT1_LOG="${ROOT_DIR}/client1.log"
CLIENT2_LOG="${ROOT_DIR}/client2.log"

if [[ ! -x "${SERVER_BIN}" || ! -x "${CLIENT_BIN}" ]]; then
  echo "[test_network] Missing binaries in ${BUILD_DIR}. Run: python3 build.py"
  exit 1
fi

if command -v ss >/dev/null 2>&1; then
  if ss -lun | grep -q ":${PORT} "; then
    echo "[test_network] Port ${PORT} is already in use. Stop existing server/client processes first."
    exit 1
  fi
fi

export LD_LIBRARY_PATH="${BUILD_DIR}:${BUILD_DIR}/lib:${LD_LIBRARY_PATH:-}"

cleanup() {
  set +e
  if [[ -n "${CLIENT1_PID:-}" ]]; then kill "${CLIENT1_PID}" 2>/dev/null || true; fi
  if [[ -n "${CLIENT2_PID:-}" ]]; then kill "${CLIENT2_PID}" 2>/dev/null || true; fi
  if [[ -n "${SERVER_PID:-}" ]]; then kill "${SERVER_PID}" 2>/dev/null || true; fi
}
trap cleanup EXIT INT TERM

run_client() {
  local log_file="$1"
  local bw_flag=()
  if [[ "${PRINT_BANDWIDTH}" == "1" ]]; then
    bw_flag=("--print-bandwidth")
  fi
  if command -v xvfb-run >/dev/null 2>&1; then
    xvfb-run -a "${CLIENT_BIN}" 127.0.0.1 "${PORT}" "--simulate-lag=${SIMULATE_LAG_MS}" "${bw_flag[@]}" >"${log_file}" 2>&1 &
  else
    "${CLIENT_BIN}" 127.0.0.1 "${PORT}" "--simulate-lag=${SIMULATE_LAG_MS}" "${bw_flag[@]}" >"${log_file}" 2>&1 &
  fi
  echo $!
}

: >"${SERVER_LOG}"
: >"${CLIENT1_LOG}"
: >"${CLIENT2_LOG}"

server_bw_flag=()
if [[ "${PRINT_BANDWIDTH}" == "1" ]]; then
  server_bw_flag=("--print-bandwidth")
fi

"${SERVER_BIN}" "${PORT}" "--simulate-lag=${SIMULATE_LAG_MS}" "${server_bw_flag[@]}" >"${SERVER_LOG}" 2>&1 &
SERVER_PID=$!

echo "[test_network] Server started (pid=${SERVER_PID})"
sleep 2

CLIENT1_PID="$(run_client "${CLIENT1_LOG}")"
echo "[test_network] Client #1 started (pid=${CLIENT1_PID})"
sleep 2

CLIENT2_PID="$(run_client "${CLIENT2_LOG}")"
echo "[test_network] Client #2 started (pid=${CLIENT2_PID})"

echo "[test_network] Running for ${DURATION_SEC}s with simulate-lag=${SIMULATE_LAG_MS}ms (print-bandwidth=${PRINT_BANDWIDTH})"
sleep "${DURATION_SEC}"

echo "[test_network] Done. Logs:"
echo "  ${SERVER_LOG}"
echo "  ${CLIENT1_LOG}"
echo "  ${CLIENT2_LOG}"
