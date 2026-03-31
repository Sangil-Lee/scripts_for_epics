import sys
import epics
import numpy as np

def print_usage():
    print("=" * 75)
    print(" 🌊 EPICS Waveform Calculator & Generator (calc_waveform.py)")
    print("=" * 75)
    print("사용법: python3 calc_waveform.py [입력PV] [출력PV] [범위] [연산자] [값(선택)]\n")
    
    print("[1. 범위(Range) 지정 방법]")
    print("  - ALL     : 배열 전체에 적용")
    print("  - 시작:끝 : 특정 인덱스 구간 적용 (예: 10:50 -> 10~49번째 인덱스)")
    print("  - :끝     : 처음부터 특정 인덱스까지 (예: :50 -> 0~49번째 인덱스)")
    print("  - 시작:   : 특정 인덱스부터 끝까지 (예: 50: -> 50번째부터 끝까지)\n")

    print("[2. 연산자 종류 및 사용 예시]")
    
    print("  ▶ 기본 사칙연산 (값 필수)")
    print("    - ADD, SUB, MUL, DIV : 더하기, 빼기, 곱하기, 나누기")
    print("      예) python3 calc_waveform.py IN OUT ALL ADD 3.5      (전체에 3.5 더하기)")
    print("      예) python3 calc_waveform.py IN OUT 0:100 MUL 2.0    (0~99번에 2 곱하기)")
    print("    - INC, DEC : 지정한 값만큼 증가/감소")
    print("      예) python3 calc_waveform.py IN OUT ALL DEC 10       (전체 10 감소)\n")

    print("  ▶ 데이터 생성 (Generation)")
    print("    - RAND:min:max    : 지정한 범위의 난수로 덮어쓰기")
    print("      예) python3 calc_waveform.py IN OUT ALL RAND             (1~100 난수)")
    print("      예) python3 calc_waveform.py IN OUT ALL RAND:50          (0~50 난수)")
    print("      예) python3 calc_waveform.py IN OUT 10:20 RAND:-5:5      (-5~5 난수)")
    print("    - RAMP:start:stop : 선형 증가/감소(사선) 파형 생성")
    print("      예) python3 calc_waveform.py IN OUT ALL RAMP:0:10        (0에서 10으로 선형 증가)")
    print("    - SINE:cycle:amp  : 사인파 생성 (주기 횟수, 진폭)")
    print("      예) python3 calc_waveform.py IN OUT ALL SINE:3:10        (3주기, 진폭 10의 사인파)\n")

    print("  ▶ 신호 처리 및 필터링 (Processing)")
    print("    - NOISE:max_val   : 기존 데이터에 -max_val ~ +max_val 범위의 랜덤 노이즈 추가")
    print("      예) python3 calc_waveform.py IN OUT ALL NOISE 0.5        (±0.5 노이즈 추가)")
    print("    - SMOOTH:window   : 이동 평균(Moving Average) 필터로 노이즈 억제")
    print("      예) python3 calc_waveform.py IN OUT ALL SMOOTH:5         (주변 5개 데이터 평균)")
    print("    - LIMIT:min:max   : 지정한 상/하한선으로 데이터 자르기 (클램핑 안전선)")
    print("      예) python3 calc_waveform.py IN OUT ALL LIMIT:0:100      (0 미만은 0, 100 초과는 100으로)")
    print("    - NORM            : 데이터를 0.0 ~ 1.0 사이로 정규화 (Min-Max 스케일링)")
    print("      예) python3 calc_waveform.py IN OUT ALL NORM\n")

    print("  ▶ 기본 수학 함수 (Math)")
    print("    - ABS  : 모든 음수 값을 양수로 변환 (절대값)")
    print("      예) python3 calc_waveform.py IN OUT ALL ABS")
    print("    - SQRT : 배열의 제곱근 계산 (음수는 절대값 처리 후 계산)")
    print("      예) python3 calc_waveform.py IN OUT ALL SQRT")
    print("=" * 75)

