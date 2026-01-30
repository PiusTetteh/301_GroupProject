#!/bin/bash

# Quick Demo Script - Shows Message Passing and SMP Comparison

echo "============================================"
echo "  Multikernel OS - Message Passing Demo"
echo "============================================"
echo ""
echo "This demo will show:"
echo "  1. Visible inter-core messages"
echo "  2. Message statistics"
echo "  3. SMP vs Multikernel comparison"
echo ""
echo "Press Enter to start..."
read

# Build if needed
if [ ! -f "./multikernel_os" ]; then
    echo "Building multikernel OS..."
    make
fi

echo ""
echo "Running demo (will take ~30 seconds)..."
echo ""

# Run and capture output
timeout 40 ./multikernel_os 2>&1 | tee /tmp/mk_demo_output.txt

echo ""
echo "============================================"
echo "  Demo Complete!"
echo "============================================"
echo ""
echo "Key Results:"
echo ""

# Extract and show message counts
echo "Messages Sent/Received:"
grep "Total Messages" /tmp/mk_demo_output.txt | head -6

echo ""
echo "Sample Messages:"
grep "^\[MSG\]" /tmp/mk_demo_output.txt | head -10

echo ""
echo "SMP System Overhead:"
grep -A 3 "SMP SYSTEM STATISTICS" /tmp/mk_demo_output.txt | head -7

echo ""
echo "Full output saved to: /tmp/mk_demo_output.txt"
echo ""
