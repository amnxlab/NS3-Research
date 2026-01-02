#!/bin/bash
# ============================================================
# install_edbr_evbf.sh - Install EDBR/EVBF to AquaSim-NG
# ============================================================
# This script copies the new energy-aware routing protocols 
# (EDBR and EVBF) to the AquaSim-NG module and rebuilds NS-3.
#
# Usage: bash install_edbr_evbf.sh
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RESEARCH_DIR="$(dirname "$SCRIPT_DIR")"
MODELS_DIR="$RESEARCH_DIR/models"

echo "=== EDBR/EVBF Installation Script ==="
echo "Research Directory: $RESEARCH_DIR"
echo "Models Directory: $MODELS_DIR"

# Find NS-3 installation
NS3_DIR=""
for dir in ~/ns3/ns-allinone-3.4*/ns-3.4* ~/ns-allinone-3.4*/ns-3.4*; do
    if [ -d "$dir" ] && [ -f "$dir/ns3" ]; then
        NS3_DIR="$dir"
        break
    fi
done

if [ -z "$NS3_DIR" ]; then
    echo "ERROR: Could not find NS-3 installation (ns-3.40-49)"
    echo "Searched in: ~/ns3/ns-allinone-3.4*/ns-3.4* and ~/ns-allinone-3.4*/ns-3.4*"
    exit 1
fi

echo "Found NS-3 at: $NS3_DIR"

# Find AquaSim-NG directory
AQUASIM_DIR=""
for dir in "$NS3_DIR/contrib/aqua-sim-ng" "$NS3_DIR/src/aqua-sim-ng"; do
    if [ -d "$dir" ]; then
        AQUASIM_DIR="$dir"
        break
    fi
done

if [ -z "$AQUASIM_DIR" ]; then
    echo "ERROR: Could not find AquaSim-NG in NS-3"
    echo "Searched in: $NS3_DIR/contrib/aqua-sim-ng and $NS3_DIR/src/aqua-sim-ng"
    exit 1
fi

echo "Found AquaSim-NG at: $AQUASIM_DIR"

AQUASIM_MODEL_DIR="$AQUASIM_DIR/model"

# Copy EDBR files
echo ""
echo "Copying EDBR files..."
cp -v "$MODELS_DIR/aqua-sim-routing-edbr.h" "$AQUASIM_MODEL_DIR/"
cp -v "$MODELS_DIR/aqua-sim-routing-edbr.cc" "$AQUASIM_MODEL_DIR/"

# Copy EVBF files
echo ""
echo "Copying EVBF files..."
cp -v "$MODELS_DIR/aqua-sim-routing-evbf.h" "$AQUASIM_MODEL_DIR/"
cp -v "$MODELS_DIR/aqua-sim-routing-evbf.cc" "$AQUASIM_MODEL_DIR/"

# Update CMakeLists.txt
CMAKE_FILE="$AQUASIM_DIR/CMakeLists.txt"
echo ""
echo "Updating CMakeLists.txt..."

# Check if already added
if grep -q "aqua-sim-routing-edbr" "$CMAKE_FILE"; then
    echo "EDBR already in CMakeLists.txt - skipping"
else
    # Find the line with aqua-sim-routing-dbr and add after it
    sed -i '/model\/aqua-sim-routing-dbr.cc/a\    model/aqua-sim-routing-edbr.cc' "$CMAKE_FILE"
    echo "Added aqua-sim-routing-edbr.cc to sources"
fi

if grep -q "aqua-sim-routing-evbf" "$CMAKE_FILE"; then
    echo "EVBF already in CMakeLists.txt - skipping"
else
    # Find the line with aqua-sim-routing-vbf and add after it
    sed -i '/model\/aqua-sim-routing-vbf.cc/a\    model/aqua-sim-routing-evbf.cc' "$CMAKE_FILE"
    echo "Added aqua-sim-routing-evbf.cc to sources"
fi

# Update header includes
if grep -q "model\/aqua-sim-routing-edbr.h" "$CMAKE_FILE"; then
    echo "EDBR header already in CMakeLists.txt - skipping"
else
    sed -i '/model\/aqua-sim-routing-dbr.h/a\    model/aqua-sim-routing-edbr.h' "$CMAKE_FILE"
    echo "Added aqua-sim-routing-edbr.h to headers"
fi

if grep -q "model\/aqua-sim-routing-evbf.h" "$CMAKE_FILE"; then
    echo "EVBF header already in CMakeLists.txt - skipping"
else
    sed -i '/model\/aqua-sim-routing-vbf.h/a\    model/aqua-sim-routing-evbf.h' "$CMAKE_FILE"
    echo "Added aqua-sim-routing-evbf.h to headers"
fi

# Rebuild NS-3
echo ""
echo "=== Rebuilding NS-3 ==="
cd "$NS3_DIR"
./ns3 configure --enable-examples
./ns3 build

echo ""
echo "=== Installation Complete ==="
echo "EDBR and EVBF are now available as protocols."
echo ""
echo "Test with:"
echo "  ./ns3 run 'uwsn-research_v2 --protocol=EDBR --nodes=50 --simStop=100'"
echo "  ./ns3 run 'uwsn-research_v2 --protocol=EVBF --nodes=50 --simStop=100'"
