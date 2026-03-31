#!../../bin/linux-x86_64/tr3

#- You may have to change tr3 to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/tr3.dbd"
tr3_registerRecordDeviceDriver pdbbase

## Load record instances
#dbLoadRecords("db/tr3.db","user=ctrluser")
dbLoadRecords("db/trafficLight.db","P=TrafficLight:")
dbLoadRecords("db/calcASub.db","P=TEST")
dbLoadRecords("db/dbProcessingExamples.db","P=TEST")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=ctrluser"
seq trafficLight
