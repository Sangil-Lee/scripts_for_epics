/*
 * calcWaveformASub.cpp
 *
 * EPICS aSub record implementation for waveform slice operations.
 * Equivalent to calc_waveform_slice_pwm_init.py
 *
 * ============================================================================
 *  aSub Record Field Mapping
 * ============================================================================
 *   INPA  : Input waveform          (DOUBLE array, NOA)
 *   INPB  : Operation name          (STRING, e.g. "INIT","ADD","PWM","SINE")
 *   INPC  : Range start index       (LONG,   0-based, default 0)
 *   INPD  : Range end index         (LONG,   exclusive, <=0 means full length)
 *   INPE  : Operand / value         (DOUBLE, used by INIT/ADD/SUB/MUL/DIV/INC/DEC/NOISE)
 *   INPF  : Param1                  (DOUBLE, cycles/min/window depending on op)
 *   INPG  : Param2                  (DOUBLE, duty/max/amp depending on op)
 *   VALA  : Output waveform         (DOUBLE array, NOVA)
 *
 * ============================================================================
 *  IOC Shell 시험 검증 방법 (caput / caget 사용)
 * ============================================================================
 *
 *  공통 매크로: P=TEST
 *  처리 실행:  caput TEST:CalcWf.PROC 1
 *  결과 확인:  caget -# 10 TEST:OutputWf   (앞 10개 요소 출력)
 *
 * --------------------------------------------------------------------------
 *  [1] INIT  — 전체 배열을 특정 값으로 초기화
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "INIT"
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0          # 0 = 전체 범위
 *    caput TEST:Operand    5.0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 5.0 5.0 5.0 ...
 *
 *    # 슬라이스 초기화 (인덱스 100~200만 99.0으로)
 *    caput TEST:RangeStart 100
 *    caput TEST:RangeEnd   200
 *    caput TEST:Operand    99.0
 *    caput TEST:CalcWf.PROC 1
 *
 * --------------------------------------------------------------------------
 *  [2] ADD / INC — 값 더하기
 * --------------------------------------------------------------------------
 *    # 사전: INIT 로 전체를 10.0 으로 세팅
 *    caput TEST:OpName     "ADD"
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:Operand    3.0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 13.0 13.0 13.0 ...
 *
 * --------------------------------------------------------------------------
 *  [3] SUB / DEC — 값 빼기
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "SUB"
 *    caput TEST:Operand    2.0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 11.0 11.0 11.0 ...
 *
 * --------------------------------------------------------------------------
 *  [4] MUL — 곱하기
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "MUL"
 *    caput TEST:Operand    2.0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 이전값 * 2.0
 *
 * --------------------------------------------------------------------------
 *  [5] DIV — 나누기
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "DIV"
 *    caput TEST:Operand    4.0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 이전값 / 4.0
 *
 *    # 에러 케이스: 0으로 나누기 → 반환값 -1, errlog 출력
 *    caput TEST:Operand    0.0
 *    caput TEST:CalcWf.PROC 1         # 에러 로그 확인
 *
 * --------------------------------------------------------------------------
 *  [6] RAND — 난수 생성
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "RAND"
 *    caput TEST:Param1     10.0       # 최솟값
 *    caput TEST:Param2     90.0       # 최댓값
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 10.0 ~ 90.0 사이 난수
 *
 *    # 기본값 (Param1=0, Param2=0 이면 0.0~100.0)
 *    caput TEST:Param1     0.0
 *    caput TEST:Param2     0.0
 *    caput TEST:CalcWf.PROC 1
 *
 * --------------------------------------------------------------------------
 *  [7] RAMP — 선형 증가/감소 파형
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "RAMP"
 *    caput TEST:Param1     0.0        # 시작값
 *    caput TEST:Param2     100.0      # 종료값
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 0.0, 0.098, 0.196, ... → 100.0
 *
 *    # 역방향 램프
 *    caput TEST:Param1     100.0
 *    caput TEST:Param2     0.0
 *    caput TEST:CalcWf.PROC 1
 *
 * --------------------------------------------------------------------------
 *  [8] SINE — 사인파 생성
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "SINE"
 *    caput TEST:Param1     3.0        # 주기 횟수 (cycles)
 *    caput TEST:Param2     5.0        # 진폭 (amplitude)
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 3주기 사인파, 범위 -5.0 ~ +5.0
 *
 *    # 기본값 (Param1=0→1주기, Param2=0→진폭1.0)
 *    caput TEST:Param1     0.0
 *    caput TEST:Param2     0.0
 *    caput TEST:CalcWf.PROC 1
 *
 * --------------------------------------------------------------------------
 *  [9] PWM — 사각파(PWM) 생성
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "PWM"
 *    caput TEST:Param1     5.0        # 주기 횟수 (cycles)
 *    caput TEST:Param2     30.0       # 듀티비 (%)
 *    caput TEST:Operand    1.0        # HIGH 레벨 값 (0이면 기본 1.0)
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 20 TEST:OutputWf        # 기대: 30% HIGH(1.0), 70% LOW(0.0) × 5주기
 *
 *    # 진폭 변경
 *    caput TEST:Operand    3.3        # HIGH = 3.3V
 *    caput TEST:CalcWf.PROC 1
 *
 * --------------------------------------------------------------------------
 *  [10] NOISE — 기존 데이터에 랜덤 노이즈 추가
 * --------------------------------------------------------------------------
 *    # 사전: RAMP 또는 SINE 으로 기본 파형 생성 후
 *    caput TEST:OpName     "NOISE"
 *    caput TEST:Operand    0.5        # 노이즈 범위: ±0.5
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 원래 값 ± 0.5 범위 내 변동
 *
 * --------------------------------------------------------------------------
 *  [11] SMOOTH — 이동 평균 필터 (노이즈 억제)
 * --------------------------------------------------------------------------
 *    # 사전: NOISE 가 추가된 파형에 적용
 *    caput TEST:OpName     "SMOOTH"
 *    caput TEST:Param1     7.0        # 윈도우 크기 (홀수 권장)
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 부드러워진(smoothed) 파형
 *
 *    # Param1 대신 Operand 로도 윈도우 크기 전달 가능
 *    caput TEST:Param1     0.0
 *    caput TEST:Operand    5.0
 *    caput TEST:CalcWf.PROC 1
 *
 * --------------------------------------------------------------------------
 *  [12] LIMIT — 상·하한 클램핑
 * --------------------------------------------------------------------------
 *    caput TEST:OpName     "LIMIT"
 *    caput TEST:Param1     -2.0       # 하한
 *    caput TEST:Param2     2.0        # 상한
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 모든 값이 -2.0 ~ +2.0 범위 내
 *
 *    # 대칭 클램핑 (Param1=0, Param2=0 이면 ±Operand)
 *    caput TEST:Param1     0.0
 *    caput TEST:Param2     0.0
 *    caput TEST:Operand    1.5
 *    caput TEST:CalcWf.PROC 1         # 기대: -1.5 ~ +1.5 클램핑
 *
 * --------------------------------------------------------------------------
 *  [13] NORM — 0.0 ~ 1.0 정규화
 * --------------------------------------------------------------------------
 *    # 사전: 임의 범위의 파형 데이터 존재
 *    caput TEST:OpName     "NORM"
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 최솟값→0.0, 최댓값→1.0
 *
 *    # 모든 값이 동일한 경우 → 전부 0.0
 *
 * --------------------------------------------------------------------------
 *  [14] ABS — 절대값
 * --------------------------------------------------------------------------
 *    # 사전: SINE 등으로 음수가 포함된 파형 생성
 *    caput TEST:OpName     "ABS"
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 모든 값 >= 0.0
 *
 * --------------------------------------------------------------------------
 *  [15] SQRT — 제곱근 (|x| 의 제곱근)
 * --------------------------------------------------------------------------
 *    # 사전: INIT 또는 RAMP 로 양수 파형 생성
 *    caput TEST:OpName     "SQRT"
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: sqrt(|원래값|)
 *
 * --------------------------------------------------------------------------
 *  [16] 슬라이스(Range) 시험 — 부분 영역만 연산 적용
 * --------------------------------------------------------------------------
 *    # 전체를 0으로 초기화 후 인덱스 100~300 구간만 SINE
 *    caput TEST:OpName     "INIT"
 *    caput TEST:Operand    0.0
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1
 *
 *    caput TEST:OpName     "SINE"
 *    caput TEST:Param1     2.0
 *    caput TEST:Param2     1.0
 *    caput TEST:RangeStart 100
 *    caput TEST:RangeEnd   300
 *    caput TEST:CalcWf.PROC 1
 *    # 기대: [0..99]=0.0, [100..299]=사인파, [300..1023]=0.0
 *
 * --------------------------------------------------------------------------
 *  [17] 에러 케이스 시험
 * --------------------------------------------------------------------------
 *    # 잘못된 범위 (start >= end)
 *    caput TEST:RangeStart 500
 *    caput TEST:RangeEnd   100
 *    caput TEST:CalcWf.PROC 1         # 기대: 에러 로그, 반환값 -1
 *
 *    # 알 수 없는 연산자
 *    caput TEST:OpName     "INVALID"
 *    caput TEST:CalcWf.PROC 1         # 기대: 에러 로그 "unknown operation"
 *
 * --------------------------------------------------------------------------
 *  [18] 복합 시험 시나리오 (여러 연산 연속 적용)
 * --------------------------------------------------------------------------
 *    # 1단계: RAMP 생성 (0→100)
 *    caput TEST:OpName "RAMP" && caput TEST:Param1 0 && caput TEST:Param2 100
 *    caput TEST:RangeEnd 0 && caput TEST:CalcWf.PROC 1
 *
 *    # 2단계: NOISE 추가 (±5.0)
 *    caput TEST:OpName "NOISE" && caput TEST:Operand 5.0
 *    caput TEST:CalcWf.PROC 1
 *
 *    # 3단계: SMOOTH 적용 (윈도우=7)
 *    caput TEST:OpName "SMOOTH" && caput TEST:Param1 7
 *    caput TEST:CalcWf.PROC 1
 *
 *    # 4단계: LIMIT 클램핑 (10~90)
 *    caput TEST:OpName "LIMIT" && caput TEST:Param1 10 && caput TEST:Param2 90
 *    caput TEST:CalcWf.PROC 1
 *
 *    # 5단계: NORM 정규화
 *    caput TEST:OpName "NORM"
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWf        # 기대: 0.0 ~ 1.0 범위의 부드러운 램프
 *
 * ============================================================================
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <string>
#include <iterator>
#include <type_traits>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <errlog.h>

namespace {

/* ------------------------------------------------------------------ */
/*  Random engine (thread-local)                                      */
/* ------------------------------------------------------------------ */
std::mt19937 &rng()
{
    static thread_local std::mt19937 gen{std::random_device{}()};
    return gen;
}

