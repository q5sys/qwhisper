#!/bin/bash

# QWhisper Installation Script
# Installs QWhisper to standard system locations

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Installation directories
PREFIX="${PREFIX:-/usr/local}"
BINDIR="${PREFIX}/bin"
ICONDIR="${PREFIX}/share/icons/hicolor/256x256/apps"
DESKTOPDIR="${PREFIX}/share/applications"
DOCDIR="${PREFIX}/share/doc/qwhisper"

# Check if running as root (needed for system-wide installation)
if [ "$EUID" -ne 0 ] && [ "$PREFIX" = "/usr/local" ]; then 
    echo -e "${YELLOW}Note: Running without root privileges. Will attempt user installation.${NC}"
    PREFIX="$HOME/.local"
    BINDIR="${PREFIX}/bin"
    ICONDIR="${PREFIX}/share/icons/hicolor/256x256/apps"
    DESKTOPDIR="${PREFIX}/share/applications"
    DOCDIR="${PREFIX}/share/doc/qwhisper"
    echo -e "${GREEN}Installing to user directory: ${PREFIX}${NC}"
fi

echo -e "${GREEN}QWhisper Installation Script${NC}"
echo "================================"
echo "Installation prefix: ${PREFIX}"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Build directory not found. Building QWhisper...${NC}"
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    cd ..
fi

# Check if executable exists
if [ ! -f "build/qwhisper" ]; then
    echo -e "${RED}Error: qwhisper executable not found in build directory!${NC}"
    echo "Please build the project first:"
    echo "  cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release"
    echo "  make -j\$(nproc)"
    exit 1
fi

# Create installation directories
echo "Creating installation directories..."
mkdir -p "${BINDIR}"
mkdir -p "${ICONDIR}"
mkdir -p "${DESKTOPDIR}"
mkdir -p "${DOCDIR}"

# Install executable
echo "Installing executable..."
cp -f build/qwhisper "${BINDIR}/qwhisper"
chmod 755 "${BINDIR}/qwhisper"

# Install icon
echo "Installing icon..."
if [ -f "resources/qwhisper-icon.png" ]; then
    cp -f resources/qwhisper-icon.png "${ICONDIR}/qwhisper.png"
else
    echo -e "${YELLOW}Warning: Icon file not found, skipping icon installation${NC}"
fi

# Install desktop file
echo "Installing desktop file..."
if [ -f "QWhisper.desktop" ]; then
    # Update paths in desktop file based on installation prefix
    sed "s|/usr/local|${PREFIX}|g" QWhisper.desktop > "${DESKTOPDIR}/QWhisper.desktop"
    chmod 644 "${DESKTOPDIR}/QWhisper.desktop"
else
    echo -e "${YELLOW}Warning: Desktop file not found, skipping desktop integration${NC}"
fi

# Install documentation
echo "Installing documentation..."
if [ -f "PROJECT_SUMMARY.md" ]; then
    cp -f PROJECT_SUMMARY.md "${DOCDIR}/README.md"
fi

# Update desktop database (if available)
if command -v update-desktop-database &> /dev/null; then
    echo "Updating desktop database..."
    update-desktop-database "${DESKTOPDIR}" 2>/dev/null || true
fi

# Update icon cache (if available)
if command -v gtk-update-icon-cache &> /dev/null; then
    echo "Updating icon cache..."
    gtk-update-icon-cache -f -t "${PREFIX}/share/icons/hicolor" 2>/dev/null || true
fi

# Add bin directory to PATH if it's a user installation and not already in PATH
if [ "$PREFIX" = "$HOME/.local" ]; then
    if [[ ":$PATH:" != *":$BINDIR:"* ]]; then
        echo ""
        echo -e "${YELLOW}Note: Add the following to your shell configuration file (.bashrc, .zshrc, etc.):${NC}"
        echo -e "${GREEN}export PATH=\"\$PATH:${BINDIR}\"${NC}"
        echo ""
    fi
fi

echo ""
echo -e "${GREEN}Installation complete!${NC}"
echo ""
echo "QWhisper has been installed to:"
echo "  Executable: ${BINDIR}/qwhisper"
echo "  Icon: ${ICONDIR}/qwhisper.png"
echo "  Desktop file: ${DESKTOPDIR}/QWhisper.desktop"
echo "  Documentation: ${DOCDIR}/"
echo ""
echo "You can now run QWhisper by:"
echo "  - Typing 'qwhisper' in the terminal"
echo "  - Launching it from your application menu"
echo ""
echo "To uninstall, run: ./uninstall.sh"
