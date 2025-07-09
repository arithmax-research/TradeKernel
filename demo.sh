#!/bin/bash

# TradeKernel Demo Script
# Comprehensive interactive demonstration of the ultra-low latency trading system

set -e  # Exit on error

# Color codes for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored headers
print_header() {
    echo -e "\n${CYAN}$1${NC}"
    echo -e "${CYAN}$(printf '=%.0s' $(seq 1 ${#1}))${NC}\n"
}

# Function to print success messages
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print warnings
print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Function to print info
print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Function to pause for user input
pause() {
    echo -e "\n${YELLOW}Press Enter to continue...${NC}"
    read -r
}

# Function to display main menu
show_menu() {
    clear
    echo -e "${PURPLE}🚀 TradeKernel v1.0 - Ultra-Low Latency Trading System${NC}"
    echo -e "${PURPLE}========================================================${NC}\n"
    
    echo -e "This demonstration showcases a complete high-frequency trading"
    echo -e "system with sub-microsecond latency execution.\n"
    
    echo -e "${CYAN}Demo Options:${NC}"
    echo -e "  ${GREEN}1)${NC} Project Overview & Architecture"
    echo -e "  ${GREEN}2)${NC} Core System Simulation"
    echo -e "  ${GREEN}3)${NC} Mock Trading System Demo"
    echo -e "  ${GREEN}4)${NC} Performance Benchmarks"
    echo -e "  ${GREEN}5)${NC} Full Trading Demo (All Components)"
    echo -e "  ${GREEN}6)${NC} Build Status & System Requirements"
    echo -e "  ${GREEN}7)${NC} Development & Deployment Guide"
    echo -e "  ${GREEN}0)${NC} Exit\n"
    
    echo -e "${YELLOW}Enter your choice [0-7]:${NC} "
}

# Function to show project overview
show_overview() {
    clear
    print_header "📁 TradeKernel Project Architecture"
    
    echo "TradeKernel/"
    echo "├── boot/"
    echo "│   └── boot.asm              # BIOS bootloader with 64-bit mode transition"
    echo "├── src/"
    echo "│   ├── kernel/"
    echo "│   │   ├── entry.asm         # 64-bit kernel entry and context switching"
    echo "│   │   └── main.cpp          # Kernel initialization and main loop"
    echo "│   ├── memory/"
    echo "│   │   └── memory_manager.cpp  # NUMA-aware lock-free allocators"
    echo "│   ├── networking/"
    echo "│   │   └── [network drivers] # Ultra-low latency networking stack"
    echo "│   └── scheduler/"
    echo "│       └── tickless_scheduler.cpp  # Priority-based task scheduler"
    echo "├── include/"
    echo "│   ├── types.h               # Core types and performance macros"
    echo "│   ├── memory.h              # Memory management interfaces"
    echo "│   ├── scheduler.h           # Scheduler interfaces"
    echo "│   └── networking.h          # Ultra-low latency networking"
    echo "├── linker.ld                 # Optimized memory layout"
    echo "├── Makefile                  # Full kernel build system"
    echo "├── test_simulation.cpp       # Userspace simulation"
    echo "├── mock_trading_system.cpp   # Complete trading engine demo"
    echo "└── TRADING_DEMO.md          # Comprehensive documentation"
    
    print_header "⚡ Core Features Implemented"
    
    print_success "Bare-metal x86_64 bootloader"
    print_success "64-bit kernel with optimized assembly entry"
    print_success "Lock-free memory management with NUMA support"
    print_success "Tickless priority-based scheduler"
    print_success "Zero-copy network packet processing"
    print_success "Cache-aligned data structures"
    print_success "Hardware cycle counting (RDTSC/RDTSCP)"
    print_success "Sub-microsecond context switching design"
    print_success "Trading-specific task priorities"
    print_success "Complete mock trading engine"
    print_success "Order book management"
    print_success "Risk management system"
    print_success "Market data simulation"
    
    print_header "🚀 Performance Targets"
    
    echo "• Interrupt Latency: < 100ns"
    echo "• NIC-to-UserSpace: < 300ns"
    echo "• Context Switch: < 500ns"
    echo "• Memory Allocation: < 100ns (lock-free pools)"
    echo "• L1 Cache Access: 40ns"
    echo "• Order Processing: < 1μs"
    echo "• Market Data Processing: < 500ns"
    
    pause
}