/* ------------------------------------------------------------------ */
/*  Operation enum & lookup                                           */
/* ------------------------------------------------------------------ */
enum class Op {
    INIT, ADD, SUB, MUL, DIV, INC, DEC,
    RAND, RAMP, SINE, PWM,
    NOISE, SMOOTH, LIMIT, NORM,
    ABS, SQRT,
    UNKNOWN
};

Op parseOp(const char *s)
{
    struct Entry { const char *name; Op op; };
    static constexpr Entry table[] = {
        {"INIT",  Op::INIT},  {"ADD",  Op::ADD},   {"SUB",   Op::SUB},
        {"MUL",   Op::MUL},   {"DIV",  Op::DIV},   {"INC",   Op::INC},
        {"DEC",   Op::DEC},   {"RAND", Op::RAND},  {"RAMP",  Op::RAMP},
        {"SINE",  Op::SINE},  {"PWM",  Op::PWM},   {"NOISE", Op::NOISE},
        {"SMOOTH",Op::SMOOTH},{"LIMIT",Op::LIMIT}, {"NORM",  Op::NORM},
        {"ABS",   Op::ABS},   {"SQRT", Op::SQRT},
    };
    std::string upper(s);
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    for (const auto &e : table)
        if (upper == e.name) return e.op;
    return Op::UNKNOWN;
}

