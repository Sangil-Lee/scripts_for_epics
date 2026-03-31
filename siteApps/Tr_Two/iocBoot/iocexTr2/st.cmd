#!../../bin/linux-x86_64/exTr2

#- You may have to change exTr2 to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/exTr2.dbd"
exTr2_registerRecordDeviceDriver pdbbase

## Load record instances
#dbLoadRecords("db/exTr2.db","user=ctrluser")
#dbLoadRecords("db/waveform.db")
dbLoadRecords("db/dbProcessingExamples.db", "P=Test")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=ctrluser"
