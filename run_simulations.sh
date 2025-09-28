#!/bin/bash
echo "TradeKernel OS Simulation"
echo "========================"
echo ""
echo "Building simulations..."
g++ -std=c++17 -O3 -I./include -o tradekernel_sim test_simulation.cpp 2>/dev/null
g++ -std=c++17 -O3 -I./include -o mock_trading_system mock_trading_system.cpp 2>/dev/null
echo ""
echo "Running core simulation:"
./tradekernel_sim
echo ""
echo "Running trading system:"
./mock_trading_system