/* ================================================================== */
/*  Template-based waveform algorithms                                */
/* ================================================================== */

/* --- Arithmetic --------------------------------------------------- */
template <typename Iter, typename T>
void algFill(Iter first, Iter last, T val)
{
    std::fill(first, last, static_cast<typename std::iterator_traits<Iter>::value_type>(val));
}

template <typename Iter, typename T>
void algAdd(Iter first, Iter last, T val)
{
    std::transform(first, last, first,
                   [val](auto x) { return x + val; });
}

template <typename Iter, typename T>
void algSub(Iter first, Iter last, T val)
{
    std::transform(first, last, first,
                   [val](auto x) { return x - val; });
}

template <typename Iter, typename T>
void algMul(Iter first, Iter last, T val)
{
    std::transform(first, last, first,
                   [val](auto x) { return x * val; });
}

template <typename Iter, typename T>
void algDiv(Iter first, Iter last, T val)
{
    std::transform(first, last, first,
                   [val](auto x) { return x / val; });
}

/* --- Generators --------------------------------------------------- */
template <typename Iter>
void algRand(Iter first, Iter last, double lo, double hi)
{
    std::uniform_real_distribution<double> dist(lo, hi);
    std::generate(first, last, [&]() { return dist(rng()); });
}

template <typename Iter>
void algRamp(Iter first, Iter last, double startV, double stopV)
{
    const auto n = std::distance(first, last);
    if (n <= 1) { if (n == 1) *first = startV; return; }
    const double step = (stopV - startV) / static_cast<double>(n - 1);
    double v = startV;
    std::generate(first, last, [&]() { double cur = v; v += step; return cur; });
}

