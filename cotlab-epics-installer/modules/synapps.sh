#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - synApps Module
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================

install_synapps() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_step "Installing synApps ${SYNAPPS_VERSION} for EPICS ${version}"

    # Check EPICS base
    if [ ! -d "${epics_path}/base/bin/${EPICS_HOST_ARCH}" ]; then
        log_error "EPICS Base ${version} not found. Install EPICS Base first."
        return 1
    fi

    # Source EPICS environment
    source "${epics_path}/setEpicsEnv.sh"

    # Install dependencies
    log_info "Installing synApps dependencies..."
    case "${PKG_MANAGER}" in
        apt)
            pkg_install re2c libusb-1.0-0-dev libusb-dev \
                        libx11-dev libxext-dev libtiff5-dev \
                        libxml2-dev libhdf5-dev
            ;;
        yum)
            pkg_install epel-release re2c libusb-devel \
                        libusbx-devel libX11-devel libXext-devel \
                        libtiff-devel libxml2-devel hdf5-devel
            ;;
    esac

    # Install msi extension (required by synApps)
    install_msi_extension "${version}"

    # Install szip (required by areaDetector HDF5 support)
    install_szip

    # Download synApps
    local synapp_file="synApps_${SYNAPPS_VERSION}.tar.gz"
    download_file "${SYNAPPS_URL}"

    # Extract
    mkdir -p "${epics_path}/epicsLibs"
    log_info "Extracting synApps..."
    ${TAR_CMD} "${DOWNLOAD_DIR}/${synapp_file}" \
        --transform "s|synApps_${SYNAPPS_VERSION}|synApps|" \
        -C "${epics_path}/epicsLibs"

    # Patch RELEASE files
    patch_synapps_release "${version}"

    # Build
    log_info "Building synApps (this may take a while)..."
    local synapp_dir="${epics_path}/epicsLibs/synApps/support"
    cd "${synapp_dir}"
    make -j$(nproc) 2>&1 | tee "${epics_path}/synapps_build_${LOG_DATE}.log"

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_warn "synApps build had errors. Check log: ${epics_path}/synapps_build_${LOG_DATE}.log"
    fi

    # Update environment with synApps path
    update_env_synapps "${version}"

    log_done "synApps installed at ${epics_path}/epicsLibs/synApps"
}

# -----------------------------------------------------------------------------
# Install msi extension (prerequisite for synApps)
# -----------------------------------------------------------------------------
install_msi_extension() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_info "Installing msi extension..."

    download_file "${EXTENSIONS_TOP_URL}"
    download_file "${MSI_URL}"

    ${TAR_CMD} "${DOWNLOAD_DIR}/extensionsTop_20120904.tar.gz" -C "${epics_path}"
    ${TAR_CMD} "${DOWNLOAD_DIR}/msi1-6.tar.gz" -C "${epics_path}/extensions/src"

    # Configure extensions
    configure_extensions "${version}"

    cd "${epics_path}/extensions/src/msi1-6"
    make
}

# -----------------------------------------------------------------------------
# Install szip library
# -----------------------------------------------------------------------------
install_szip() {
    if ldconfig -p 2>/dev/null | grep -q libsz; then
        log_info "szip already installed, skipping"
        return 0
    fi

    log_info "Installing szip 2.1.1..."
    local szip_url="https://support.hdfgroup.org/ftp/lib-external/szip/2.1.1/src/szip-2.1.1.tar.gz"
    download_file "${szip_url}"

    cd "${DOWNLOAD_DIR}"
    ${TAR_CMD} szip-2.1.1.tar.gz
    cd szip-2.1.1
    ./configure --prefix=/usr/local
    make
    make check
    ${SUDO_CMD} make install
    cd "${DOWNLOAD_DIR}"
}

# -----------------------------------------------------------------------------
# Configure extensions build for current architecture
# -----------------------------------------------------------------------------
configure_extensions() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")
    local extn_conf="${epics_path}/extensions/configure/os/CONFIG_SITE.${EPICS_HOST_ARCH}.${EPICS_HOST_ARCH}"

    if [ -f "${extn_conf}" ]; then
        mv "${extn_conf}" "${extn_conf}_origin"
    fi

    local lib_arch=""
    case "${EPICS_HOST_ARCH}" in
        linux-x86_64)  lib_arch="/x86_64-linux-gnu" ;;
        linux-x86)     lib_arch="/i386-linux-gnu" ;;
        linux-arm)     lib_arch="/arm-linux-gnueabihf" ;;
        linux-aarch64) lib_arch="/aarch64-linux-gnu" ;;
    esac

    # Use yum lib path for CentOS/RHEL
    if [ "${PKG_MANAGER}" = "yum" ]; then
        case "${EPICS_HOST_ARCH}" in
            linux-x86_64) lib_arch="64" ;;
            *)            lib_arch="" ;;
        esac
    fi

    cat > "${extn_conf}" <<EXTEOF
