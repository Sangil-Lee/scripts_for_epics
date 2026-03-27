#!/bin/bash
# =============================================================================
#
#   COTLAB EPICS Installer
#   Automated installation script for EPICS control system packages
#
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
#
# This software is freely available for use, modification, and redistribution
# by anyone for any purpose. No restrictions apply. See LICENSE file for
# the full license text.
#
# Based on the RAON EPICS install_script by Seung Hee Nam (namsh@ibs.re.kr)
# Restructured and modernized for COTLAB by COTLAB Team.
# =============================================================================
# Usage:
#   ./install.sh                    # Interactive menu
#   ./install.sh base [VERSION]     # Install EPICS Base (default: 7.0.8)
#   ./install.sh synapps [VERSION]  # Install synApps for EPICS version
#   ./install.sh extensions [VER]   # Install Extensions for EPICS version
#   ./install.sh pyepics [VERSION]  # Install PyEpics for EPICS version
#   ./install.sh phoebus [release|source]  # Install Phoebus
#   ./install.sh list               # List installed versions
#   ./install.sh all [VERSION]      # Install everything for a version
# =============================================================================

set -e

# Resolve script directory
INSTALLER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Load configuration and libraries
source "${INSTALLER_DIR}/config.sh"
source "${INSTALLER_DIR}/lib/common.sh"
source "${INSTALLER_DIR}/lib/env.sh"

# Load all modules
source "${INSTALLER_DIR}/modules/base.sh"
source "${INSTALLER_DIR}/modules/synapps.sh"
source "${INSTALLER_DIR}/modules/extensions.sh"
source "${INSTALLER_DIR}/modules/pyepics.sh"
source "${INSTALLER_DIR}/modules/phoebus.sh"

# Detect OS once
detect_os

# -----------------------------------------------------------------------------
# Banner
# -----------------------------------------------------------------------------
show_banner() {
    cat <<'BANNER'

   ██████╗ ██████╗ ████████╗██╗      █████╗ ██████╗
  ██╔════╝██╔═══██╗╚══██╔══╝██║     ██╔══██╗██╔══██╗
  ██║     ██║   ██║   ██║   ██║     ███████║██████╔╝
  ██║     ██║   ██║   ██║   ██║     ██╔══██║██╔══██╗
  ╚██████╗╚██████╔╝   ██║   ███████╗██║  ██║██████╔╝
   ╚═════╝ ╚═════╝    ╚═╝   ╚══════╝╚═╝  ╚═╝╚═════╝
          EPICS Installation Suite

BANNER
    printf "  Version: 1.0.0 | Default EPICS Base: %s\n" "${DEFAULT_EPICS_VERSION}"
    printf "  OS: %s %s | Arch: %s\n\n" "${OS_ID}" "${OS_VERSION}" "${EPICS_HOST_ARCH}"
}