def main():
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print_usage()
        sys.exit(1)

    input_pv = sys.argv[1]
    output_pv = sys.argv[2]
    range_str = sys.argv[3].upper()
    operation = sys.argv[4].upper()
    
    operand = 0.0
    if len(sys.argv) == 6:
        try:
            operand = float(sys.argv[5])
        except ValueError:
            print("오류: 마지막 인자(값)는 숫자여야 합니다.")
            sys.exit(1)

    print(f">>> [{input_pv}] 데이터 읽는 중...")
    waveform_data = epics.caget(input_pv)

    if waveform_data is None:
        print(f"오류: [{input_pv}] 연결 실패 또는 데이터 없음.")
        sys.exit(1)

    result = np.copy(waveform_data)
    total_len = len(result)

    # 범위 파싱
    if range_str == "ALL":
        start, end = 0, total_len
    else:
        try:
            parts = range_str.split(':')
            start = int(parts[0]) if parts[0] else 0
            end = int(parts[1]) if len(parts) > 1 and parts[1] else total_len
        except ValueError:
            print("오류: 범위 형식이 잘못되었습니다.")
            sys.exit(1)

    start = max(0, start)
    end = min(total_len, end)
    size = end - start

    if size <= 0:
        print("오류: 유효하지 않은 인덱스 범위입니다.")
        sys.exit(1)

    target_slice = result[start:end]

    # 연산자 파싱 (콜론 기준으로 분리)
    op_parts = operation.split(':')
    base_op = op_parts[0]

    # --- 1. 기본 사칙 연산 ---
    if base_op == "ADD" or base_op == "INC": target_slice += operand
    elif base_op == "SUB" or base_op == "DEC": target_slice -= operand
    elif base_op == "MUL": target_slice *= operand
    elif base_op == "DIV":
        if operand == 0:
            print("오류: 0으로 나눌 수 없습니다.")
            sys.exit(1)
        target_slice /= operand
        
    # --- 2. 데이터 생성 (Generate) ---
    elif base_op == "RAND":
        r_min = float(op_parts[1]) if len(op_parts) > 1 else 1.0
        r_max = float(op_parts[2]) if len(op_parts) > 2 else 100.0
        if len(op_parts) == 2: r_min, r_max = 0.0, float(op_parts[1])
        target_slice[:] = np.random.uniform(r_min, r_max, size)
        
    elif base_op == "RAMP":
        # RAMP:시작값:종료값 (기본값 0 ~ 100)
        start_v = float(op_parts[1]) if len(op_parts) > 1 else 0.0
        stop_v = float(op_parts[2]) if len(op_parts) > 2 else 100.0
        target_slice[:] = np.linspace(start_v, stop_v, size)
        
    elif base_op == "SINE":
        # SINE:주기:진폭 (기본 주기 1, 진폭 1)
        cycles = float(op_parts[1]) if len(op_parts) > 1 else 1.0
        amp = float(op_parts[2]) if len(op_parts) > 2 else 1.0
        x_vals = np.linspace(0, cycles * 2 * np.pi, size)
        target_slice[:] = np.sin(x_vals) * amp

    # --- 3. 신호 처리 (Processing) ---
    elif base_op == "NOISE":
        noise = np.random.uniform(-operand, operand, size)
        target_slice += noise
        
    elif base_op == "SMOOTH":
        # 이동 평균(Moving Average) 창 크기 설정
        window = int(op_parts[1]) if len(op_parts) > 1 else int(operand)
        if window < 2: window = 3
        box = np.ones(window) / window
        # np.convolve로 평활화 진행 (mode='same'으로 배열 크기 유지)
        target_slice[:] = np.convolve(target_slice, box, mode='same')
        
    elif base_op == "LIMIT":
        # 상하한선 클램핑
        l_min = float(op_parts[1]) if len(op_parts) > 1 else -operand
        l_max = float(op_parts[2]) if len(op_parts) > 2 else operand
        target_slice[:] = np.clip(target_slice, l_min, l_max)
        
    elif base_op == "NORM":
        # 0 ~ 1 정규화
        min_val = np.min(target_slice)
        max_val = np.max(target_slice)
        if max_val > min_val:
            target_slice[:] = (target_slice - min_val) / (max_val - min_val)
        else:
            target_slice[:] = 0.0 # 모든 값이 같을 경우 0으로 초기화

    # --- 4. 기본 수학 함수 (Math) ---
    elif base_op == "ABS":
        target_slice[:] = np.abs(target_slice)
    elif base_op == "SQRT":
        # 음수의 제곱근 에러 방지를 위해 절대값 취한 후 계산
        target_slice[:] = np.sqrt(np.abs(target_slice))
        
    else:
        print(f"오류: 지원하지 않는 연산자입니다. ({base_op})")
        sys.exit(1)

    # 5. 결과 출력 및 쓰기
    print(f"--- 연산 완료 ---")
    print(f"수행: {operation}, 적용 범위: {start}~{end-1} ({size}개)")
    print(f"결과 미리보기: {target_slice[:5]} ...") 
    
    print(f">>> [{output_pv}] 쓰기 중...")
    if epics.caput(output_pv, result):
        print("성공!")
    else:
        print("실패: PV 상태 확인 필요.")

if __name__ == "__main__":
    main()