# Function to run core simulation
run_simulation() {
    clear
    print_header "🧪 Core System Simulation"
    
    print_info "Running TradeKernel core system simulation..."
    
    if [ -f "tradekernel_sim" ]; then
        print_success "Found existing simulation binary"
    else
        print_info "Building simulation from source..."
        if clang++ -std=c++17 -O3 -Wall -Wextra -I./include -o tradekernel_sim test_simulation.cpp; then
            print_success "Simulation built successfully"
        else
            print_warning "Build failed, trying alternative compiler..."
            g++ -std=c++17 -O3 -Wall -Wextra -I./include -o tradekernel_sim test_simulation.cpp
        fi
    fi
    
    echo ""
    print_info "Executing core system tests..."
    ./tradekernel_sim
    
    pause
}

# Function to run mock trading system
run_trading_demo() {
    clear
    print_header "💹 Mock Trading System Demonstration"
    
    print_info "Building and running comprehensive trading system..."
    
    if make -f Makefile.simple trading-demo 2>/dev/null; then
        print_success "Trading system executed successfully"
    else
        print_info "Building trading system..."
        if clang++ -std=c++17 -O3 -Wall -Wextra -I./include -o mock_trading mock_trading_system.cpp; then
            print_success "Trading system built successfully"
            print_info "Running trading demo..."
            ./mock_trading
        else
            print_warning "Build failed, trying alternative compiler..."
            g++ -std=c++17 -O3 -Wall -Wextra -I./include -o mock_trading mock_trading_system.cpp
            ./mock_trading
        fi
    fi
    
    pause
}

# Function to run performance benchmarks
run_benchmarks() {
    clear
    print_header "⚡ Performance Benchmarking Suite"
    
    print_info "Running comprehensive performance tests..."
    
    if make -f Makefile.simple perf 2>/dev/null; then
        print_success "Performance benchmarks completed"
    else
        print_info "Running individual benchmarks..."
        
        # Run simulation benchmark
        if [ -f "tradekernel_sim" ]; then
            echo -e "\n${CYAN}Core System Performance:${NC}"
            ./tradekernel_sim
        fi
        
        # Run trading benchmark
        if [ -f "mock_trading" ]; then
            echo -e "\n${CYAN}Trading System Performance:${NC}"
            ./mock_trading
        fi
    fi
    
    pause
}

# Function to run full demo
run_full_demo() {
    clear
    print_header "🎯 Complete TradeKernel Demonstration"
    
    print_info "Running full system demonstration..."
    
    if make -f Makefile.simple full-demo 2>/dev/null; then
        print_success "Full demonstration completed successfully"
    else
        print_info "Running demonstration components individually..."
        
        echo -e "\n${CYAN}1. Core System Test:${NC}"
        run_simulation
        
        echo -e "\n${CYAN}2. Trading System Demo:${NC}"
        run_trading_demo
        
        echo -e "\n${CYAN}3. Performance Analysis:${NC}"
        run_benchmarks
    fi
    
    pause
}

