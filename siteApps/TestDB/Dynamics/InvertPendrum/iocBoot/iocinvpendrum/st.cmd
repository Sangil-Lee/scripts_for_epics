#!../../bin/linux-x86_64/invpendrum

#- You may have to change invpendrum to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/invpendrum.dbd"
invpendrum_registerRecordDeviceDriver pdbbase

## Load record instances
#dbLoadRecords("db/invpendrum.db","user=ctrluser")
dbLoadRecords("db/pendulum.db","P=Sim:")
#dbLoadRecords("db/pendulum_epid.db","P=Sim:")
dbLoadRecords("db/pendulum_calc.db","P=Sim:")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=ctrluser"
