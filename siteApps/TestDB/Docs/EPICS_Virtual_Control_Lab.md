# **EPICS Virtual Control Lab (가상 제어 실습 키트)**

이 문서는 EPICS Base의 기본 레코드(calc, ao, bo)만을 사용하여 구현한 4가지 가상 물리 시스템의 데이터베이스(DB) 코드를 포함하고 있습니다.

## **목차**

1. \[Part 1\] 수조 수위 제어 (Level Control with PI Logic)  
2. \[Part 2\] 진공 배기 시스템 (Vacuum Pumping with Interlocks)  
3. \[Part 3\] 모션 제어 (Motion Control with Soft Start/Stop)  
4. \[Part 4\] 빔 궤도 피드백 (MIMO Beam Orbit Feedback)  
5. \[Appendix\] IOC 구동 스크립트 예시 (st.cmd)

## **1\. 수조 수위 제어 (tank.db)**

**특징:** calc 레코드만으로 PI(비례-적분) 제어기를 구현하여 수위를 일정하게 유지합니다.

\# \------------------------------------------------------------------  
\# EPICS Water Tank Simulator (PID logic using CALC records)  
\# Macro: $(P) \= Prefix, $(R) \= Device Name  
\# \------------------------------------------------------------------

\# \[User Settings\]  
record(ao, "$(P)$(R)SetPoint") {  
    field(DESC, "Target Level")  
    field(VAL,  "50.0")  
    field(DRVL, "0")  
    field(DRVH, "100")  
    field(EGU,  "cm")  
    field(PINI, "YES")  
}

record(ao, "$(P)$(R)Outflow") {  
    field(DESC, "Disturbance (Leak)")  
    field(VAL,  "0.0")  
    field(DRVL, "0")  
    field(DRVH, "50")  
    field(PINI, "YES")  
}

record(bo, "$(P)$(R)Mode") {  
    field(DESC, "0=Man, 1=Auto")  
    field(ZNAM, "Manual")  
    field(ONAM, "Auto")  
    field(VAL,  "0")  
    field(FLNK, "$(P)$(R)PID:I\_Term")  
}

record(ao, "$(P)$(R)Valve:Man") {  
    field(DESC, "Manual Valve Set")  
    field(VAL,  "0")  
    field(DRVL, "0")  
    field(DRVH, "100")  
}

\# \[PID Controller\]  
record(ao, "$(P)$(R)PID:KP") {  
    field(DESC, "Proportional Gain")  
    field(VAL,  "2.0")  
}  
record(ao, "$(P)$(R)PID:KI") {  
    field(DESC, "Integral Gain")  
    field(VAL,  "0.5")  
}

record(calc, "$(P)$(R)PID:Error") {  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)SetPoint NPP")  
    field(INPB, "$(P)$(R)Level NPP")  
    field(CALC, "A \- B")  
    field(FLNK, "$(P)$(R)PID:I\_Term")  
}

record(calc, "$(P)$(R)PID:I\_Term") {  
    field(DESC, "Integral Accumulator")  
    field(INPA, "$(P)$(R)PID:Error NPP")  
    field(INPB, "$(P)$(R)PID:KI NPP")  
    field(INPC, "$(P)$(R)Mode NPP")  
    field(INPD, "$(P)$(R)PID:I\_Term NPP")  
    \# Anti-windup Logic  
    field(CALC, "C ? MAX(-20, MIN(120, D \+ (A\*B\*0.1))) : 0")  
    field(FLNK, "$(P)$(R)PID:Output")  
}

record(calc, "$(P)$(R)PID:Output") {  
    field(DESC, "Final PID Output")  
    field(INPA, "$(P)$(R)PID:Error NPP")  
    field(INPB, "$(P)$(R)PID:KP NPP")  
    field(INPC, "$(P)$(R)PID:I\_Term NPP")  
    field(CALC, "MAX(0, MIN(100, (A\*B) \+ C))")  
    field(FLNK, "$(P)$(R)Valve:Act")  
}

