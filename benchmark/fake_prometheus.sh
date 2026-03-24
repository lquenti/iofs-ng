#!/bin/bash
# Simulates Prometheus scraping iofs-ng every 1 second

echo "Starting background poller targeting http://127.0.0.1:9090/metrics..."
echo "Press [CTRL+C] to stop."

while true; do
    # Fetch metrics and discard output silently
    curl -s http://127.0.0.1:9090/metrics > /dev/null

    # Check if curl failed (e.g. iofs-ng is not running yet)
    if [ $? -ne 0 ]; then
        # Silent fail, just wait and try again
        echo "BAD BAD BAD BAD BAD"
        sleep 1
        continue
    fi
    echo "worked"

    sleep 10
done
