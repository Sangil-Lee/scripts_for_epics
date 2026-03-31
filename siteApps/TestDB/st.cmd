#softIoc -s -m "P=Sim:, R=Test:" -d Triangle.db
#softIoc -s -m "P=Sim:, R=Test:" -d PWM.db
#softIoc -s -m "P=IOC:, R=Test:" -d Sine.db -d Triangle.db
#softIoc -s -m "P=Sim:, R=Test:" -d Noise.db
#softIoc -s -m "P=Sim:, R=Test:" -d Virtual_Temp_Control.db
#softIoc -s -m "P=Sim:, R=Test:" -d TankPI_Control.db
#softIoc -s -m "P=Sim:, R=Test:" -d VacuumPump_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d Motor_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d MotorAccel_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d BeamOrbit_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d SiCTemp_Profile_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d MOCVD_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d RTP_Fast_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d APC_Semicon_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d RFMatching_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d Dose_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d BeamOptics_Mag_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d LinacBeamOptics_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d BeamBump_Control_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d BeamLifeTime_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d BeamTune_FB_Sim.db
#softIoc -s -m "P=Sim:, R=Test:" -d TankPI_Control.db -d Sine.db -d Triangle.db -d PWM.db

#softIoc -s -m "P=IOC:, R=Test:" -d Sine.db -d Triangle.db -m "P=IOC2:,R=Test2:" -d Sine.db -d Triangle.db
#softIoc -s -d WCS_sensor_act.db -d WCS.db

softIoc -s -d WTS.db -m "P=IOC:, R=Test:" -d Sine.db -d Triangle.db
