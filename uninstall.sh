#!/bin/bash

# QWhisper Uninstallation Script
# Removes QWhisper from system

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Installation directories (check both system and user locations)
SYSTEM_PREFIX="/usr/local"
USER_PREFIX="$HOME/.local"

# Function to remove file if it exists
remove_file() {
    if [ -f "$1" ]; then
        echo "  Removing: $1"
        rm -f "$1" 2>/dev/null || sudo rm -f "$1" 2>/dev/null || true
    fi
}

# Function to remove directory if it exists and is empty
remove_dir_if_empty() {
    if [ -d "$1" ]; then
        if [ -z "$(ls -A "$1")" ]; then
            echo "  Removing empty directory: $1"
            rmdir "$1" 2>/dev/null || sudo rmdir "$1" 2>/dev/null || true
        fi
    fi
}

echo -e "${GREEN}QWhisper Uninstallation Script${NC}"
echo "================================="
echo ""

# Check for installations
FOUND_INSTALLATION=false

# Check system installation
if [ -f "${SYSTEM_PREFIX}/bin/qwhisper" ]; then
    echo -e "${YELLOW}Found system-wide installation in ${SYSTEM_PREFIX}${NC}"
    FOUND_INSTALLATION=true
    
    # Check if we need sudo
    if [ ! -w "${SYSTEM_PREFIX}/bin/qwhisper" ]; then
        echo -e "${YELLOW}Note: System-wide uninstallation requires root privileges.${NC}"
        echo "You may be prompted for your password."
        echo ""
    fi
    
    echo "Removing system-wide installation..."
    remove_file "${SYSTEM_PREFIX}/bin/qwhisper"
    remove_file "${SYSTEM_PREFIX}/share/icons/hicolor/256x256/apps/qwhisper.png"
    remove_file "${SYSTEM_PREFIX}/share/applications/QWhisper.desktop"
    remove_file "${SYSTEM_PREFIX}/share/doc/qwhisper/README.md"
    remove_dir_if_empty "${SYSTEM_PREFIX}/share/doc/qwhisper"
    
    # Update desktop database (if available)
    if command -v update-desktop-database &> /dev/null; then
        echo "Updating desktop database..."
        update-desktop-database "${SYSTEM_PREFIX}/share/applications" 2>/dev/null || \
        sudo update-desktop-database "${SYSTEM_PREFIX}/share/applications" 2>/dev/null || true
    fi
    
    # Update icon cache (if available)
    if command -v gtk-update-icon-cache &> /dev/null; then
        echo "Updating icon cache..."
        gtk-update-icon-cache -f -t "${SYSTEM_PREFIX}/share/icons/hicolor" 2>/dev/null || \
        sudo gtk-update-icon-cache -f -t "${SYSTEM_PREFIX}/share/icons/hicolor" 2>/dev/null || true
    fi
fi

# Check user installation
if [ -f "${USER_PREFIX}/bin/qwhisper" ]; then
    echo -e "${YELLOW}Found user installation in ${USER_PREFIX}${NC}"
    FOUND_INSTALLATION=true
    
    echo "Removing user installation..."
    remove_file "${USER_PREFIX}/bin/qwhisper"
    remove_file "${USER_PREFIX}/share/icons/hicolor/256x256/apps/qwhisper.png"
    remove_file "${USER_PREFIX}/share/applications/QWhisper.desktop"
    remove_file "${USER_PREFIX}/share/doc/qwhisper/README.md"
    remove_dir_if_empty "${USER_PREFIX}/share/doc/qwhisper"
    
    # Update desktop database (if available)
    if command -v update-desktop-database &> /dev/null; then
        echo "Updating desktop database..."
        update-desktop-database "${USER_PREFIX}/share/applications" 2>/dev/null || true
    fi
    
    # Update icon cache (if available)
    if command -v gtk-update-icon-cache &> /dev/null; then
        echo "Updating icon cache..."
        gtk-update-icon-cache -f -t "${USER_PREFIX}/share/icons/hicolor" 2>/dev/null || true
    fi
fi

# Check for configuration files
CONFIG_DIR="$HOME/.config/qwhisper"
if [ -d "$CONFIG_DIR" ]; then
    echo ""
    echo -e "${YELLOW}Found configuration directory: ${CONFIG_DIR}${NC}"
    read -p "Do you want to remove configuration files? (y/N): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing configuration files..."
        rm -rf "$CONFIG_DIR"
    else
        echo "Configuration files preserved."
    fi
fi

# Check for data files (models)
DATA_DIR="$HOME/.local/share/qwhisper"
if [ -d "$DATA_DIR" ]; then
    echo ""
    echo -e "${YELLOW}Found data directory (contains downloaded models): ${DATA_DIR}${NC}"
    # Calculate size of models directory
    if [ -d "$DATA_DIR/models" ]; then
        SIZE=$(du -sh "$DATA_DIR/models" 2>/dev/null | cut -f1)
        echo "Model directory size: ${SIZE}"
    fi
    read -p "Do you want to remove downloaded models? (y/N): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing data files..."
        rm -rf "$DATA_DIR"
    else
        echo "Data files preserved."
    fi
fi

if [ "$FOUND_INSTALLATION" = false ]; then
    echo -e "${YELLOW}No QWhisper installation found.${NC}"
    echo "Checked locations:"
    echo "  - ${SYSTEM_PREFIX}/bin/qwhisper"
    echo "  - ${USER_PREFIX}/bin/qwhisper"
else
    echo ""
    echo -e "${GREEN}Uninstallation complete!${NC}"
fi

echo ""
echo "Note: If you added QWhisper to your PATH manually,"
echo "you may want to remove that line from your shell configuration file."
