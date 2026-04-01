/*
 * calcWaveformASub.cpp
 *
 * EPICS aSub record implementation for dual-output waveform slice operations.
 *
 * ============================================================================
 *  aSub Record Field Mapping
 * ============================================================================
 *   INPA  : Input waveform (init source)  (DOUBLE array, NOA)
 *   INPB  : Operation name                (STRING, e.g. "INIT","ADD","PWM")
 *   INPC  : Range start index             (LONG,   0-based)
 *   INPD  : Range end index               (LONG,   exclusive, <=0 = full)
 *   INPE  : Operand / value               (DOUBLE)
 *   INPF  : Param1                        (DOUBLE, cycles/min/window)
 *   INPG  : Param2                        (DOUBLE, duty/max/amp)
 *   INPH  : Index selector                (LONG,   0=both, 1=VALA, 2=VALB)
 *   VALA  : Output waveform A             (DOUBLE array, NOVA)
 *   VALB  : Output waveform B             (DOUBLE array, NOVB)
 *
 * ============================================================================
 *  DB Record Template
 * ============================================================================
 *
 *   record(waveform, "$(P):InputWf") {
 *       field(NELM, "1024")
 *       field(FTVL, "DOUBLE")
 *   }
 *   record(stringout, "$(P):OpName")    { field(VAL,  "INIT")  }
 *   record(longout,   "$(P):RangeStart"){ field(VAL,  "0")     }
 *   record(longout,   "$(P):RangeEnd")  { field(VAL,  "0")     }
 *   record(ao,        "$(P):Operand")   { field(VAL,  "0.0")   }
 *   record(ao,        "$(P):Param1")    { field(VAL,  "0.0")   }
 *   record(ao,        "$(P):Param2")    { field(VAL,  "0.0")   }
 *   record(longout,   "$(P):Index")     { field(VAL,  "0")     }
 *
 *   record(aSub, "$(P):CalcWf") {
 *       field(INAM, "calcWaveformInit")
 *       field(SNAM, "calcWaveformProcess")
 *       field(FTA,  "DOUBLE")
 *       field(FTB,  "STRING")
 *       field(FTC,  "LONG")
 *       field(FTD,  "LONG")
 *       field(FTE,  "DOUBLE")
 *       field(FTF,  "DOUBLE")
 *       field(FTG,  "DOUBLE")
 *       field(FTH,  "LONG")
 *       field(FTVA, "DOUBLE")
 *       field(FTVB, "DOUBLE")
 *       field(NOA,  "1024")
 *       field(NOVA, "1024")
 *       field(NOVB, "1024")
 *       field(INPA, "$(P):InputWf NPP")
 *       field(INPB, "$(P):OpName NPP")
 *       field(INPC, "$(P):RangeStart NPP")
 *       field(INPD, "$(P):RangeEnd NPP")
 *       field(INPE, "$(P):Operand NPP")
 *       field(INPF, "$(P):Param1 NPP")
 *       field(INPG, "$(P):Param2 NPP")
 *       field(INPH, "$(P):Index NPP")
 *       field(OUTA, "$(P):OutputWfA PP")
 *       field(OUTB, "$(P):OutputWfB PP")
 *   }
 *
 *   record(waveform, "$(P):OutputWfA") {
 *       field(NELM, "1024")
 *       field(FTVL, "DOUBLE")
 *   }
 *   record(waveform, "$(P):OutputWfB") {
 *       field(NELM, "1024")
 *       field(FTVL, "DOUBLE")
 *   }
 *
 * ============================================================================
 *  IOC Shell 시험 검증 방법 (caput / caget)
 * ============================================================================
 *
 *  공통 매크로: P=TEST
 *  처리 실행:  caput TEST:CalcWf.PROC 1
 *  결과 확인:  caget -# 10 TEST:OutputWfA   (VALA 앞 10개)
 *              caget -# 10 TEST:OutputWfB   (VALB 앞 10개)
 *
 * --------------------------------------------------------------------------
 *  [1] Index=0 — VALA + VALB 동시 초기화
 * --------------------------------------------------------------------------
 *    caput TEST:Index      0
 *    caput TEST:OpName     "INIT"
 *    caput TEST:Operand    5.0
 *    caput TEST:RangeStart 0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 5 TEST:OutputWfA         # 기대: 5.0 5.0 5.0 5.0 5.0
 *    caget -# 5 TEST:OutputWfB         # 기대: 5.0 5.0 5.0 5.0 5.0
 *
 * --------------------------------------------------------------------------
 *  [2] Index=1 — VALA 만 SINE 적용
 * --------------------------------------------------------------------------
 *    caput TEST:Index      1
 *    caput TEST:OpName     "SINE"
 *    caput TEST:Param1     3.0
 *    caput TEST:Param2     5.0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWfA        # 기대: 3주기 사인파 (-5 ~ +5)
 *    caget -# 10 TEST:OutputWfB        # 기대: 변동 없음 (5.0 유지)
 *
 * --------------------------------------------------------------------------
 *  [3] Index=2 — VALB 만 PWM 적용
 * --------------------------------------------------------------------------
 *    caput TEST:Index      2
 *    caput TEST:OpName     "PWM"
 *    caput TEST:Param1     5.0
 *    caput TEST:Param2     30.0
 *    caput TEST:Operand    3.3
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 10 TEST:OutputWfA        # 기대: 변동 없음 (사인파 유지)
 *    caget -# 10 TEST:OutputWfB        # 기대: PWM 사각파 (3.3V/0V)
 *
 * --------------------------------------------------------------------------
 *  [4] Index=0 슬라이스 — 양쪽 동시 부분 연산
 * --------------------------------------------------------------------------
 *    caput TEST:Index      0
 *    caput TEST:OpName     "INIT"
 *    caput TEST:Operand    0.0
 *    caput TEST:RangeEnd   0
 *    caput TEST:CalcWf.PROC 1          # 양쪽 전체 0 초기화
 *
 *    caput TEST:OpName     "RAMP"
 *    caput TEST:Param1     0
 *    caput TEST:Param2     100
 *    caput TEST:RangeStart 100
 *    caput TEST:RangeEnd   300
 *    caput TEST:CalcWf.PROC 1
 *    # 기대: 양쪽 모두 [0..99]=0, [100..299]=램프, [300..]=0
 *
 * --------------------------------------------------------------------------
 *  [5] 독립 채널 복합 시나리오
 * --------------------------------------------------------------------------
 *    # VALA: RAMP → NOISE → SMOOTH
 *    caput TEST:Index 1
 *    caput TEST:OpName "RAMP" && caput TEST:Param1 0 && caput TEST:Param2 100
 *    caput TEST:RangeStart 0 && caput TEST:RangeEnd 0
 *    caput TEST:CalcWf.PROC 1
 *
 *    caput TEST:OpName "NOISE" && caput TEST:Operand 5.0
 *    caput TEST:CalcWf.PROC 1
 *
 *    caput TEST:OpName "SMOOTH" && caput TEST:Param1 7
 *    caput TEST:CalcWf.PROC 1
 *
 *    # VALB: SINE → ABS → LIMIT
 *    caput TEST:Index 2
 *    caput TEST:OpName "SINE" && caput TEST:Param1 4 && caput TEST:Param2 10
 *    caput TEST:CalcWf.PROC 1
 *
 *    caput TEST:OpName "ABS"
 *    caput TEST:CalcWf.PROC 1
 *
 *    caput TEST:OpName "LIMIT" && caput TEST:Param1 0 && caput TEST:Param2 7
 *    caput TEST:CalcWf.PROC 1
 *
 *    caget -# 10 TEST:OutputWfA        # 기대: 부드러운 램프 (0~100)
 *    caget -# 10 TEST:OutputWfB        # 기대: |사인| 클램핑 (0~7)
 *
 * --------------------------------------------------------------------------
 *  [6] ADDW / SUBW / MULW / DIVW — 두 waveform 간 사칙연산
 * --------------------------------------------------------------------------
 *    # 사전: VALA 와 VALB 를 각각 독립적으로 세팅
 *    caput TEST:Index 1
 *    caput TEST:OpName "RAMP" && caput TEST:Param1 0 && caput TEST:Param2 10
 *    caput TEST:RangeStart 0 && caput TEST:RangeEnd 0
 *    caput TEST:CalcWf.PROC 1          # VALA = 0,0.01,...,10 (램프)
 *
 *    caput TEST:Index 2
 *    caput TEST:OpName "INIT" && caput TEST:Operand 2.0
 *    caput TEST:CalcWf.PROC 1          # VALB = 2.0 (전체)
 *
 *    # --- ADDW: VALA = VALA + VALB (Index=1) ---
 *    caput TEST:Index   1
 *    caput TEST:OpName  "ADDW"
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 5 TEST:OutputWfA         # 기대: 2.0, 2.01, 2.02, ... (+2)
 *    caget -# 5 TEST:OutputWfB         # 기대: 2.0 유지 (변동 없음)
 *
 *    # --- SUBW: VALB = VALB - VALA (Index=2) ---
 *    caput TEST:Index   2
 *    caput TEST:OpName  "SUBW"
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 5 TEST:OutputWfB         # 기대: 2.0 - VALA 값들
 *    caget -# 5 TEST:OutputWfA         # 기대: 변동 없음
 *
 *    # --- MULW: 슬라이스 적용 (100~200 구간만) ---
 *    caput TEST:Index     1
 *    caput TEST:OpName    "MULW"
 *    caput TEST:RangeStart 100
 *    caput TEST:RangeEnd   200
 *    caput TEST:CalcWf.PROC 1
 *    # 기대: VALA[100..199] = VALA[100..199] * VALB[100..199]
 *    #       나머지 구간 변동 없음
 *
 *    # --- DIVW: VALA = VALA / VALB (0 나누기 보호 → 0.0) ---
 *    caput TEST:Index   1
 *    caput TEST:OpName  "DIVW"
 *    caput TEST:RangeStart 0 && caput TEST:RangeEnd 0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 5 TEST:OutputWfA         # 기대: VALA / VALB (element-wise)
 *
 *    # --- Index=0: 양쪽 동시 (VALA=A+B, VALB=B+A 독립 적용) ---
 *    # 사전: 다시 초기 세팅
 *    caput TEST:Index 0
 *    caput TEST:OpName "INIT" && caput TEST:Operand 0
 *    caput TEST:CalcWf.PROC 1
 *    caput TEST:Index 1
 *    caput TEST:OpName "RAMP" && caput TEST:Param1 1 && caput TEST:Param2 5
 *    caput TEST:CalcWf.PROC 1          # VALA = 1~5
 *    caput TEST:Index 2
 *    caput TEST:OpName "INIT" && caput TEST:Operand 10
 *    caput TEST:CalcWf.PROC 1          # VALB = 10
 *
 *    caput TEST:Index   0
 *    caput TEST:OpName  "ADDW"
 *    caput TEST:RangeStart 0 && caput TEST:RangeEnd 0
 *    caput TEST:CalcWf.PROC 1
 *    caget -# 5 TEST:OutputWfA         # 기대: 11, 11.004, ... (원래A + 원래B)
 *    caget -# 5 TEST:OutputWfB         # 기대: 11, 11.004, ... (원래B + 원래A)
 *
 * --------------------------------------------------------------------------
 *  [7] 에러 케이스
 * --------------------------------------------------------------------------
 *    # 잘못된 범위
 *    caput TEST:RangeStart 500
 *    caput TEST:RangeEnd   100
 *    caput TEST:CalcWf.PROC 1          # 에러 로그, 반환값 -1
 *
 *    # 알 수 없는 연산자
 *    caput TEST:OpName "INVALID"
 *    caput TEST:CalcWf.PROC 1          # 에러 로그 "unknown operation"
 *
 *    # 잘못된 Index
 *    caput TEST:Index 5
 *    caput TEST:CalcWf.PROC 1          # 에러 로그 "invalid index"
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
    /* Cross-waveform arithmetic: dest[i] = dest[i] op src[i] */
    ADDW, SUBW, MULW, DIVW,
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
        {"ADDW",  Op::ADDW},  {"SUBW", Op::SUBW},
        {"MULW",  Op::MULW},  {"DIVW", Op::DIVW},
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

