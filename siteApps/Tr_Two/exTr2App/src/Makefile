TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = exTr2
# exTr2.dbd will be created and installed
DBD += exTr2.dbd

# exTr2.dbd will be made up from these files:
exTr2_DBD += base.dbd

# Include dbd files from all support applications:
#exTr2_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#exTr2_LIBS += xxx

# exTr2_registerRecordDeviceDriver.cpp derives from exTr2.dbd
exTr2_SRCS += exTr2_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
exTr2_SRCS_DEFAULT += exTr2Main.cpp
exTr2_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#exTr2_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
exTr2_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