-include \$(TOP)/configure/os/CONFIG_SITE.linux-x86.linux-x86
X11_LIB=/usr/lib${lib_arch}
X11_INC=/usr/include
MOTIF_LIB=/usr/lib${lib_arch}
MOTIF_INC=/usr/include
JAVA_DIR=/usr
SCIPLOT=YES
XRTGRAPH_EXTENSIONS =
XRTGRAPH =
EXTEOF
}

# -----------------------------------------------------------------------------
# Patch synApps RELEASE files to point to our EPICS Base
# -----------------------------------------------------------------------------
patch_synapps_release() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")
    local epics_base="${epics_path}/base"
    local synapp_dir="${epics_path}/epicsLibs/synApps/support"

    log_info "Patching synApps RELEASE files..."
    cd "${synapp_dir}"

    # Patch EPICS_BASE in all RELEASE files
    find . -name 'RELEASE' -o -name 'RELEASE_LIBS.local' \
         -o -name 'RELEASE_BASE.local' -o -name 'RELEASE_PRODS.local' | \
    while read -r f; do
        sed -i "s|^EPICS_BASE=.*|EPICS_BASE=${epics_base}|g" "$f"
    done

    # Patch SUPPORT path
    find . -name 'RELEASE' -o -name 'RELEASE_SUPPORT.local' | \
    while read -r f; do
        sed -i "s|^SUPPORT=.*|SUPPORT=${synapp_dir}|g" "$f"
    done

    # Disable areaDetector by default (complex dependency, enable manually if needed)
    disable_areadetector "${synapp_dir}"

    # Fix HDF5/SZIP paths for systems that have areaDetector enabled
    local ad_config="${synapp_dir}/areaDetector-R2-0/configure/CONFIG_SITE.local.${EPICS_HOST_ARCH}"
    if [ -f "${ad_config}" ]; then
        sed -i "s|^HDF5         = /APSshare.*|HDF5=/usr|g" "${ad_config}"
        sed -i "s|^HDF5_LIB.*|HDF5_LIB=\$(HDF5)/lib/x86_64-linux-gnu/hdf5/serial|g" "${ad_config}"
        sed -i "s|^HDF5_INCLUDE.*|HDF5_INCLUDE=-I\$(HDF5)/include/hdf5/serial|g" "${ad_config}"
        sed -i "s|^SZIP           = /APSshare.*|SZIP=/usr/local|g" "${ad_config}"
        sed -i "s|^SZIP_LIB.*|SZIP_LIB=\$(SZIP)/lib|g" "${ad_config}"
        sed -i "s|^SZIP_INCLUDE.*|SZIP_INCLUDE=-I\$(SZIP)/include|g" "${ad_config}"
    fi
}

# -----------------------------------------------------------------------------
# Disable areaDetector modules in synApps
# -----------------------------------------------------------------------------
disable_areadetector() {
    local synapp_dir="$1"
    log_info "Disabling areaDetector modules (enable manually if needed)..."
    cd "${synapp_dir}"

    find . -name 'RELEASE' | while read -r f; do
        sed -i "s|^AREA_DETECTOR=.*|#AREA_DETECTOR=|g" "$f"
        sed -i "s|^ADCORE=.*|#ADCORE=|g" "$f"
        sed -i "s|^ADBINARIES=.*|#ADBINARIES=|g" "$f"
        sed -i "s|^QUADEM=.*|#QUADEM=|g" "$f"
        sed -i "s|^DXP=.*|#DXP=|g" "$f"
    done
}

# -----------------------------------------------------------------------------
# Update environment file with synApps path
# -----------------------------------------------------------------------------
update_env_synapps() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")
    local env_file="${epics_path}/setEpicsEnv.sh"

    if grep -q "EPICS_SYNAPPS" "${env_file}"; then
        sed -i "s|^export EPICS_SYNAPPS=.*|export EPICS_SYNAPPS=${epics_path}/epicsLibs/synApps/support|g" "${env_file}"
    else
        cat >> "${env_file}" <<SYNEOF

# synApps
export EPICS_SYNAPPS=${epics_path}/epicsLibs/synApps/support
SYNEOF
    fi
}
