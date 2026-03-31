TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = invpendrum
# invpendrum.dbd will be created and installed
DBD += invpendrum.dbd

# invpendrum.dbd will be made up from these files:
invpendrum_DBD += base.dbd

# Include dbd files from all support applications:
#invpendrum_DBD += xxx.dbd
invpendrum_DBD += stdSupport.dbd 
invpendrum_DBD += simPendulum.dbd
invpendrum_DBD += menuScan.dbd

# Add all the support libraries needed by this IOC
#invpendrum_LIBS += xxx

# invpendrum_registerRecordDeviceDriver.cpp derives from invpendrum.dbd
invpendrum_SRCS += invpendrum_registerRecordDeviceDriver.cpp
invpendrum_SRCS += simPendulum.cpp


# Build the main IOC entry point on workstation OSs.
invpendrum_SRCS_DEFAULT += invpendrumMain.cpp
invpendrum_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#invpendrum_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
invpendrum_LIBS += std
invpendrum_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

