#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - Extensions Module (VDCT, StripTool)
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================

install_extensions() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_step "Installing EPICS Extensions for EPICS ${version}"

    # Check EPICS base
    if [ ! -d "${epics_path}/base/bin/${EPICS_HOST_ARCH}" ]; then
        log_error "EPICS Base ${version} not found. Install EPICS Base first."
        return 1
    fi

    # Source EPICS environment
    source "${epics_path}/setEpicsEnv.sh"

    # Install dependencies
    log_info "Installing extension dependencies..."
    case "${PKG_MANAGER}" in
        apt)
            pkg_install libxt-dev libmotif-dev libxpm-dev libxmu-dev unzip
            ;;
        yum)
            pkg_install libXt-devel libXpm-devel motif-devel
            ;;
    esac

    # Download and extract extensions top
    download_file "${EXTENSIONS_TOP_URL}"

    if [ ! -d "${epics_path}/extensions" ]; then
        ${TAR_CMD} "${DOWNLOAD_DIR}/extensionsTop_20120904.tar.gz" -C "${epics_path}"
    fi

    # Configure extensions build
    configure_extensions "${version}"

    # Install StripTool
    install_striptool "${version}"

    # Install VisualDCT
    install_vdct "${version}"

    log_done "EPICS Extensions installed at ${epics_path}/extensions"
}

# -----------------------------------------------------------------------------
# Install StripTool
# -----------------------------------------------------------------------------
install_striptool() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_info "Installing StripTool ${STRIPTOOL_VERSION}..."

    download_file "${STRIPTOOL_URL}"

    local striptool_file="StripTool${STRIPTOOL_VERSION}.tar.gz"
    ${TAR_CMD} "${DOWNLOAD_DIR}/${striptool_file}" -C "${epics_path}/extensions/src"

    cd "${epics_path}/extensions/src/StripTool${STRIPTOOL_VERSION}"

    # Fix basename conflict
    sed -i "s|basename|basename_sc|g" StripMisc.c StripDialog.c Strip.c StripMisc.h

    make 2>&1 | tee "${epics_path}/striptool_build_${LOG_DATE}.log"

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_warn "StripTool build had errors. Check log."
    else
        log_info "StripTool installed successfully"
    fi
}

# -----------------------------------------------------------------------------
# Install VisualDCT
# -----------------------------------------------------------------------------
install_vdct() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_info "Installing VisualDCT ${VDCT_VERSION}..."

    download_file "${VDCT_URL}"

    local vdct_file="VisualDCT-${VDCT_VERSION}-distribution.zip"
    unzip -o "${DOWNLOAD_DIR}/${vdct_file}" -d "${epics_path}/extensions/src"

    # Create launcher script
    mkdir -p "${epics_path}/extensions/bin/${EPICS_HOST_ARCH}"
    cat > "${epics_path}/extensions/bin/${EPICS_HOST_ARCH}/vdct" <<VDCTEOF
#!/bin/bash
java -jar ${epics_path}/extensions/src/VisualDCT-${VDCT_VERSION}/VisualDCT.jar "\$@"
VDCTEOF
    chmod +x "${epics_path}/extensions/bin/${EPICS_HOST_ARCH}/vdct"

    log_info "VisualDCT installed successfully"
}
