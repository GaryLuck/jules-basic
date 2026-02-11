#!/bin/bash
# Reproduce the infinite loop issue with invalid string syntax
# Note: Using single quotes 'hello' is invalid in this interpreter, but should produce an error, not an infinite loop.
# This script requires `basic_interpreter` to be in the parent directory (or build directory).

INTERPRETER=../basic_interpreter
if [ ! -f "$INTERPRETER" ]; then
    echo "Interpreter not found at $INTERPRETER"
    # Try current dir if run from root
    if [ -f "./basic_interpreter" ]; then
        INTERPRETER=./basic_interpreter
    else
        exit 1
    fi
fi

timeout 5s $INTERPRETER <<'EOF'
10 print left$('hello', 2)
RUN
EOF

EXIT_CODE=$?
if [ $EXIT_CODE -eq 124 ]; then
    echo "FAILED: Interpreter timed out (likely infinite loop)"
    exit 1
else
    echo "PASSED: Interpreter exited normally"
    exit 0
fi
