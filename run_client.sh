#!/bin/bash
echo "=== Binary Info ==="
echo "Path: $(pwd)/bin/battleship_client"
echo "Size: $(ls -lh bin/battleship_client | awk '{print $5}')"
echo "MD5: $(md5sum bin/battleship_client | awk '{print $1}')"
echo "Modified: $(stat -c '%y' bin/battleship_client)"
echo "==================="
echo ""
exec ./bin/battleship_client "$@"
