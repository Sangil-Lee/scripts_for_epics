/*
 * aSubExamples.cpp
 *
 * EPICS aSub 레코드 초급자 교육용 예제 모음 (Modern C++)
 *
 * aSub 레코드란?
 *   - 사용자 정의 C/C++ 함수를 EPICS 레코드로 호출할 수 있는 범용 레코드
 *   - INAM : IOC 기동 시 1회 호출되는 초기화 함수
 *   - SNAM : 레코드가 처리(process)될 때마다 호출되는 실행 함수
 *   - INPA~INPU (21개) : 입력 필드 (다른 PV 값을 읽어옴)
 *   - VALA~VALU (21개) : 출력 필드 (계산 결과를 저장)
 *   - FTA~FTU / FTVA~FTVU : 각 입출력의 데이터 타입 (DOUBLE, LONG, STRING 등)
 *   - NOA~NOU / NOVA~NOVU : 배열일 경우 요소 개수
 *   - 반환값: 0=정상, -1=에러 (레코드 알람 발생)
 *
 * ============================================================================
 *  예제 목록
 * ============================================================================
 *
 *  [예제 1] addTwoValues     — 두 수의 덧셈 (가장 기본적인 aSub 사용법)
 *  [예제 2] linearScale      — 1차 함수 변환 (y = ax + b, 센서 보정 패턴)
 *  [예제 3] arrayAccumulator — 배열 누적 카운터 (매 호출마다 1씩 증가)
 *
 *  시험:
 *    camonitor TEST:AccumWf
 *    # 1초마다 배열의 각 요소가 1씩 증가하는 것을 확인
 *    #   [1,1,1,...] → [2,2,2,...] → [3,3,3,...] ...
 *
 *    # 리셋
 *    caput TEST:AccReset 1
 *    # 다음 처리 시 배열이 [0,0,0,...] 으로 초기화
 *    caput TEST:AccReset 0
 *    # 다시 1씩 증가 시작
 *
 * ============================================================================
 */

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <numeric>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <errlog.h>

extern "C" {

/* ====================================================================
 *  예제 1: addTwoValues — 두 수의 덧셈
 * ====================================================================
 *
 *  가장 기본적인 aSub 사용법.
 *  입력 2개를 읽어서 더한 결과를 출력 1개에 쓴다.
 *
 *  입력: INPA(DOUBLE) = A,  INPB(DOUBLE) = B
 *  출력: VALA(DOUBLE) = A + B
 */
static long addTwoValues(aSubRecord *prec)
{
    /* 입력 필드에서 값 읽기: prec->a 는 void* 이므로 캐스팅 필요 */
    auto a = *static_cast<epicsFloat64 *>(prec->a);
    auto b = *static_cast<epicsFloat64 *>(prec->b);

    /* 계산 */
    auto result = a + b;

    /* 출력 필드에 결과 쓰기 */
    *static_cast<epicsFloat64 *>(prec->vala) = result;

    errlogPrintf("addTwoValues: %.2f + %.2f = %.2f\n", a, b, result);
    return 0;  /* 0 = 정상 종료 */
}


/* ====================================================================
 *  예제 2: linearScale — 1차 함수 변환 (y = a*x + b)
 * ====================================================================
 *
 *  센서 raw 값을 공학 단위로 변환하는 가장 흔한 패턴.
 *  예: ADC 카운트 → 온도(℃), 전압 → 압력(bar)
 *
 *  입력: INPA(DOUBLE) = x (raw 값)
 *        INPB(DOUBLE) = a (기울기/스케일)
 *        INPC(DOUBLE) = b (오프셋)
 *  출력: VALA(DOUBLE) = a * x + b
 */
static long linearScale(aSubRecord *prec)
{
    auto x = *static_cast<epicsFloat64 *>(prec->a);
    auto a = *static_cast<epicsFloat64 *>(prec->b);
    auto b = *static_cast<epicsFloat64 *>(prec->c);

    auto y = a * x + b;

    *static_cast<epicsFloat64 *>(prec->vala) = y;

    errlogPrintf("linearScale: y = %.4f * %.4f + %.4f = %.4f\n", a, x, b, y);
    return 0;
}
/* ====================================================================
 *  예제 3: arrayAccumulator — 배열 누적 카운터
 * ====================================================================
 *
 *  aSub 의 INAM + SNAM 조합 예제.
 *  VALA 배열이 매 호출마다 요소별로 1.0씩 증가한다.
 *  INPA 가 1(Reset) 이면 배열을 0 으로 초기화.
 *
 *  핵심 포인트:
 *    - VALA 는 aSub 레코드 내부에 영속적으로 유지된다.
 *    - SNAM 이 호출될 때마다 이전 VALA 값 위에 덧셈이 누적된다.
 *    - INAM 은 IOC 기동 시 1회만 호출 → 초기 상태 세팅용.
 *
 *  입력: INPA(LONG) = 리셋 플래그 (0=유지, 1=리셋)
 *  출력: VALA(DOUBLE[], NOVA) = 누적 카운터 배열
 */
static long arrayAccumulatorInit(aSubRecord *prec)
{
    auto *out = static_cast<epicsFloat64 *>(prec->vala);
    auto  n   = static_cast<long>(prec->nova);

    /* 배열 전체를 0.0 으로 초기화 */
    std::fill_n(out, n, 0.0);

    errlogPrintf("arrayAccumulatorInit: %ld elements zeroed\n", n);
    return 0;
}

static long arrayAccumulatorProc(aSubRecord *prec)
{
    auto  reset = *static_cast<epicsInt32 *>(prec->a);
    auto *out   = static_cast<epicsFloat64 *>(prec->vala);
    auto  n     = static_cast<long>(prec->nova);

    if (reset) {
        /* 리셋 요청: 배열 초기화 */
        std::fill_n(out, n, 0.0);
        errlogPrintf("arrayAccumulatorProc: reset\n");
    } else {
        /* 각 요소에 1.0 씩 누적 — std::transform 으로 in-place 변환 */
        std::transform(out, out + n, out,
                       [](auto x) { return x + 1.0; });
    }

    return 0;
}


epicsRegisterFunction(addTwoValues);
epicsRegisterFunction(linearScale);
epicsRegisterFunction(arrayAccumulatorInit);
epicsRegisterFunction(arrayAccumulatorProc);

} /* extern "C" */