# -----------------------------------------------------------------------------
# Interactive menu
# -----------------------------------------------------------------------------
show_menu() {
    show_banner
    echo "  Select a component to install:"
    echo "  ─────────────────────────────────────────────────────"
    echo "  1) EPICS Base          (version: ${DEFAULT_EPICS_VERSION})"
    echo "  2) EPICS Extensions    (VDCT, StripTool)"
    echo "  3) EPICS synApps       (synApps ${SYNAPPS_VERSION})"
    echo "  4) PyEpics             (Python EPICS bindings)"
    echo "  5) Phoebus             (v${PHOEBUS_VERSION}, replaces CS-Studio)"
    echo "  ─────────────────────────────────────────────────────"
    echo "  6) Install ALL         (Base + Extensions + synApps + PyEpics)"
    echo "  7) List installed      (Show installed EPICS versions)"
    echo "  ─────────────────────────────────────────────────────"
    echo "  0) Exit"
    echo ""
    printf "  Enter selection [0-7]: "
    read -r choice

    case "${choice}" in
        1)
            printf "  EPICS Base version [${DEFAULT_EPICS_VERSION}]: "
            read -r ver
            ver="${ver:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_epics_base "${ver}"
            post_install_env_setup "${ver}"
            ;;
        2)
            printf "  EPICS Base version [${DEFAULT_EPICS_VERSION}]: "
            read -r ver
            ver="${ver:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_extensions "${ver}"
            post_install_env_setup "${ver}"
            ;;
        3)
            printf "  EPICS Base version [${DEFAULT_EPICS_VERSION}]: "
            read -r ver
            ver="${ver:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_synapps "${ver}"
            post_install_env_setup "${ver}"
            ;;
        4)
            printf "  EPICS Base version [${DEFAULT_EPICS_VERSION}]: "
            read -r ver
            ver="${ver:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_pyepics "${ver}"
            post_install_env_setup "${ver}"
            ;;
        5)
            printf "  Install mode (release/source) [release]: "
            read -r mode
            mode="${mode:-release}"
            pkg_update
            install_phoebus "${mode}"
            ;;
        6)
            printf "  EPICS Base version [${DEFAULT_EPICS_VERSION}]: "
            read -r ver
            ver="${ver:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_epics_base "${ver}"
            install_extensions "${ver}"
            install_synapps "${ver}"
            install_pyepics "${ver}"
            log_info "All EPICS components installed for version ${ver}"
            post_install_env_setup "${ver}"
            ;;
        7)
            list_installed_bases
            ;;
        0)
            echo "  Bye!"
            exit 0
            ;;
        *)
            log_error "Invalid selection: ${choice}"
            exit 1
            ;;
    esac
}

# -----------------------------------------------------------------------------
# CLI mode
# -----------------------------------------------------------------------------
cli_mode() {
    local command="$1"
    shift

    case "${command}" in
        base)
            local ver="${1:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_epics_base "${ver}"
            post_install_env_setup "${ver}"
            ;;
        synapps)
            local ver="${1:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_synapps "${ver}"
            post_install_env_setup "${ver}"
            ;;
        extensions)
            local ver="${1:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_extensions "${ver}"
            post_install_env_setup "${ver}"
            ;;
        pyepics)
            local ver="${1:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_pyepics "${ver}"
            post_install_env_setup "${ver}"
            ;;
        phoebus)
            local mode="${1:-release}"
            pkg_update
            install_phoebus "${mode}"
            ;;
        list)
            list_installed_bases
            ;;
        all)
            local ver="${1:-${DEFAULT_EPICS_VERSION}}"
            pkg_update
            install_epics_base "${ver}"
            install_extensions "${ver}"
            install_synapps "${ver}"
            install_pyepics "${ver}"
            log_info "All EPICS components installed for version ${ver}"
            post_install_env_setup "${ver}"
            ;;
        help|--help|-h)
            show_banner
            echo "Usage:"
            echo "  ./install.sh                         Interactive menu"
            echo "  ./install.sh base [VERSION]           Install EPICS Base"
            echo "  ./install.sh synapps [VERSION]        Install synApps"
            echo "  ./install.sh extensions [VERSION]     Install Extensions"
            echo "  ./install.sh pyepics [VERSION]        Install PyEpics"
            echo "  ./install.sh phoebus [release|source] Install Phoebus"
            echo "  ./install.sh all [VERSION]            Install all components"
            echo "  ./install.sh list                     List installed versions"
            echo ""
            echo "Examples:"
            echo "  ./install.sh base 7.0.8"
            echo "  ./install.sh base 7.0.7"
            echo "  ./install.sh all 7.0.8"
            echo "  ./install.sh phoebus source"
            ;;
        *)
            log_error "Unknown command: ${command}"
            echo "Run './install.sh help' for usage information."
            exit 1
            ;;
    esac
}

# -----------------------------------------------------------------------------
# Entry point
# -----------------------------------------------------------------------------
if [ $# -eq 0 ]; then
    show_menu
else
    cli_mode "$@"
fi
