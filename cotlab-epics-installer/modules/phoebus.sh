#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - Phoebus Module
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# =============================================================================
# Phoebus: https://github.com/ControlSystemStudio/phoebus
# Replaces CS-Studio as the EPICS operator interface tool.
# Requires JDK 17+ and Maven 3.x for source build, or download release JAR.
# =============================================================================

install_phoebus() {
    local mode="${1:-release}"  # "release" or "source"

    log_step "Installing Phoebus ${PHOEBUS_VERSION} (mode: ${mode})"

    case "${mode}" in
        release)
            install_phoebus_release
            ;;
        source)
            install_phoebus_source
            ;;
        *)
            log_error "Unknown Phoebus install mode: ${mode}. Use 'release' or 'source'."
            return 1
            ;;
    esac
}

# -----------------------------------------------------------------------------
# Install Phoebus from GitHub release (recommended, fast)
# -----------------------------------------------------------------------------
install_phoebus_release() {
    local phoebus_dir="${EPICS_ROOT}/phoebus"

    if [ -d "${phoebus_dir}" ]; then
        log_warn "Phoebus already installed at ${phoebus_dir}"
        if ! confirm "Reinstall Phoebus?"; then
            return 0
        fi
        mv "${phoebus_dir}" "${phoebus_dir}_bak_${LOG_DATE}"
    fi

    # Install JDK 17
    install_jdk17

    mkdir -p "${phoebus_dir}"

    # Download release
    local filename="phoebus-${PHOEBUS_VERSION}.zip"
    download_file "${PHOEBUS_URL}"

    log_info "Extracting Phoebus..."
    unzip -o "${DOWNLOAD_DIR}/${filename}" -d "${phoebus_dir}"

    # Create launcher script
    create_phoebus_launcher "${phoebus_dir}"

    log_done "Phoebus ${PHOEBUS_VERSION} installed at ${phoebus_dir}"
}

# -----------------------------------------------------------------------------
# Build Phoebus from source (for customization)
# -----------------------------------------------------------------------------
install_phoebus_source() {
    local phoebus_dir="${EPICS_ROOT}/phoebus"

    if [ -d "${phoebus_dir}" ]; then
        log_warn "Phoebus directory already exists at ${phoebus_dir}"
        if ! confirm "Remove and rebuild Phoebus from source?"; then
            return 0
        fi
        mv "${phoebus_dir}" "${phoebus_dir}_bak_${LOG_DATE}"
    fi

    # Install JDK 17 and Maven
    install_jdk17
    install_maven

    log_info "Cloning Phoebus repository..."
    git clone --depth 1 --branch "v${PHOEBUS_VERSION}" \
        https://github.com/ControlSystemStudio/phoebus.git "${phoebus_dir}"

    if [ $? -ne 0 ]; then
        log_error "Failed to clone Phoebus repository"
        return 1
    fi

    log_info "Building Phoebus (this will take 10-20 minutes)..."
    cd "${phoebus_dir}"
    mvn clean install -pl services/alarm-server,services/alarm-logger,phoebus-product -am -DskipTests \
        2>&1 | tee "${EPICS_ROOT}/phoebus_build_${LOG_DATE}.log"

    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        log_error "Phoebus build failed. Check log: ${EPICS_ROOT}/phoebus_build_${LOG_DATE}.log"
        return 1
    fi

    # Create launcher script
    create_phoebus_launcher "${phoebus_dir}"

    log_done "Phoebus built from source at ${phoebus_dir}"
}

# -----------------------------------------------------------------------------
# Install JDK 17
# -----------------------------------------------------------------------------
install_jdk17() {
    if java -version 2>&1 | grep -q 'version "17\|version "21\|version "22'; then
        log_info "JDK 17+ already installed"
        export JAVA_HOME=$(dirname $(dirname $(readlink -f $(which java))))
        return 0
    fi

    log_info "Installing JDK 17..."
    case "${PKG_MANAGER}" in
        apt)
            pkg_install openjdk-17-jdk
            ;;
        yum)
            pkg_install java-17-openjdk-devel
            ;;
    esac

    export JAVA_HOME=$(dirname $(dirname $(readlink -f $(which java))))
    log_info "JAVA_HOME=${JAVA_HOME}"
}

# -----------------------------------------------------------------------------
# Install Maven 3.x
# -----------------------------------------------------------------------------
install_maven() {
    if command -v mvn &>/dev/null; then
        log_info "Maven already installed: $(mvn --version | head -1)"
        return 0
    fi

    log_info "Installing Maven..."
    case "${PKG_MANAGER}" in
        apt)
            pkg_install maven
            ;;
        yum)
            pkg_install maven
            ;;
    esac
}

# -----------------------------------------------------------------------------
# Create Phoebus launcher script
# -----------------------------------------------------------------------------
create_phoebus_launcher() {
    local phoebus_dir="$1"

    # Find the product JAR
    local product_jar
    product_jar=$(find "${phoebus_dir}" -name "product-*-SNAPSHOT.jar" -o -name "phoebus-product*.jar" 2>/dev/null | head -1)

    if [ -z "${product_jar}" ]; then
        # For release downloads, look for any runnable jar or script
        product_jar=$(find "${phoebus_dir}" -name "*.jar" -path "*/product/*" 2>/dev/null | head -1)
    fi

    cat > "${phoebus_dir}/run-phoebus.sh" <<PHEOF
#!/bin/bash
# COTLAB Phoebus Launcher
# Phoebus ${PHOEBUS_VERSION}

export JAVA_HOME=\$(dirname \$(dirname \$(readlink -f \$(which java))))

# Find the Phoebus product JAR
PHOEBUS_DIR="${phoebus_dir}"
PRODUCT_JAR=\$(find "\${PHOEBUS_DIR}" -name "product-*-SNAPSHOT.jar" -o -name "phoebus-product*.jar" 2>/dev/null | head -1)

if [ -z "\${PRODUCT_JAR}" ]; then
    PRODUCT_JAR=\$(find "\${PHOEBUS_DIR}" -name "*.jar" -path "*/product/*" 2>/dev/null | head -1)
fi

if [ -z "\${PRODUCT_JAR}" ]; then
    echo "Error: Cannot find Phoebus product JAR in \${PHOEBUS_DIR}"
    exit 1
fi

echo "Starting Phoebus from: \${PRODUCT_JAR}"
java -jar "\${PRODUCT_JAR}" "\$@"
PHEOF

    chmod +x "${phoebus_dir}/run-phoebus.sh"

    # Create symlink in ~/bin if exists
    if [ -d "${HOME}/bin" ]; then
        ln -sf "${phoebus_dir}/run-phoebus.sh" "${HOME}/bin/phoebus"
        log_info "Symlink created: ~/bin/phoebus"
    fi

    log_info "Launcher: ${phoebus_dir}/run-phoebus.sh"
}
