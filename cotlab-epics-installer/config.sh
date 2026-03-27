#!/bin/bash
# =============================================================================
# COTLAB EPICS Installer - Global Configuration
# =============================================================================
# Copyright (c) 2025-2026 COTLAB
# This software is freely available for modification and redistribution.
# See LICENSE file for details.
# =============================================================================

# -----------------------------------------------------------------------------
# Default EPICS Base version (override with: ./install.sh base 7.0.7)
# -----------------------------------------------------------------------------
DEFAULT_EPICS_VERSION="7.0.8"

# -----------------------------------------------------------------------------
# Directory layout
#   ~/epics/{VERSION}/base/
#   ~/epics/{VERSION}/extensions/
#   ~/epics/{VERSION}/epicsLibs/synApps/
#   ~/epics/{VERSION}/siteApps/
#   ~/epics/{VERSION}/siteLibs/
#   ~/epics/downloads/
#   ~/epics/phoebus/
# -----------------------------------------------------------------------------
EPICS_ROOT="${HOME}/epics"
DOWNLOAD_DIR="${EPICS_ROOT}/downloads"

# -----------------------------------------------------------------------------
# synApps
# -----------------------------------------------------------------------------
SYNAPPS_VERSION="6_3"
SYNAPPS_URL="https://github.com/EPICS-synApps/support/releases/download/R6-3/synApps_6_3.tar.gz"

# -----------------------------------------------------------------------------
# Extensions
# -----------------------------------------------------------------------------
EXTENSIONS_TOP_URL="https://epics.anl.gov/download/extensions/extensionsTop_20120904.tar.gz"
MSI_URL="https://epics.anl.gov/download/extensions/msi1-6.tar.gz"
STRIPTOOL_VERSION="2_5_16_0"
STRIPTOOL_URL="https://epics.anl.gov/download/extensions/StripTool${STRIPTOOL_VERSION}.tar.gz"
VDCT_VERSION="2.8.2"
VDCT_URL="https://github.com/epics-extensions/VisualDCT/releases/download/v${VDCT_VERSION}/VisualDCT-${VDCT_VERSION}-distribution.zip"

# -----------------------------------------------------------------------------
# Phoebus (replaces CS-Studio)
# -----------------------------------------------------------------------------
PHOEBUS_VERSION="4.7.3"
PHOEBUS_URL="https://github.com/ControlSystemStudio/phoebus/releases/download/v${PHOEBUS_VERSION}/phoebus-${PHOEBUS_VERSION}.zip"

# -----------------------------------------------------------------------------
# PyEpics
# -----------------------------------------------------------------------------
PYEPICS_VERSION="3.5.7"

# -----------------------------------------------------------------------------
# Common commands
# -----------------------------------------------------------------------------
SUDO_CMD="sudo"
WGET_CMD="wget -c --timeout=30"
TAR_CMD="tar xzf"
LOG_DATE=$(date +"%Y.%m.%d.%H:%M")
