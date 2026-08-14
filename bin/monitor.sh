#!/bin/bash
# monitor.sh - Serial monitor tool with auto-reconnect loop for XIAO ESP32C3

PORT="${1:-/dev/ttyACM1}"

## I use flatpack to run arduino, therefore this exotic path
CLI_EXE="/app/arduino-ide/resources/app/lib/backend/resources/arduino-cli"

echo "======================================================="
echo "Attaching to serial monitor on port $PORT..."
echo "======================================================="
echo "Automatically re-connecting when port is lost (deep sleep/reset)."
echo "Press Ctrl+C to exit."
echo ""

# Set up a trap to exit the loop cleanly on Ctrl+C (SIGINT)
trap "echo -e '\nExiting serial monitor...'; exit 0" SIGINT

while true; do
    # Wait for the serial port device to become available
    if [ ! -e "$PORT" ]; then
        echo -n "Waiting for port $PORT to appear..."
        while [ ! -e "$PORT" ]; do
            sleep 0.5
            echo -n "."
        done
        echo " detected!"
        sleep 0.5 # Give the OS a moment to initialize the CDC port
    fi
    
    # Run arduino-cli monitor in Flatpak sandbox
    flatpak run --command=sh cc.arduino.IDE2 -c "$CLI_EXE monitor -p $PORT --config baudrate=115200"
    
    echo ""
    echo "Connection lost. Re-attaching as soon as $PORT becomes available..."
    sleep 1
done