template <typename Iter>
void algSine(Iter first, Iter last, double cycles, double amp)
{
    const auto n = std::distance(first, last);
    if (n <= 0) return;
    const double inc = cycles * 2.0 * M_PI / static_cast<double>(n);
    double phase = 0.0;
    std::generate(first, last, [&]() {
        double val = std::sin(phase) * amp;
        phase += inc;
        return val;
    });
}

template <typename Iter>
void algPwm(Iter first, Iter last, double cycles, double dutyPct, double highVal)
{
    const auto n = std::distance(first, last);
    if (n <= 0) return;
    const double dutyRatio = dutyPct / 100.0;
    const double phaseInc = cycles / static_cast<double>(n);
    double phase = 0.0;
    std::generate(first, last, [&]() {
        double frac = std::fmod(phase, 1.0);
        phase += phaseInc;
        return (frac < dutyRatio) ? highVal : 0.0;
    });
}

/* --- Signal processing -------------------------------------------- */
template <typename Iter>
void algNoise(Iter first, Iter last, double maxNoise)
{
    std::uniform_real_distribution<double> dist(-maxNoise, maxNoise);
    std::transform(first, last, first,
                   [&](auto x) { return x + dist(rng()); });
}

template <typename Iter>
void algSmooth(Iter first, Iter last, int window)
{
    if (window < 2) window = 3;
    const auto n = std::distance(first, last);
    if (n <= 0) return;

    /* copy original data for convolution source */
    std::vector<double> src(first, last);
    const int half = window / 2;

    for (long i = 0; i < n; ++i) {
        long lo = std::max(0L, i - half);
        long hi = std::min(static_cast<long>(n), i - half + static_cast<long>(window));
        double sum = std::accumulate(src.begin() + lo, src.begin() + hi, 0.0);
        *(first + i) = sum / static_cast<double>(hi - lo);
    }
}

template <typename Iter>
void algLimit(Iter first, Iter last, double lo, double hi)
{
    std::transform(first, last, first,
                   [lo, hi](auto x) { return std::clamp(x, lo, hi); });
}

template <typename Iter>
void algNorm(Iter first, Iter last)
{
    auto [mi, mx] = std::minmax_element(first, last);
    double minVal = *mi, maxVal = *mx;
    if (maxVal <= minVal) {
        std::fill(first, last, 0.0);
        return;
    }
    double range = maxVal - minVal;
    std::transform(first, last, first,
                   [minVal, range](auto x) { return (x - minVal) / range; });
}

/* --- Math --------------------------------------------------------- */
template <typename Iter>
void algAbs(Iter first, Iter last)
{
    std::transform(first, last, first,
                   [](auto x) { return std::fabs(x); });
}

template <typename Iter>
void algSqrt(Iter first, Iter last)
{
    std::transform(first, last, first,
                   [](auto x) { return std::sqrt(std::fabs(x)); });
}

/* ================================================================== */
/*  aSub functions                                                    */
/* ================================================================== */

} /* anonymous namespace */

