#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - EPICS Base Module
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================

install_epics_base() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_step "Installing EPICS Base ${version}"

    # Check if already installed
    if [ -d "${epics_path}/base/bin/${EPICS_HOST_ARCH}" ]; then
        log_warn "EPICS Base ${version} already installed at ${epics_path}/base"
        if ! confirm "Reinstall EPICS Base ${version}?"; then
            log_info "Skipping EPICS Base installation"
            return 0
        fi
        log_info "Backing up existing base to base_bak_${LOG_DATE}"
        mv "${epics_path}/base" "${epics_path}/base_bak_${LOG_DATE}"
    fi

    # Load version-specific config if available
    local version_conf="${INSTALLER_DIR}/versions/base-${version}.conf"
    if [ -f "${version_conf}" ]; then
        log_info "Loading version config: base-${version}.conf"
        source "${version_conf}"
    fi

    # Determine download URL
    local base_url
    base_url=$(get_base_download_url "${version}")

    # Create directory structure
    mkdir -p "${epics_path}"
    mkdir -p "${DOWNLOAD_DIR}"

    # Install build prerequisites
    install_build_essentials

    # Download EPICS Base
    local filename="base-${version}.tar.gz"
    if [ ! -f "${DOWNLOAD_DIR}/${filename}" ]; then
        log_info "Downloading EPICS Base ${version}..."
        ${WGET_CMD} -O "${DOWNLOAD_DIR}/${filename}" "${base_url}"
        if [ $? -ne 0 ]; then
            log_error "Failed to download EPICS Base ${version}"
            return 1
        fi
    else
        log_info "Already downloaded: ${filename}"
    fi

    # Extract
    log_info "Extracting EPICS Base ${version}..."
    ${TAR_CMD} "${DOWNLOAD_DIR}/${filename}" \
        --transform "s|^base-${version}|base|" \
        -C "${epics_path}"

    if [ $? -ne 0 ]; then
        log_error "Failed to extract EPICS Base"
        return 1
    fi

    # Build
    log_info "Building EPICS Base ${version} (this may take several minutes)..."
    cd "${epics_path}/base"
    make -j$(nproc) 2>&1 | tee "${epics_path}/base_build_${LOG_DATE}.log"

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_error "EPICS Base build failed. Check log: ${epics_path}/base_build_${LOG_DATE}.log"
        return 1
    fi

    # Create supporting directories
    mkdir -p "${epics_path}/siteApps"
    mkdir -p "${epics_path}/siteLibs"

    # Generate environment script
    generate_epics_env "${version}"

    # Source the environment
    source "${epics_path}/setEpicsEnv.sh"

    # Generate version switcher
    generate_version_switcher

    log_done "EPICS Base ${version} installed at ${epics_path}/base"
}

# -----------------------------------------------------------------------------
# Resolve download URL for a given EPICS Base version
# Priority: version config file > GitHub releases > ANL site
# -----------------------------------------------------------------------------
get_base_download_url() {
    local version="$1"

    # Check if version config provides a custom URL
    if [ -n "${BASE_DOWNLOAD_URL:-}" ]; then
        echo "${BASE_DOWNLOAD_URL}"
        return
    fi

    # Determine major version to pick the right source
    local major_minor
    major_minor=$(echo "${version}" | cut -d. -f1,2)

    case "${major_minor}" in
        7.*)
            # EPICS 7.x - GitHub releases
            echo "https://epics-controls.org/download/base/base-${version}.tar.gz"
            ;;
        3.*)
            # EPICS 3.x - ANL site
            echo "https://epics.anl.gov/download/base/base-${version}.tar.gz"
            ;;
        *)
            # Fallback
            echo "https://epics-controls.org/download/base/base-${version}.tar.gz"
            ;;
    esac
}

# -----------------------------------------------------------------------------
# List installed EPICS Base versions
# -----------------------------------------------------------------------------
list_installed_bases() {
    log_info "Installed EPICS Base versions:"
    local found=0
    for d in "${EPICS_ROOT}"/*/base; do
        if [ -d "$d" ]; then
            local ver
            ver=$(basename "$(dirname "$d")")
            local status="incomplete"
            if [ -d "${d}/bin/${EPICS_HOST_ARCH}" ]; then
                status="ready"
            fi
            printf "  %-12s [%s]  %s\n" "${ver}" "${status}" "$(dirname "$d")"
            found=1
        fi
    done
    if [ $found -eq 0 ]; then
        log_info "  (none)"
    fi
}