# Function to show build status
show_build_status() {
    clear
    print_header "🔧 Build Status & System Requirements"
    
    print_info "Checking system requirements..."
    
    # Check for required tools
    echo -e "\n${CYAN}Required Development Tools:${NC}"
    
    if command -v clang++ &> /dev/null; then
        print_success "clang++ compiler available"
    elif command -v g++ &> /dev/null; then
        print_success "g++ compiler available"
    else
        print_warning "No C++ compiler found - install clang++ or g++"
    fi
    
    if command -v nasm &> /dev/null; then
        print_success "NASM assembler available"
    else
        print_warning "NASM not found - install for kernel builds (brew install nasm)"
    fi
    
    if command -v qemu-system-x86_64 &> /dev/null; then
        print_success "QEMU emulator available"
    else
        print_warning "QEMU not found - install for kernel testing (brew install qemu)"
    fi
    
    if command -v make &> /dev/null; then
        print_success "GNU Make available"
    else
        print_warning "Make not found - required for build system"
    fi
    
    # Check build targets
    echo -e "\n${CYAN}Available Build Targets:${NC}"
    echo "• make simulation          - Build core system simulation"
    echo "• make trading-demo        - Build and run trading system"
    echo "• make perf               - Run performance benchmarks"
    echo "• make full-demo          - Complete demonstration"
    echo "• make all                - Build full kernel (requires NASM)"
    echo "• make clean              - Clean all build artifacts"
    
    # Check current build status
    echo -e "\n${CYAN}Current Build Status:${NC}"
    
    if [ -f "tradekernel_sim" ]; then
        print_success "Core simulation binary ready"
    else
        print_info "Core simulation needs building"
    fi
    
    if [ -f "mock_trading" ]; then
        print_success "Trading system binary ready"
    else
        print_info "Trading system needs building"
    fi
    
    if [ -f "kernel.bin" ]; then
        print_success "Kernel binary available"
    else
        print_info "Kernel binary not built (requires NASM)"
    fi
    
    pause
}

# Function to show development guide
show_dev_guide() {
    clear
    print_header "📚 Development & Deployment Guide"
    
    echo -e "${CYAN}Quick Start:${NC}"
    echo "1. Clone repository and navigate to directory"
    echo "2. Run ./demo.sh for interactive demonstration"
    echo "3. Run 'make -f Makefile.simple full-demo' for automated demo"
    echo ""
    
    echo -e "${CYAN}Development Workflow:${NC}"
    echo "1. Core Development:"
    echo "   • Edit source files in src/ and include/"
    echo "   • Test with: make -f Makefile.simple simulation"
    echo "   • Profile with: make -f Makefile.simple perf"
    echo ""
    echo "2. Trading System Development:"
    echo "   • Modify mock_trading_system.cpp"
    echo "   • Test with: make -f Makefile.simple trading-demo"
    echo "   • Customize trading strategies and risk parameters"
    echo ""
    echo "3. Kernel Development:"
    echo "   • Install: brew install nasm qemu"
    echo "   • Build: make clean && make all"
    echo "   • Test: make run (launches QEMU)"
    echo ""
    
    echo -e "${CYAN}Production Deployment:${NC}"
    echo "1. Hardware Requirements:"
    echo "   • Intel x86_64 CPU with RDTSC support"
    echo "   • >= 8GB RAM (preferably DDR4-3200)"
    echo "   • Intel 10GbE NIC (for DPDK integration)"
    echo "   • NVMe SSD for low-latency logging"
    echo ""
    echo "2. Network Setup:"
    echo "   • Configure SR-IOV for NIC"
    echo "   • Set up DPDK environment"
    echo "   • Configure CPU isolation and NUMA"
    echo ""
    echo "3. Integration Steps:"
    echo "   • Add market data feed parsers"
    echo "   • Implement exchange connectivity"
    echo "   • Add compliance and audit logging"
    echo "   • Performance tuning and validation"
    echo ""
    
    echo -e "${CYAN}Documentation:${NC}"
    echo "• README.md           - Project overview and build instructions"
    echo "• DEVELOPMENT.md      - Detailed development guidelines"
    echo "• TRADING_DEMO.md     - Trading system documentation"
    echo "• docs/               - Technical specifications"
    
    pause
}

# Main execution loop
main() {
    while true; do
        show_menu
        read -r choice
        
        case $choice in
            1) show_overview ;;
            2) run_simulation ;;
            3) run_trading_demo ;;
            4) run_benchmarks ;;
            5) run_full_demo ;;
            6) show_build_status ;;
            7) show_dev_guide ;;
            0) 
                echo -e "\n${GREEN}Thank you for exploring TradeKernel!${NC}"
                echo -e "${GREEN}Ready for HFT deployment! 🚀📈${NC}\n"
                exit 0
                ;;
            *)
                echo -e "\n${RED}Invalid option. Please try again.${NC}"
                pause
                ;;
        esac
    done
}

# Check if script is being run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