\# \[Plant Model\]  
record(calc, "$(P)$(R)Valve:Act") {  
    field(DESC, "Actual Valve Position")  
    field(INPA, "$(P)$(R)Valve:Man NPP")  
    field(INPB, "$(P)$(R)PID:Output NPP")  
    field(INPC, "$(P)$(R)Mode NPP")  
    field(CALC, "C ? B : A")  
    field(FLNK, "$(P)$(R)Level")  
}

record(calc, "$(P)$(R)Level") {  
    field(DESC, "Water Level Simulation")  
    field(INPA, "$(P)$(R)Level NPP")  
    field(INPB, "$(P)$(R)Valve:Act NPP")  
    field(INPC, "$(P)$(R)Outflow NPP")  
    field(CALC, "MAX(0, MIN(100, A \+ (B\*0.05) \- (C\*0.05)))")  
    field(PREC, "2")  
}

### **\[Test Scenario 1\] 수위 제어 실습**

1. **수동 조작 (Manual Mode)**  
   * Sim:Tank:Mode를 \*\*0 (Manual)\*\*으로 설정합니다.  
   * Sim:Tank:Valve:Man을 **30**으로 설정합니다.  
   * **현상:** Sim:Tank:Level이 계속 상승하여 100cm(최대치)에 도달합니다.  
2. **자동 제어 (Auto Mode)**  
   * Sim:Tank:SetPoint를 **50**으로 설정합니다.  
   * Sim:Tank:Mode를 \*\*1 (Auto)\*\*로 변경합니다.  
   * **현상:** PID 제어기가 밸브를 조절하여 수위가 50cm 근처로 수렴하고 안정화됩니다.  
3. **외란 테스트 (Disturbance)**  
   * Sim:Tank:Outflow를 **20**으로 설정합니다 (누수 발생).  
   * **현상:** 수위가 일시적으로 떨어지지만, 제어기가 이를 감지하고 밸브(Valve:Act)를 더 열어 다시 50cm를 유지합니다.

## **2\. 진공 배기 시스템 (vacuum.db)**

**특징:** 시퀀스 제어, 인터락, 물리적 한계($10^{-12}$ Torr) 및 로그 스케일 변환을 구현했습니다.

\# \------------------------------------------------------------------  
\# EPICS Virtual Vacuum Pumping Simulator (v2)  
\# Macro: $(P) \= Prefix, $(R) \= Device Name  
\# \------------------------------------------------------------------

\# \[User Controls\]  
record(bo, "$(P)$(R)RP:Sw") {  
    field(DESC, "Roughing Pump Switch")  
    field(ZNAM, "Off")  
    field(ONAM, "On")  
}  
record(bo, "$(P)$(R)TMP:Sw") {  
    field(DESC, "Turbo Pump Switch")  
    field(ZNAM, "Off")  
    field(ONAM, "On")  
}  
record(ao, "$(P)$(R)Leak:Rate") {  
    field(DESC, "Leak Rate")  
    field(VAL,  "0.001")  
    field(PREC, "4")  
}  
record(bo, "$(P)$(R)Reset") {  
    field(DESC, "Vent to Atmosphere")  
    field(ZNAM, "Idle")  
    field(ONAM, "Vent")  
}

\# \[Interlock & Status\]  
record(calc, "$(P)$(R)TMP:Permit") {  
    field(DESC, "Allow TMP Start if P \< 1.0")  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)Pres:Sim NPP")  
    field(CALC, "A \< 1.0 ? 1 : 0")  
}  
record(calc, "$(P)$(R)TMP:Sts") {  
    field(DESC, "Actual TMP Run Status")  
    field(INPA, "$(P)$(R)TMP:Sw NPP")  
    field(INPB, "$(P)$(R)TMP:Permit PP")  
    field(CALC, "A && B")  
}

\# \[Physics Model\]  
record(calc, "$(P)$(R)Pres:Sim") {  
    field(DESC, "Vacuum Pressure Model")  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)Pres:Sim NPP")  
    field(INPB, "$(P)$(R)RP:Sw NPP")  
    field(INPC, "$(P)$(R)TMP:Sts PP")  
    field(INPD, "$(P)$(R)Leak:Rate NPP")  
    field(INPE, "$(P)$(R)Reset NPP")  
      
    \# Logic: Reset \-\> TMP(limit 1e-12) \-\> RP(limit 0.01) \-\> Leak  
    field(CALC, "E ? 760 : (C ? MAX(1e-12, A\*0.7) : (B ? MAX(0.01, A\*0.95) : A+(760-A)\*D))")  
    field(PREC, "12")  
    field(EGU,  "Torr")  
}

