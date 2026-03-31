TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Use typed rset structure (see 3.16.1 release notes)
USR_CPPFLAGS += -DUSE_TYPED_RSET

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += exSupport

# Compile and add code to the support library
exSupport_SRCS += xxxRecord.c
exSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application, that
# causes problems on Windows DLL builds
exSupport_SRCS += dbSubExample.c
exSupport_SRCS += devexVersion.c
exSupport_SRCS += exHello.c
exSupport_SRCS += initTrace.c

exSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Auto-generate a header file containing a version string.
# Version comes from the VCS if available, else date+time.
GENVERSION = exVersion.h
# Macro name
GENVERSIONMACRO = exVERSION

# Build the IOC application
PROD_IOC = ex

# ex.dbd will be created and installed
DBD += ex.dbd

# ex.dbd will include these files:
ex_DBD += base.dbd
ex_DBD += xxxSupport.dbd
ex_DBD += dbSubExample.dbd
ex_DBD += devexVersion.dbd
ex_DBD += exHello.dbd
ex_DBD += initTrace.dbd

# ex_registerRecordDeviceDriver.cpp derives from ex.dbd
ex_SRCS += ex_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
ex_SRCS_DEFAULT += exMain.cpp
ex_SRCS_vxWorks += -nil-

# Link in the code from our support library
ex_LIBS += exSupport

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into exSupport
    sncExample_SNCFLAGS += +r
    ex_DBD += sncExample.dbd
    # A .stt sequence program is *not* pre-processed:
    exSupport_SRCS += sncExample.stt
    exSupport_LIBS += seq pv
    ex_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    # A .st sequence program *is* pre-processed:
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Link QSRV (pvAccess Server) if available
ifdef EPICS_QSRV_MAJOR_VERSION
    ex_LIBS += qsrv
    ex_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
    ex_DBD += PVAServerRegister.dbd
    ex_DBD += qsrv.dbd
endif

# Finally link IOC to the EPICS Base libraries
ex_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

# Explicit dependency needed for generated header file
devexVersion$(DEP): $(COMMON_DIR)/$(GENVERSION)
