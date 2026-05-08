#!/bin/sh

# Configuration
DAEMON="./build/src/daemon/nanshe-d"
CLIENT="./build/src/client/nanshe-cli"
MODEL="models/gemma-3-4b-it-Q4_K_M.gguf"
SOCKET="/tmp/nanshe.sock"

# 1. Cleanup old socket
if [ -S "$SOCKET" ]; then
    rm "$SOCKET"
fi

# 2. Start the daemon in the background
echo "[Launcher] Starting Nanshe daemon..."
$DAEMON "$MODEL" &
DAEMON_PID=$!

# 3. Wait for the socket to appear
# Loading the 2.3GB model takes a moment. We wait for the socket
# to exist before firing the client.
echo "[Launcher] Waiting for model to load and socket to initialize..."
while [ ! -S "$SOCKET" ]; do
    sleep 0.5
done

# 4. Run the client in the foreground
echo "[Launcher] Daemon ready. Launching client..."
$CLIENT

# 5. Optional: Kill the daemon after the client exits
# Comment this out if you want the daemon to stay resident.
#echo "[Launcher] Client finished. Shutting down daemon (PID: $DAEMON_PID)..."
#kill $DAEMON_PID
