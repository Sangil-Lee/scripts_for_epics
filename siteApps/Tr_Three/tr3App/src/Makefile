TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = tr3
# tr3.dbd will be created and installed
DBD += tr3.dbd

# tr3.dbd will be made up from these files:
tr3_DBD += base.dbd

# Include dbd files from all support applications:
#tr3_DBD += xxx.dbd
tr3_DBD += calcWaveformASub.dbd
tr3_DBD += calc.dbd

# Add all the support libraries needed by this IOC
#tr3_LIBS += xxx
tr3_LIBS += calc

# tr3_registerRecordDeviceDriver.cpp derives from tr3.dbd
tr3_SRCS += tr3_registerRecordDeviceDriver.cpp
tr3_SRCS += calcWaveformASub.cpp

# Build the main IOC entry point on workstation OSs.
tr3_SRCS_DEFAULT += tr3Main.cpp
tr3_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#tr3_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary
ifneq ($(SNCSEQ),)
    tr3_SNCFLAGS += +r
    tr3_DBD  += trafficLight.dbd
    tr3_SRCS += trafficLight.st

    tr3_LIBS += seq pv
endif

# Finally link to the EPICS Base libraries
tr3_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