record(calc, "$(P)$(R)Pres:Log") {  
    field(DESC, "Log10 Pressure")  
    field(SCAN, ".5 second")  
    field(INPA, "$(P)$(R)Pres:Sim NPP")  
    \# Using LOG (Base-10) and preventing log(0)  
    field(CALC, "LOG(MAX(1e-13, A))")   
    field(PREC, "2")  
}

### **\[Test Scenario 2\] 진공 배기 실습**

1. **인터락 확인 (Interlock Check)**  
   * 초기 대기압(760 Torr) 상태에서 Sim:Vac:TMP:Sw를 **On**으로 켭니다.  
   * **현상:** Sim:Vac:TMP:Permit이 0이므로, 실제 상태 Sim:Vac:TMP:Sts는 \*\*0(Off)\*\*을 유지합니다. 압력은 변하지 않습니다.  
2. **정상 배기 절차 (Normal Sequence)**  
   * Sim:Vac:RP:Sw를 **On**으로 켭니다.  
   * **현상:** 압력이 760에서 0.01 Torr 근처까지 빠르게 떨어집니다.  
   * 1.0 Torr 밑으로 내려가면 Permit이 **1**로 바뀝니다.  
   * 이때 Sim:Vac:TMP:Sw를 **On**으로 켭니다.  
   * **현상:** 압력이 순식간에 고진공 영역($10^{-12}$ Torr)으로 떨어집니다.  
3. **누설 테스트 (Leak Check)**  
   * 모든 펌프(RP, TMP)를 **Off**로 끕니다.  
   * **현상:** Sim:Vac:Leak:Rate 설정값에 따라 진공도가 서서히 깨지며 압력이 상승합니다.

## **3\. 모션 제어 (motor.db)**

**특징:** 목표 위치 추종 및 S-Curve 가감속(관성 계수 적용)을 시뮬레이션합니다.

\# \------------------------------------------------------------------  
\# EPICS Virtual Motor with Soft Acceleration  
\# Macro: $(P) \= Prefix, $(R) \= Device Name  
\# \------------------------------------------------------------------

\# \[Settings\]  
record(ao, "$(P)$(R)SetPoint") {  
    field(DESC, "Target Position")  
    field(VAL,  "0.0")  
    field(DRVL, "0")  
    field(DRVH, "200")  
    field(EGU,  "mm")  
    field(PINI, "YES")  
}  
record(ao, "$(P)$(R)MaxVel") {  
    field(DESC, "Max Speed Limit")  
    field(VAL,  "10.0")  
    field(EGU,  "mm/s")  
    field(PINI, "YES")  
}  
record(ao, "$(P)$(R)Accel") {  
    field(DESC, "Inertia Factor (0.0\~1.0)")  
    field(VAL,  "0.05")   
    field(PINI, "YES")  
}  
record(bo, "$(P)$(R)Enable") {  
    field(DESC, "Servo On/Off")  
    field(ZNAM, "Disable")  
    field(ONAM, "Enable")  
    field(VAL,  "0")  
}

\# \[Physics Engine\]  
record(calc, "$(P)$(R)Vel:Act") {  
    field(DESC, "Calculated Soft Velocity")  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)SetPoint NPP")  
    field(INPB, "$(P)$(R)Pos NPP")  
    field(INPC, "$(P)$(R)MaxVel NPP")  
    field(INPD, "$(P)$(R)Accel NPP")  
    field(INPE, "$(P)$(R)Vel:Act NPP")  
    field(INPF, "$(P)$(R)Enable NPP")  
      
    \# Velocity with Inertia Filter  
    field(CALC, "F ? (E \+ (MAX(-C, MIN(C, (A-B))) \- E) \* D) : 0")  
    field(EGU,  "mm/s")  
    field(PREC, "3")  
    field(FLNK, "$(P)$(R)Pos")  
}

