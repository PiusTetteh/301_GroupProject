#!/bin/bash

# Multikernel OS - Complete Build and Test Script

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════╗"
echo "║   MULTIKERNEL OS - BUILD AND TEST SCRIPT              ║"
echo "║   Group 54 - University of Ghana                      ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Build C++ components
echo "Step 1: Building C++ components..."
echo "-----------------------------------"
cmake .
make
echo "✓ Build complete"
echo ""

# Step 2: Run tests
echo "Step 2: Running test suite..."
echo "-----------------------------------"
timeout 10 ./multikernel_test 2>&1 | head -100
echo "✓ Tests complete"
echo ""

# Step 3: Check Python dependencies
echo "Step 3: Checking Python dependencies..."
echo "-----------------------------------"
if command -v python3 &> /dev/null; then
    echo "Python3 found: $(python3 --version)"
    
    # Check if Flask is installed
    if python3 -c "import flask" &> /dev/null; then
        echo "✓ Flask installed"
    else
        echo "⚠ Flask not installed"
        echo "  Install with: pip3 install -r requirements.txt"
    fi
else
    echo "⚠ Python3 not found"
fi
echo ""

echo "╔════════════════════════════════════════════════════════╗"
echo "║   BUILD COMPLETE - READY FOR DEMONSTRATION            ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""
echo "To run the system:"
echo ""
echo "  CLI Demo:       ./multikernel_os"
echo "  Test Suite:     ./multikernel_test"
echo "  Web Interface:  python3 web_server.py"
echo ""
echo "For web interface, then open: http://localhost:5000"
echo ""
