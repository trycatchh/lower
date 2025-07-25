#!/bin/bash

echo "=== Server Debug Test ==="

echo "1. Directory Struct:"
find ./public -type f 2>/dev/null | head -10

echo "2. Server starting..."
./build/lwserver -d -v &
SERVER_PID=$!

sleep 3

echo "3. Test Requests:"

echo "--- Home Page File Test ---"
curl -v http://localhost:8080/ 2>&1 | head -20

echo "--- CSS File Test ---"
curl -v http://localhost:8080/css/style.css 2>&1 | head -20

echo "--- JS File Test ---"
curl -v http://localhost:8080/js/script.js 2>&1 | head -20

kill $SERVER_PID 2>/dev/null

echo "=== Test Compulated ==="
