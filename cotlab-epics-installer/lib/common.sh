#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - Common Library
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================

# -----------------------------------------------------------------------------
# Color output
# -----------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# -----------------------------------------------------------------------------
# Logging
# -----------------------------------------------------------------------------
log_info()  { printf "${GREEN}[INFO]${NC}  %s\n" "$1"; }
log_warn()  { printf "${YELLOW}[WARN]${NC}  %s\n" "$1"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$1"; }
log_step()  { printf "\n${CYAN}>>>>${NC} %s\n" "$1"; }
log_done()  { printf "${CYAN}<<<<${NC} %s ${GREEN}done${NC}\n\n" "$1"; }

# -----------------------------------------------------------------------------
# OS Detection (run once, export globally)
# -----------------------------------------------------------------------------
detect_os() {
    OS_ID="unknown"
    OS_VERSION=""
    EPICS_HOST_ARCH=""
    PKG_MANAGER=""

    # Detect distribution
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS_ID="${ID}"
        OS_VERSION="${VERSION_ID}"
    fi

    # Detect architecture
    case "$(uname -m)" in
        x86_64)
            EPICS_HOST_ARCH="linux-x86_64"
            ;;
        aarch64)
            EPICS_HOST_ARCH="linux-aarch64"
            ;;
        armv7l|armv6l)
            EPICS_HOST_ARCH="linux-arm"
            ;;
        *)
            log_error "Unsupported architecture: $(uname -m)"
            exit 1
            ;;
    esac

    # Detect package manager
    case "${OS_ID}" in
        ubuntu|debian|linuxmint|pop)
            PKG_MANAGER="apt"
            ;;
        centos|rhel|rocky|alma|fedora)
            PKG_MANAGER="yum"
            ;;
        *)
            log_warn "Unknown OS: ${OS_ID}. Assuming apt-based."
            PKG_MANAGER="apt"
            ;;
    esac

    export OS_ID OS_VERSION EPICS_HOST_ARCH PKG_MANAGER

    log_info "OS: ${OS_ID} ${OS_VERSION} | Arch: ${EPICS_HOST_ARCH} | Pkg: ${PKG_MANAGER}"
}

# -----------------------------------------------------------------------------
# Package installation wrapper
# -----------------------------------------------------------------------------
pkg_install() {
    local packages="$@"
    log_info "Installing packages: ${packages}"

    case "${PKG_MANAGER}" in
        apt)
            ${SUDO_CMD} apt-get install -y ${packages}
            ;;
        yum)
            ${SUDO_CMD} yum install -y ${packages}
            ;;
    esac
}

pkg_update() {
    log_info "Updating package lists..."
    case "${PKG_MANAGER}" in
        apt)
            ${SUDO_CMD} apt-get update
            ;;
        yum)
            ${SUDO_CMD} yum -y update
            ;;
    esac
}

# -----------------------------------------------------------------------------
# Prerequisite packages for EPICS build
# -----------------------------------------------------------------------------
install_build_essentials() {
    log_step "Installing build prerequisites"

    case "${PKG_MANAGER}" in
        apt)
            pkg_install build-essential libreadline-dev wget curl unzip \
                        perl pkg-config
            ;;
        yum)
            ${SUDO_CMD} yum -y groupinstall "Development Tools"
            pkg_install readline-devel perl-devel perl-Pod-Checker wget curl unzip
            ;;
    esac

    log_done "Build prerequisites"
}

# -----------------------------------------------------------------------------
# Resolve EPICS version path
# -----------------------------------------------------------------------------
get_epics_path() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    echo "${EPICS_ROOT}/${version}"
}

# -----------------------------------------------------------------------------
# Check if EPICS base is already installed for a version
# -----------------------------------------------------------------------------
check_epics_base_exists() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    if [ -d "${epics_path}/base/bin/${EPICS_HOST_ARCH}" ]; then
        return 0
    fi
    return 1
}

# -----------------------------------------------------------------------------
# Ensure downloads directory exists
# -----------------------------------------------------------------------------
ensure_download_dir() {
    mkdir -p "${DOWNLOAD_DIR}"
}

# -----------------------------------------------------------------------------
# Download file if not already present
# -----------------------------------------------------------------------------
download_file() {
    local url="$1"
    local filename
    filename=$(basename "${url}")

    ensure_download_dir
    if [ -f "${DOWNLOAD_DIR}/${filename}" ]; then
        log_info "Already downloaded: ${filename}"
    else
        log_info "Downloading: ${filename}"
        ${WGET_CMD} -P "${DOWNLOAD_DIR}" "${url}"
    fi
}

# -----------------------------------------------------------------------------
# Confirmation prompt
# -----------------------------------------------------------------------------
confirm() {
    local msg="${1:-Continue?}"
    printf "${YELLOW}%s [Y/n]: ${NC}" "${msg}"
    read -r answer
    case "${answer}" in
        [Nn]*) return 1 ;;
        *)     return 0 ;;
    esac
}
