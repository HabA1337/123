#!/bin/sh
cd "$(dirname "$0")"
echo "КОНТУР демо-стенд: http://127.0.0.1:8765/"
echo "ТСД:              http://127.0.0.1:8765/tsd.html"
exec python3 -m http.server 8765
