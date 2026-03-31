import sys
import epics
import numpy as np

def main():
    # 1. 인자 개수 확인 (스크립트명, 입력PV, 출력PV, 연산자, 숫자 -> 총 5개)
    if len(sys.argv) != 5:
        print("사용법: python3 calc_waveform.py [입력PV] [출력PV] [연산자] [숫자]")
        print("연산자: ADD(+), SUB(-), MUL(*), DIV(/)")
        print("예시: python3 calc_waveform.py IN:WAVEFORM OUT:WAVEFORM ADD 3.5")
        sys.exit(1)

    # 2. 인자 파싱
    input_pv = sys.argv[1]           # 첫 번째 인자: 데이터를 읽어올 PV
    output_pv = sys.argv[2]          # 두 번째 인자: 결과를 저장할 PV
    operation = sys.argv[3].upper()  # 세 번째 인자: 연산자
    
    try:
        operand = float(sys.argv[4]) # 네 번째 인자: 연산할 숫자
    except ValueError:
        print("오류: 네 번째 인자(숫자)는 반드시 숫자 형식이어야 합니다.")
        sys.exit(1)

    # 3. EPICS Waveform 데이터 읽기 (caget)
    print(f">>> [{input_pv}]에서 데이터를 읽는 중...")
    waveform_data = epics.caget(input_pv)

    if waveform_data is None:
        print(f"오류: [{input_pv}] 연결에 실패했거나 데이터를 읽을 수 없습니다.")
        sys.exit(1)

    print(f"원본 데이터 (앞 5개): {waveform_data[:5]}")
    print(f"수행할 연산: {operation}, 적용할 값: {operand}")

    # 4. 사칙 연산 수행 (NumPy 브로드캐스팅)
    if operation == "ADD":
        result = waveform_data + operand
    elif operation == "SUB":
        result = waveform_data - operand
    elif operation == "MUL":
        result = waveform_data * operand
    elif operation == "DIV":
        if operand == 0:
            print("오류: 0으로 나눌 수 없습니다.")
            sys.exit(1)
        result = waveform_data / operand
    else:
        print("오류: 지원하지 않는 연산자입니다. (지원: ADD, SUB, MUL, DIV)")
        sys.exit(1)

    # 5. 연산 결과 출력
    print(f"연산 결과 (앞 5개): {result[:5]}")
    
    # 6. 결과를 출력 PV에 쓰기 (caput)
    print(f">>> [{output_pv}]에 계산된 데이터를 쓰는 중...")
    put_success = epics.caput(output_pv, result)
    
    if put_success:
        print(f"성공: [{output_pv}]에 데이터 쓰기 완료!")
    else:
        print(f"실패: [{output_pv}]에 데이터를 쓰는 데 실패했습니다. PV 이름이나 권한을 확인해 주세요.")

if __name__ == "__main__":
    main()
