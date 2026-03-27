#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - PyEpics Module
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================

install_pyepics() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_step "Installing PyEpics for EPICS ${version}"

    # Check EPICS base
    if [ ! -d "${epics_path}/base/bin/${EPICS_HOST_ARCH}" ]; then
        log_error "EPICS Base ${version} not found. Install EPICS Base first."
        return 1
    fi

    # Source EPICS environment
    source "${epics_path}/setEpicsEnv.sh"

    # Install Python3 and pip
    log_info "Installing Python3 and pip..."
    case "${PKG_MANAGER}" in
        apt)
            pkg_install python3 python3-pip python3-venv
            ;;
        yum)
            pkg_install python3 python3-pip
            ;;
    esac

    # Create a virtual environment for EPICS Python tools
    local venv_dir="${epics_path}/pyepics-venv"

    if [ -d "${venv_dir}" ]; then
        log_warn "PyEpics venv already exists at ${venv_dir}"
        if ! confirm "Recreate PyEpics virtual environment?"; then
            log_info "Skipping PyEpics installation"
            return 0
        fi
        rm -rf "${venv_dir}"
    fi

    log_info "Creating Python virtual environment..."
    python3 -m venv "${venv_dir}"

    # Install pyepics and common scientific packages
    log_info "Installing PyEpics ${PYEPICS_VERSION} and scientific packages..."
    "${venv_dir}/bin/pip" install --upgrade pip
    "${venv_dir}/bin/pip" install \
        pyepics=="${PYEPICS_VERSION}" \
        numpy scipy matplotlib pandas

    if [ $? -ne 0 ]; then
        log_error "PyEpics installation failed"
        return 1
    fi

    # Update environment file with pyepics venv activation hint
    update_env_pyepics "${version}"

    log_info ""
    log_info "To use PyEpics:"
    log_info "  source ${venv_dir}/bin/activate"
    log_info "  python3 -c 'import epics; print(epics.__version__)'"
    log_info ""

    log_done "PyEpics ${PYEPICS_VERSION} installed in ${venv_dir}"
}

# -----------------------------------------------------------------------------
# Update environment file with PyEpics info
# -----------------------------------------------------------------------------
update_env_pyepics() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")
    local env_file="${epics_path}/setEpicsEnv.sh"

    if ! grep -q "PYEPICS_VENV" "${env_file}"; then
        cat >> "${env_file}" <<PYEOF

# PyEpics virtual environment
export PYEPICS_VENV=${epics_path}/pyepics-venv
# Activate with: source \${PYEPICS_VENV}/bin/activate
PYEOF
    fi
}
