#!/bin/bash
#
# UWSN Research Project Setup Script
# ===================================
# This script sets up the simulation environment by:
# 1. Copying simulation source to ns-3 scratch folder
# 2. Building ns-3
#
# Usage:
#   ./setup.sh           # Full setup (copy + build)
#   ./setup.sh --build   # Build only (no copy)
#   ./setup.sh --copy    # Copy only (no build)
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="${HOME}/ns3/ns-allinone-3.42/ns-3.42"
SCRATCH_DIR="${NS3_DIR}/scratch"
SRC_DIR="${SCRIPT_DIR}/src"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}   UWSN Research Project Setup${NC}"
echo -e "${GREEN}============================================${NC}"

# Check if ns-3 exists
if [ ! -d "$NS3_DIR" ]; then
    echo -e "${RED}ERROR: ns-3 not found at ${NS3_DIR}${NC}"
    echo "Please install ns-3 first. See README.md for instructions."
    exit 1
fi

# Parse arguments
DO_COPY=true
DO_BUILD=true

if [ "$1" == "--build" ]; then
    DO_COPY=false
elif [ "$1" == "--copy" ]; then
    DO_BUILD=false
fi

# Copy simulation files
if [ "$DO_COPY" = true ]; then
    echo -e "\n${YELLOW}Copying simulation files...${NC}"
    
    if [ -f "${SRC_DIR}/uwsn-research.cc" ]; then
        cp -v "${SRC_DIR}/uwsn-research.cc" "${SCRATCH_DIR}/"
        echo -e "${GREEN}✓ Copied uwsn-research.cc to scratch folder${NC}"
    else
        echo -e "${RED}ERROR: ${SRC_DIR}/uwsn-research.cc not found${NC}"
        exit 1
    fi
fi

# Build ns-3
if [ "$DO_BUILD" = true ]; then
    echo -e "\n${YELLOW}Building ns-3...${NC}"
    cd "$NS3_DIR"
    ./ns3 build
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Build successful${NC}"
    else
        echo -e "${RED}✗ Build failed${NC}"
        exit 1
    fi
fi

echo -e "\n${GREEN}============================================${NC}"
echo -e "${GREEN}   Setup Complete!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo "To run a simulation:"
echo "  cd ${NS3_DIR}"
echo "  ./ns3 run \"uwsn-research --protocol=VBF --simStop=100\""
echo ""
echo "To run experiments:"
echo "  cd ${SCRIPT_DIR}/scripts"
echo "  python run_experiments.py --runs 5 --simtime 100"
echo ""