extern "C" {

/**
 * INAM — Initialization.
 * Copies input waveform to output so a valid baseline exists.
 */
static long calcWaveformInit(aSubRecord *prec)
{
    auto *inp  = static_cast<epicsFloat64 *>(prec->a);
    auto *out  = static_cast<epicsFloat64 *>(prec->vala);
    auto  nIn  = static_cast<long>(prec->noa);
    auto  nOut = static_cast<long>(prec->nova);
    long  n    = std::min(nIn, nOut);

    std::copy_n(inp, n, out);
    if (nOut > n)
        std::fill(out + n, out + nOut, 0.0);

    errlogPrintf("calcWaveformInit: copied %ld elements\n", n);
    return 0;
}

/**
 * SNAM — Process.
 * Reads operation string and parameters, applies the algorithm to
 * the specified slice of the output waveform.
 */
static long calcWaveformProcess(aSubRecord *prec)
{
    /* --- Unpack inputs -------------------------------------------- */
    auto *inp      = static_cast<epicsFloat64 *>(prec->a);
    auto *opStr    = static_cast<const char *>(prec->b);
    auto  startIdx = *static_cast<epicsInt32 *>(prec->c);
    auto  endIdx   = *static_cast<epicsInt32 *>(prec->d);
    auto  operand  = *static_cast<epicsFloat64 *>(prec->e);
    auto  param1   = *static_cast<epicsFloat64 *>(prec->f);
    auto  param2   = *static_cast<epicsFloat64 *>(prec->g);

    auto *out  = static_cast<epicsFloat64 *>(prec->vala);
    long  nIn  = static_cast<long>(prec->noa);
    long  nOut = static_cast<long>(prec->nova);
    long  total = std::min(nIn, nOut);

    /* Copy input -> output first */
    std::copy_n(inp, total, out);

    /* --- Resolve slice -------------------------------------------- */
    if (startIdx < 0) startIdx = 0;
    if (endIdx <= 0 || endIdx > total) endIdx = static_cast<epicsInt32>(total);
    if (startIdx >= endIdx) {
        errlogPrintf("calcWaveformProcess: invalid range [%d, %d)\n",
                     startIdx, endIdx);
        return -1;
    }

    auto first = out + startIdx;
    auto last  = out + endIdx;

    /* --- Dispatch operation --------------------------------------- */
    Op op = parseOp(opStr);

    switch (op) {
    /* 1. Arithmetic */
    case Op::INIT: algFill(first, last, operand);          break;
    case Op::ADD:  /* fall through */
    case Op::INC:  algAdd(first, last, operand);           break;
    case Op::SUB:  /* fall through */
    case Op::DEC:  algSub(first, last, operand);           break;
    case Op::MUL:  algMul(first, last, operand);           break;
    case Op::DIV:
        if (operand == 0.0) {
            errlogPrintf("calcWaveformProcess: division by zero\n");
            return -1;
        }
        algDiv(first, last, operand);
        break;

    /* 2. Generators  (param1 / param2 carry sub-parameters) */
    case Op::RAND: {
        double lo = param1, hi = param2;
        if (lo == 0.0 && hi == 0.0) { lo = 0.0; hi = 100.0; }
        algRand(first, last, lo, hi);
        break;
    }
    case Op::RAMP: {
        double sv = param1, ev = param2;
        if (sv == 0.0 && ev == 0.0) { sv = 0.0; ev = 100.0; }
        algRamp(first, last, sv, ev);
        break;
    }
    case Op::SINE: {
        double cycles = (param1 != 0.0) ? param1 : 1.0;
        double amp    = (param2 != 0.0) ? param2 : 1.0;
        algSine(first, last, cycles, amp);
        break;
    }
    case Op::PWM: {
        double cycles  = (param1 != 0.0) ? param1 : 1.0;
        double duty    = (param2 != 0.0) ? param2 : 50.0;
        double highVal = (operand != 0.0) ? operand : 1.0;
        algPwm(first, last, cycles, duty, highVal);
        break;
    }

    /* 3. Signal processing */
    case Op::NOISE:
        algNoise(first, last, operand);
        break;
    case Op::SMOOTH: {
        int win = (param1 > 0.0) ? static_cast<int>(param1)
                                 : static_cast<int>(operand);
        algSmooth(first, last, win);
        break;
    }
    case Op::LIMIT: {
        double lo = param1, hi = param2;
        if (lo == 0.0 && hi == 0.0) { lo = -operand; hi = operand; }
        algLimit(first, last, lo, hi);
        break;
    }
    case Op::NORM:
        algNorm(first, last);
        break;

    /* 4. Math */
    case Op::ABS:  algAbs(first, last);  break;
    case Op::SQRT: algSqrt(first, last); break;

    case Op::UNKNOWN:
        errlogPrintf("calcWaveformProcess: unknown operation '%s'\n", opStr);
        return -1;
    }

    errlogPrintf("calcWaveformProcess: op=%s range=[%d,%d) done\n",
                 opStr, startIdx, endIdx);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Registration                                                      */
/* ------------------------------------------------------------------ */
epicsRegisterFunction(calcWaveformInit);
epicsRegisterFunction(calcWaveformProcess);

} /* extern "C" */