record(calc, "$(P)$(R)Pos") {  
    field(DESC, "Current Position")  
    field(INPA, "$(P)$(R)Pos NPP")  
    field(INPB, "$(P)$(R)Vel:Act NPP")  
    field(CALC, "A \+ (B \* 0.1)")  
    field(EGU,  "mm")  
    field(PREC, "2")  
    field(FLNK, "$(P)$(R)DMOV")  
}

\# \[Status\]  
record(calc, "$(P)$(R)DMOV") {  
    field(DESC, "Done Moving Status")  
    field(INPA, "$(P)$(R)Vel:Act NPP")  
    field(INPB, "$(P)$(R)Pos NPP")  
    field(INPC, "$(P)$(R)SetPoint NPP")  
    \# Done if Velocity \~ 0 and Position Reached  
    field(CALC, "(ABS(A)\<0.01) && (ABS(B-C)\<0.1)")  
}

### **\[Test Scenario 3\] 모션 제어 실습**

1. **기본 이동 (Basic Move)**  
   * Sim:Motor:Enable을 \*\*1 (Enable)\*\*로 설정합니다.  
   * Sim:Motor:SetPoint를 **100**으로 설정합니다.  
   * **현상:** Sim:Motor:Pos가 0에서 100까지 이동합니다. 이때 Sim:Motor:Vel:Act가 서서히 증가했다가(가속), 10.0(최대속도)을 유지하고, 목표 근처에서 서서히 감소(감속)하는 **S-Curve** 패턴을 관찰합니다.  
2. **관성 테스트 (Inertia Effect)**  
   * Sim:Motor:Accel 값을 **0.01**로 낮춥니다.  
   * Sim:Motor:SetPoint를 **0**으로 변경합니다.  
   * **현상:** 속도가 매우 천천히 오르고 내립니다. 무거운 물체를 끄는 것처럼 둔탁한 움직임을 보입니다.

## **4\. 빔 궤도 피드백 (orbit.db)**

**특징:** MIMO(다입력 다출력) 시스템, 커플링(Coupling) 현상 및 노이즈 보정 제어를 구현했습니다.

\# \------------------------------------------------------------------  
\# EPICS Beam Orbit Feedback Simulator (MIMO System)  
\# Macro: $(P) \= Prefix, $(R) \= Device Name  
\# \------------------------------------------------------------------

\# \[Environment: Noise\]  
record(calc, "$(P)$(R)Noise:Src") {  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)Noise:Tick NPP")  
    field(INPB, "$(P)$(R)Noise:Amp NPP")  
    field(CALC, "(SIN(A/10)\*2.0 \+ (RNDM-0.5)\*0.5) \* B")  
    field(FLNK, "$(P)$(R)Noise:Tick")  
}  
record(calc, "$(P)$(R)Noise:Tick") {  
    field(INPA, "$(P)$(R)Noise:Tick NPP")  
    field(CALC, "(A+1)%63")  
}  
record(ao, "$(P)$(R)Noise:Amp") {  
    field(VAL,  "1.0")  
    field(PINI, "YES")  
}

\# \[Magnets & Sensors with Coupling\]  
record(ao, "$(P)$(R)CM1:Set") {  
    field(VAL, "0")  
    field(DRVL, "-10")  
    field(DRVH, "10")  
}  
record(ao, "$(P)$(R)CM2:Set") {  
    field(VAL, "0")  
    field(DRVL, "-10")  
    field(DRVH, "10")  
}

record(calc, "$(P)$(R)BPM1:Pos") {  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)CM1:Set NPP")  
    field(INPB, "$(P)$(R)CM2:Set NPP")  
    field(INPC, "$(P)$(R)Noise:Src NPP")  
    \# Coupling: CM1 dominates, CM2 effects 30%  
    field(CALC, "(A \* 1.0) \+ (B \* 0.3) \+ C")  
}  
record(calc, "$(P)$(R)BPM2:Pos") {  
    field(SCAN, ".1 second")  
    field(INPA, "$(P)$(R)CM1:Set NPP")  
    field(INPB, "$(P)$(R)CM2:Set NPP")  
    field(INPC, "$(P)$(R)Noise:Src NPP")  
    \# Coupling: CM2 dominates, CM1 effects 20%  
    field(CALC, "(A \* 0.2) \+ (B \* 0.8) \+ C")  
}