/* --- Cross-waveform arithmetic: dest[i] = dest[i] op src[i] ------ */
template <typename IterD, typename IterS>
void algAddW(IterD dFirst, IterD dLast, IterS sFirst)
{
    std::transform(dFirst, dLast, sFirst, dFirst,
                   [](auto d, auto s) { return d + s; });
}

template <typename IterD, typename IterS>
void algSubW(IterD dFirst, IterD dLast, IterS sFirst)
{
    std::transform(dFirst, dLast, sFirst, dFirst,
                   [](auto d, auto s) { return d - s; });
}

template <typename IterD, typename IterS>
void algMulW(IterD dFirst, IterD dLast, IterS sFirst)
{
    std::transform(dFirst, dLast, sFirst, dFirst,
                   [](auto d, auto s) { return d * s; });
}

template <typename IterD, typename IterS>
void algDivW(IterD dFirst, IterD dLast, IterS sFirst)
{
    std::transform(dFirst, dLast, sFirst, dFirst,
                   [](auto d, auto s) { return (s != 0.0) ? d / s : 0.0; });
}

/* ================================================================== */
/*  Apply cross-waveform operation: dest[slice] op= src[slice]        */
/* ================================================================== */

long applyCrossOp(epicsFloat64 *dest, epicsFloat64 *src,
                  long nDest, long nSrc,
                  epicsInt32 startIdx, epicsInt32 endIdx,
                  Op op, const char *label)
{
    long total = std::min(nDest, nSrc);
    if (startIdx < 0) startIdx = 0;
    if (endIdx <= 0 || endIdx > total) endIdx = static_cast<epicsInt32>(total);
    if (startIdx >= endIdx) {
        errlogPrintf("calcWaveformProcess[%s]: invalid cross range [%d, %d)\n",
                     label, startIdx, endIdx);
        return -1;
    }

    auto dFirst = dest + startIdx;
    auto dLast  = dest + endIdx;
    auto sFirst = src  + startIdx;

    switch (op) {
    case Op::ADDW: algAddW(dFirst, dLast, sFirst); break;
    case Op::SUBW: algSubW(dFirst, dLast, sFirst); break;
    case Op::MULW: algMulW(dFirst, dLast, sFirst); break;
    case Op::DIVW: algDivW(dFirst, dLast, sFirst); break;
    default: return -1;
    }
    return 0;
}

