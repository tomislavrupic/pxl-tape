#!/bin/zsh
cd -- "${0:A:h}"
for pxl_python in /opt/homebrew/bin/python3 /usr/local/bin/python3 /usr/bin/python3; do
 if [[ -x "$pxl_python" ]] && "$pxl_python" -c 'import sys; assert sys.version_info >= (3,11)' 2>/dev/null; then
  "$pxl_python" tools/zogg_bridge.py
  pxl_status=$?
  if (( pxl_status != 0 )); then
   echo 'Zogg bridge did not start. If it is already running, use its existing window. Otherwise check the Zogg MCP configuration above.'
   read -k 1 '?Press a key to close.'
  fi
  exit $pxl_status
 fi
done
echo 'PXL Tape needs Python3.11 or newer and your existing Zogg MCP connection.'
echo 'Install Python from python.org, then open this launcher again.'
read -k 1 '?Press a key to close.'
