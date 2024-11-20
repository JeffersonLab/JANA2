#!/bin/bash

# Default named pipe path
DEFAULT_PIPE="/tmp/jana_status"

# Check for required arguments
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <PID> [jana_status_pipe]"
    exit 1
fi

# Assign variables
PID="$1"
PIPE="${2:-$DEFAULT_PIPE}" # Use the second argument if provided, otherwise use the default

# Validate the PID
if ! kill -0 "$PID" 2>/dev/null; then
    echo "Error: Process with PID $PID does not exist."
    exit 1
fi

# Check if the pipe exists
if [[ ! -p "$PIPE" ]]; then
    echo "Error: Named pipe '$PIPE' does not exist."
    exit 1
fi

# Send the USR1 signal to the process
if ! kill -USR1 "$PID"; then
    echo "Error: Failed to send SIGUSR1 to PID $PID."
    exit 1
fi

# Cat the named pipe
cat "$PIPE"