/* ================================================================== */
/*  Apply operation to a single buffer slice                          */
/* ================================================================== */

long applyOp(epicsFloat64 *buf, long total,
             epicsInt32 startIdx, epicsInt32 endIdx,
             Op op, double operand, double param1, double param2,
             const char *opStr, const char *label)
{
    /* --- Resolve slice -------------------------------------------- */
    if (startIdx < 0) startIdx = 0;
    if (endIdx <= 0 || endIdx > total) endIdx = static_cast<epicsInt32>(total);
    if (startIdx >= endIdx) {
        errlogPrintf("calcWaveformProcess[%s]: invalid range [%d, %d)\n",
                     label, startIdx, endIdx);
        return -1;
    }

    auto first = buf + startIdx;
    auto last  = buf + endIdx;

    switch (op) {
    /* 1. Arithmetic */
    case Op::INIT: algFill(first, last, operand);          break;
    case Op::ADD:
    case Op::INC:  algAdd(first, last, operand);           break;
    case Op::SUB:
    case Op::DEC:  algSub(first, last, operand);           break;
    case Op::MUL:  algMul(first, last, operand);           break;
    case Op::DIV:
        if (operand == 0.0) {
            errlogPrintf("calcWaveformProcess[%s]: division by zero\n", label);
            return -1;
        }
        algDiv(first, last, operand);
        break;

    /* 2. Generators */
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

    /* Cross-waveform ops are not handled here */
    case Op::ADDW: case Op::SUBW: case Op::MULW: case Op::DIVW:
        return -1;

    case Op::UNKNOWN:
        errlogPrintf("calcWaveformProcess[%s]: unknown operation '%s'\n",
                     label, opStr);
        return -1;
    }
    return 0;
}