\# \[Feedback Controller\]  
record(bo, "$(P)$(R)FB:Run") {  
    field(ZNAM, "Open Loop")  
    field(ONAM, "Closed Loop")  
}  
record(ao, "$(P)$(R)FB:Gain") {  
    field(VAL,  "0.5")   
}

record(calc, "$(P)$(R)FB:Calc1") {  
    field(SCAN, ".2 second")  
    field(INPA, "$(P)$(R)BPM1:Pos NPP")  
    field(INPB, "$(P)$(R)CM1:Set NPP")  
    field(INPC, "$(P)$(R)FB:Gain NPP")  
    field(INPD, "$(P)$(R)FB:Run NPP")  
    \# Integral Action  
    field(CALC, "D ? (B \- (A \* C)) : B")  
    field(OUT,  "$(P)$(R)CM1:Set PP")  
}

record(calc, "$(P)$(R)FB:Calc2") {  
    field(SCAN, ".2 second")  
    field(INPA, "$(P)$(R)BPM2:Pos NPP")  
    field(INPB, "$(P)$(R)CM2:Set NPP")  
    field(INPC, "$(P)$(R)FB:Gain NPP")  
    field(INPD, "$(P)$(R)FB:Run NPP")  
    field(CALC, "D ? (B \- (A \* C)) : B")  
    field(OUT,  "$(P)$(R)CM2:Set PP")  
}

### **\[Test Scenario 4\] 빔 궤도 피드백 실습**

1. **오픈 루프 관찰 (Open Loop)**  
   * Sim:Orbit:FB:Run이 \*\*0 (Open Loop)\*\*인 상태에서 시작합니다.  
   * **현상:** BPM1:Pos, BPM2:Pos 값이 노이즈에 의해 ±2\~3mm 범위로 계속 흔들립니다.  
2. **커플링 확인 (Coupling Check)**  
   * Sim:Orbit:CM1:Set을 **2.0**으로 변경합니다.  
   * **현상:** BPM1뿐만 아니라 BPM2 값도 같이 변하는(간섭) 현상을 확인합니다.  
3. **피드백 제어 (Closed Loop)**  
   * Sim:Orbit:FB:Run을 \*\*1 (Closed Loop)\*\*로 변경합니다.  
   * **현상:** CM1, CM2 전류값이 빠르게 변하면서 BPM 값들을 0.0mm 근처로 강력하게 고정시킵니다. 노이즈가 있어도 중앙을 유지하려는 복원력을 확인합니다.

## **5\. 실행 스크립트 (st.cmd)**

위의 DB 파일들을 db 폴더에 저장한 후, 이 스크립트로 IOC를 실행하면 4가지 시스템이 동시에 구동됩니다.

\#\!../../bin/linux-x86\_64/softIoc

\# 1\. 환경 설정  
\< envPaths

cd "${TOP}"

\# 2\. 데이터베이스 정의 파일 로드  
dbLoadDatabase "dbd/softIoc.dbd"  
softIoc\_registerRecordDeviceDriver pdbbase

\# 3\. 가상 시스템 DB 로드  
\# 매크로: P=Prefix, R=Region/Device

\# (1) 수조 제어 (Sim:Tank:...)  
dbLoadRecords("db/tank.db", "P=Sim:, R=Tank:")

\# (2) 진공 시스템 (Sim:Vac:...)  
dbLoadRecords("db/vacuum.db", "P=Sim:, R=Vac:")

\# (3) 모터 제어 (Sim:Motor:...)  
dbLoadRecords("db/motor.db", "P=Sim:, R=Motor:")

\# (4) 빔 궤도 (Sim:Orbit:...)  
dbLoadRecords("db/orbit.db", "P=Sim:, R=Orbit:")

\# 4\. IOC 시작  
cd "${TOP}/iocBoot/${IOC}"  
iocInit

\# 5\. 초기 설정 (옵션)  
\# 모터 활성화  
dbpf Sim:Motor:Enable 1  
\# 수조 목표값 설정  
dbpf Sim:Tank:SetPoint 50  
