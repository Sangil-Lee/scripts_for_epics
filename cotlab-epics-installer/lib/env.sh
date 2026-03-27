#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - Environment Script Generator
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================

# -----------------------------------------------------------------------------
# Generate setEpicsEnv.sh for a specific version
# -----------------------------------------------------------------------------
generate_epics_env() {
    local version="$1"
    local epics_path
    epics_path=$(get_epics_path "${version}")

    log_step "Generating setEpicsEnv.sh for EPICS ${version}"

    cat > "${epics_path}/setEpicsEnv.sh" <<ENVEOF
#!/bin/bash
# COTLAB EPICS Environment - Version ${version}
# Generated: $(date +"%Y-%m-%d %H:%M:%S")

export EPICS_HOST_ARCH=${EPICS_HOST_ARCH}
export EPICS_ROOT=${EPICS_ROOT}
export EPICS_PATH=${epics_path}
export EPICS_BASE=${epics_path}/base
export EPICS_EXTENSIONS=${epics_path}/extensions
export COTLAB_SITEAPPS=${epics_path}/siteApps
export COTLAB_SITELIBS=${epics_path}/siteLibs

# PATH
export PATH=\${EPICS_BASE}/bin/\${EPICS_HOST_ARCH}:\${EPICS_EXTENSIONS}/bin/\${EPICS_HOST_ARCH}:\${COTLAB_SITEAPPS}/bin/\${EPICS_HOST_ARCH}:\${COTLAB_SITELIBS}/bin/\${EPICS_HOST_ARCH}:\${PATH}

# Library path
export LD_LIBRARY_PATH=\${EPICS_BASE}/lib/\${EPICS_HOST_ARCH}:\${EPICS_EXTENSIONS}/lib/\${EPICS_HOST_ARCH}:\${COTLAB_SITEAPPS}/lib/\${EPICS_HOST_ARCH}:\${COTLAB_SITELIBS}/lib/\${EPICS_HOST_ARCH}:\${LD_LIBRARY_PATH}

echo "COTLAB EPICS ${version} environment loaded (ARCH=\${EPICS_HOST_ARCH})"
ENVEOF

    chmod +x "${epics_path}/setEpicsEnv.sh"
    log_done "setEpicsEnv.sh"
}

# -----------------------------------------------------------------------------
# Generate use_epics.sh - Version switcher (installed to ~/epics/)
# Usage: source ~/epics/use_epics.sh 7.0.8
# -----------------------------------------------------------------------------
generate_version_switcher() {
    log_step "Generating version switcher: ~/epics/use_epics.sh"

    cat > "${EPICS_ROOT}/use_epics.sh" <<'SWITCHEOF'
#!/bin/bash
# COTLAB EPICS Version Switcher
# Usage: source ~/epics/use_epics.sh <version>
# Example: source ~/epics/use_epics.sh 7.0.8

if [ -z "$1" ]; then
    echo "Usage: source use_epics.sh <version>"
    echo ""
    echo "Available versions:"
    for d in ~/epics/*/; do
        if [ -f "${d}setEpicsEnv.sh" ]; then
            ver=$(basename "$d")
            if [ -f "${d}base/bin/linux-x86_64/softIoc" ] || \
               [ -f "${d}base/bin/linux-x86_64/softIocPVA" ] || \
               [ -d "${d}base/bin" ]; then
                echo "  ${ver}  [installed]"
            else
                echo "  ${ver}  [incomplete]"
            fi
        fi
    done
    return 1 2>/dev/null || exit 1
fi

EPICS_VER="$1"
ENV_FILE="${HOME}/epics/${EPICS_VER}/setEpicsEnv.sh"

if [ ! -f "${ENV_FILE}" ]; then
    echo "Error: EPICS ${EPICS_VER} not found at ~/epics/${EPICS_VER}/"
    return 1 2>/dev/null || exit 1
fi

# Clean old EPICS paths from PATH and LD_LIBRARY_PATH
_clean_path() {
    echo "$1" | tr ':' '\n' | grep -v "/epics/" | paste -sd ':' -
}
export PATH=$(_clean_path "$PATH")
export LD_LIBRARY_PATH=$(_clean_path "$LD_LIBRARY_PATH")

# Load the new environment
source "${ENV_FILE}"
SWITCHEOF

    chmod +x "${EPICS_ROOT}/use_epics.sh"
    log_done "use_epics.sh"
}

# -----------------------------------------------------------------------------
# Post-install: show environment summary and offer .bashrc registration
# Called at the end of any successful installation
# -----------------------------------------------------------------------------
post_install_env_setup() {
    local version="${1:-${DEFAULT_EPICS_VERSION}}"
    local epics_path
    epics_path=$(get_epics_path "${version}")
    local env_file="${epics_path}/setEpicsEnv.sh"

    # Only proceed if env file exists
    if [ ! -f "${env_file}" ]; then
        return 0
    fi

    echo ""
    echo "  ==========================================================="
    echo "  Installation Complete!"
    echo "  ==========================================================="
    echo ""
    echo "  EPICS ${version} environment file:"
    echo "    ${env_file}"
    echo ""
    echo "  To load manually:"
    echo "    source ${env_file}"
    echo ""
    echo "  To switch between versions:"
    echo "    source ~/epics/use_epics.sh ${version}"
    echo ""
    echo "  ─────────────────────────────────────────────────────────"
    echo "  Installed environment variables:"
    echo "  ─────────────────────────────────────────────────────────"
    grep '^export ' "${env_file}" | while IFS= read -r line; do
        printf "    %s\n" "${line}"
    done
    echo "  ─────────────────────────────────────────────────────────"
    echo ""

    # Check if already registered in .bashrc
    local bashrc="${HOME}/.bashrc"
    local marker="# COTLAB EPICS Environment (${version})"

    if grep -qF "${marker}" "${bashrc}" 2>/dev/null; then
        log_info ".bashrc already contains EPICS ${version} environment. Skipping."
        return 0
    fi

    printf "  Register EPICS ${version} environment in ~/.bashrc? [Y/n]: "
    read -r answer
    case "${answer}" in
        [Nn]*)
            echo ""
            log_info "Skipped .bashrc registration."
            log_info "You can always load it manually with:"
            log_info "  source ${env_file}"
            echo ""
            ;;
        *)
            # Append to .bashrc with a clear block
            cat >> "${bashrc}" <<RCEOF

${marker}
# Auto-generated by COTLAB EPICS Installer on $(date +"%Y-%m-%d %H:%M:%S")
# To remove: delete this block or comment it out
if [ -f "${env_file}" ]; then
    source "${env_file}"
fi
RCEOF
            echo ""
            log_info "Registered in ~/.bashrc"
            log_info "EPICS ${version} environment will load automatically on next login."
            log_info "To apply now, run:  source ~/.bashrc"
            echo ""
            ;;
    esac
}