/* ================================================================== */
/*  aSub functions                                                    */
/* ================================================================== */

} /* anonymous namespace */

extern "C" {

/**
 * INAM — Initialization.
 * Copies INPA waveform to both VALA and VALB as initial baseline.
 */
static long calcWaveformInit(aSubRecord *prec)
{
    auto *inp   = static_cast<epicsFloat64 *>(prec->a);
    auto *outA  = static_cast<epicsFloat64 *>(prec->vala);
    auto *outB  = static_cast<epicsFloat64 *>(prec->valb);
    long  nIn   = static_cast<long>(prec->noa);
    long  nOutA = static_cast<long>(prec->nova);
    long  nOutB = static_cast<long>(prec->novb);

    long  nA = std::min(nIn, nOutA);
    long  nB = std::min(nIn, nOutB);

    std::copy_n(inp, nA, outA);
    std::copy_n(inp, nB, outB);

    if (nOutA > nA) std::fill(outA + nA, outA + nOutA, 0.0);
    if (nOutB > nB) std::fill(outB + nB, outB + nOutB, 0.0);

    errlogPrintf("calcWaveformInit: VALA=%ld, VALB=%ld elements initialized\n",
                 nA, nB);
    return 0;
}

/**
 * SNAM — Process.
 *
 * Applies operation IN-PLACE on the selected output buffer(s).
 * INPA is NOT re-copied each call — outputs accumulate operations.
 * Use "INIT" operation to reset a buffer to a desired value.
 *
 * Index selector (INPH):
 *   0 = apply to VALA and VALB simultaneously
 *   1 = apply to VALA only
 *   2 = apply to VALB only
 */
static long calcWaveformProcess(aSubRecord *prec)
{
    /* --- Unpack inputs -------------------------------------------- */
    auto *opStr    = static_cast<const char *>(prec->b);
    auto  startIdx = *static_cast<epicsInt32 *>(prec->c);
    auto  endIdx   = *static_cast<epicsInt32 *>(prec->d);
    auto  operand  = *static_cast<epicsFloat64 *>(prec->e);
    auto  param1   = *static_cast<epicsFloat64 *>(prec->f);
    auto  param2   = *static_cast<epicsFloat64 *>(prec->g);
    auto  index    = *static_cast<epicsInt32 *>(prec->h);

    auto *outA  = static_cast<epicsFloat64 *>(prec->vala);
    auto *outB  = static_cast<epicsFloat64 *>(prec->valb);
    long  nOutA = static_cast<long>(prec->nova);
    long  nOutB = static_cast<long>(prec->novb);

    Op op = parseOp(opStr);
    if (op == Op::UNKNOWN) {
        errlogPrintf("calcWaveformProcess: unknown operation '%s'\n", opStr);
        return -1;
    }

    /* --- Validate index ------------------------------------------- */
    if (index < 0 || index > 2) {
        errlogPrintf("calcWaveformProcess: invalid index %d (must be 0,1,2)\n",
                     index);
        return -1;
    }

    long rc = 0;
    bool isCrossOp = (op == Op::ADDW || op == Op::SUBW ||
                      op == Op::MULW || op == Op::DIVW);

    if (isCrossOp) {
        /* ----------------------------------------------------------
         * Cross-waveform arithmetic: dest[slice] op= src[slice]
         *   Index=0 : VALA = VALA op VALB,  VALB = VALB op VALA (동시)
         *             (VALA 먼저 임시복사 후 양쪽 독립 적용)
         *   Index=1 : VALA = VALA op VALB
         *   Index=2 : VALB = VALB op VALA
         * ---------------------------------------------------------- */
        if (index == 0) {
            /* 양쪽 동시: VALA op VALB, VALB op VALA
             * VALA 를 먼저 변경하면 VALB 연산 소스가 오염되므로
             * VALA 원본을 임시 보관한 뒤 양쪽 독립 적용 */
            long total = std::min(nOutA, nOutB);
            std::vector<double> tmpA(outA, outA + total);

            rc = applyCrossOp(outA, outB, nOutA, nOutB,
                              startIdx, endIdx, op, "VALA=A op B");
            if (rc != 0) return rc;
            rc = applyCrossOp(outB, tmpA.data(), nOutB, total,
                              startIdx, endIdx, op, "VALB=B op A");
            if (rc != 0) return rc;
        } else if (index == 1) {
            rc = applyCrossOp(outA, outB, nOutA, nOutB,
                              startIdx, endIdx, op, "VALA=A op B");
            if (rc != 0) return rc;
        } else {
            rc = applyCrossOp(outB, outA, nOutB, nOutA,
                              startIdx, endIdx, op, "VALB=B op A");
            if (rc != 0) return rc;
        }
    } else {
        /* --- Single-buffer operations ----------------------------- */
        if (index == 0 || index == 1) {
            rc = applyOp(outA, nOutA, startIdx, endIdx,
                         op, operand, param1, param2, opStr, "VALA");
            if (rc != 0) return rc;
        }

        if (index == 0 || index == 2) {
            rc = applyOp(outB, nOutB, startIdx, endIdx,
                         op, operand, param1, param2, opStr, "VALB");
            if (rc != 0) return rc;
        }
    }

    errlogPrintf("calcWaveformProcess: op=%s idx=%d range=[%d,%d) done\n",
                 opStr, index, startIdx, endIdx);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Registration                                                      */
/* ------------------------------------------------------------------ */
epicsRegisterFunction(calcWaveformInit);
epicsRegisterFunction(calcWaveformProcess);

} /* extern "C" */
