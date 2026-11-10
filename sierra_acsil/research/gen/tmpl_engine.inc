#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>

#define STUDY_VERSION      "v3"
#define STUDY_BUILD_STAMP  __DATE__ " " __TIME__

namespace @NS@
{
    // sierrachart.h #defines min/max: typed helpers only.
    static inline int    IMax(int a, int b)       { return a > b ? a : b; }
    static inline int    IMin(int a, int b)       { return a < b ? a : b; }
    static inline double DMax(double a, double b) { return a > b ? a : b; }
    static inline double DMin(double a, double b) { return a < b ? a : b; }
    static inline double DAbs(double a)           { return a < 0.0 ? -a : a; }

    enum : int { DIR_BOTH = 0, DIR_LONG, DIR_SHORT };
    enum : int { MAXB = 400, MAXLOOK = 30 };            // bars per day session, sessions of history kept
    enum : int { KIND_MARKET = 0, KIND_STOP = 1 };

    enum InputIdx : int
    {
@INPUT_ENUM@
        IN_SESS_OPEN, IN_SESS_CLOSE, IN_EOD, IN_DIRECTIONS, IN_LOOKBACK, IN_MAX_TRADES, IN_TALLY_COST, IN_VERBOSE,
        IN_ORDER_MODE, IN_ALLOW_LIVE, IN_SIZE_MODE, IN_QTY, IN_ACCOUNT, IN_RISK_PCT, IN_MAX_QTY, IN_MAX_RISK_USD,
        IN_DAILY_LOSS, IN_PROFIT_LOCK, IN_MAX_CONSEC, IN_SAFETY, IN_INHERITED, IN_ENTRY_DELAY, IN_EARLY_CLOSE
    };

    enum SubgraphIdx : int
    {
        SG_VWAP = 0, SG_UB, SG_LB, SG_L1, SG_L2, SG_SIG_LONG, SG_SIG_SHORT, SG_VENTRY, SG_VEXIT, SG_STATE,
        SG_ORD_ENTRY, SG_ORD_STOP, NSG
    };

    struct Params
    {
@PARAM_FIELDS@
        int    SessOpen   = 9 * 3600 + 30 * 60;        // day session, New York
        int    SessClose  = 16 * 3600;
        int    EOD        = 15 * 3600 + 55 * 60;       // flat time (measured 15:55)
        int    Directions = DIR_BOTH;
        int    Lookback   = 14;                        // full sessions in the range / noise averages
        int    MaxTrades  = @DEF_MAXTR@;               // per day, 0 = off
        double TallyCost  = 1.25;                      // points per contract per round trip (tally only)
        int    Verbose    = 0;
        int    Fallbacks  = 0;
    };

    // ------------------------------------------------------------------ dates
    static void YMD(int serial, int& y, int& m, int& d)        // serial = days since 1899-12-30
    {
        const int z   = serial - 25569 + 719468;
        const int era = (z >= 0 ? z : z - 146096) / 146097;
        const unsigned doe = (unsigned)(z - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp  = (5 * doy + 2) / 153;
        const unsigned dd  = doy - (153 * mp + 2) / 5 + 1;
        const unsigned mm  = mp < 10 ? mp + 3 : mp - 9;
        y = (int)yoe + era * 400 + (mm <= 2 ? 1 : 0);
        m = (int)mm;
        d = (int)dd;
    }
    static inline int WeekDay(int serial) { return (serial + 6) % 7; }   // 0 = Sunday .. 6 = Saturday

    static void StampStr(char* out, size_t n, int date, int tod)
    {
        int y, m, d;
        YMD(date, y, m, d);
        std::snprintf(out, n, "%04d-%02d-%02d %02d:%02d", y, m, d, tod / 3600, (tod / 60) % 60);
    }
    static inline double RoundNear(double x, double tick) { return std::floor(x / tick + 0.5) * tick; }

    // CME equity-index halt on a trading day, New York time (0 = normal day): 13:00 holiday
    // sessions, 13:15 early closes (lessons #66; matched every short session 2019-2026).
    static int ExchangeHaltTod(int td)
    {
        int y, m, d;
        YMD(td, y, m, d);
        const int wd = WeekDay(td);
        if (wd == 0 || wd == 6) return 0;
        const int H13 = 13 * 3600, H1315 = 13 * 3600 + 15 * 60;
        if (m == 1  && wd == 1 && d >= 15 && d <= 21) return H13;                                    // MLK
        if (m == 2  && wd == 1 && d >= 15 && d <= 21) return H13;                                    // Presidents
        if (m == 5  && wd == 1 && d >= 25)            return H13;                                    // Memorial
        if (m == 6  && y >= 2022 && (d == 19 || (d == 18 && wd == 5) || (d == 20 && wd == 1))) return H13;  // Juneteenth
        if (m == 7  && (d == 4 || (d == 3 && wd == 5) || (d == 5 && wd == 1))) return H13;           // Independence
        if (m == 9  && wd == 1 && d <= 7)             return H13;                                    // Labor
        if (m == 11 && wd == 4 && d >= 22 && d <= 28) return H13;                                    // Thanksgiving
        if (m == 7  && d == 3 && wd >= 1 && wd <= 4)  return H1315;
        if (m == 11 && wd == 5 && d >= 23 && d <= 29) return H1315;                                  // day after Thanksgiving
        if (m == 12 && d == 24 && wd >= 1 && wd <= 4) return H1315;
        return 0;
    }

    // ----------------------------------------------------------------- engine
    enum : int { EX_STOP = 0, EX_TRAIL, EX_EOD, EX_N };
    static const char* const kExit[EX_N] = { "STOP", "TRAIL (band / VWAP)", "EOD" };
    enum : int { VS_FLAT = 0, VS_PEND_MKT, VS_PEND_STOP, VS_OPEN };

    struct VTrade                                           // the engine's own (virtual) trade
    {
        int    state, dir, seq, exitNext;
        double px, stop, dist, trig, cancelPx;
        int    expireTod;
    };

    struct Sess { float rng; float mv[MAXB]; };

    struct Engine
    {
        int    lastCommitted, recalcOpen, bannerDone, tallyLogged, tickWarned, quiet, virtualLog;
        // the day session
        int    curDate, nb, dayOK, halt, barsSeen, lastB, sessions;
        double dOpen, dHigh, dLow, dClose, pv, vol, vwap, prevClose, adr;
        float  mv[MAXB], sig[MAXB];
        Sess   ring[MAXLOOK];
        int    ringN, ringHead;
        // strategy day state
        int    trend, tradesToday, dayDead;
        double orh, orl;
        // order-layer interface: the last entry signal and the last exit decision
        int     sigSeq, sigBar, sigDir, sigKind, sigExpireTod, sigDate;
        int64_t sigKey, exitKey;
        double  sigRef, sigTrig, sigStopDist, sigCancelPx, sigAdr;
        int     exitLong, exitShort;
        // tally
        VTrade vt;
        int    nTr, nWin, nEx[EX_N], nDir[2];
        double sumPts, grossW, grossL, usdPerPt, eq, peak, maxDD;

        void ResetAll()
        {
            std::memset(this, 0, sizeof(*this));
            lastCommitted = -1;
            sigBar = -1;
        }
    };

    static inline bool DirOn(const Params& P, int d)
    {
        return P.Directions == DIR_BOTH || (d > 0 && P.Directions == DIR_LONG) || (d < 0 && P.Directions == DIR_SHORT);
    }
    static inline const char* DirName(int d) { return d > 0 ? "LONG" : "SHORT"; }

    template <class F> static void CloseVirtual(F& f, Engine& E, const Params& P, int how, double ex, int k)
    {
        VTrade& v = E.vt;
        const double pts = v.dir * (ex - v.px) - P.TallyCost;
        ++E.nTr; ++E.nEx[how]; ++E.nDir[v.dir > 0 ? 0 : 1];
        E.sumPts += pts;
        if (pts > 0.0) { ++E.nWin; E.grossW += pts; } else E.grossL -= pts;
        E.eq += pts;
        if (E.eq > E.peak) E.peak = E.eq;
        if (E.peak - E.eq > E.maxDD) E.maxDD = E.peak - E.eq;
        f.Out(SG_VEXIT, k) = (float)ex;
        if (P.Verbose || (!E.quiet && E.virtualLog))
        {
            char ts[48], msg[320];
            StampStr(ts, sizeof ts, f.Date(k), f.Tod(k));
            std::snprintf(msg, sizeof msg, "@TAG@ virtual #%d %s exit %s at %.2f (%s) -> %+.2f pts after %.2f cost | entry %.2f",
                          v.seq, DirName(v.dir), kExit[how], ex, ts, pts, P.TallyCost, v.px);
            f.Log(msg, 0);
        }
        if (P.Verbose == 2)                                 // machine-readable line for the test harness
        {
            char msg[200];
            std::snprintf(msg, sizeof msg, "VT,%d,%d,%d,%d,%.2f,%.2f,%d", f.Date(k), f.Tod(k), v.seq, v.dir, v.px, ex, how);
            f.Log(msg, 0);
        }
        v.state = VS_FLAT;
    }

    template <class F> static void LogTally(F& f, const Engine& E, const Params& P, const char* tag)
    {
        char msg[900];
        std::snprintf(msg, sizeof msg,
                      "@TAG@ TALLY %s (diagnostic, not P&L: this chart's bars, one virtual 1-contract trade at a time, stop first on a "
                      "shared bar, %.2f pt cost per round trip) | sessions %d | trades %d (long %d, short %d) | winners %d (%.1f%%) | "
                      "profit factor %.2f | net %+.2f pts = $%.0f per contract | max drawdown %.2f pts | exits: stop %d, trail %d, EOD %d",
                      tag, P.TallyCost, E.sessions, E.nTr, E.nDir[0], E.nDir[1], E.nWin, E.nTr > 0 ? 100.0 * E.nWin / E.nTr : 0.0,
                      E.grossL > 0.0 ? E.grossW / E.grossL : 0.0, E.sumPts, E.sumPts * E.usdPerPt, E.maxDD,
                      E.nEx[EX_STOP], E.nEx[EX_TRAIL], E.nEx[EX_EOD]);
        f.Log(msg, 0);
    }

    // Publish an entry signal to the order layer and open the engine's own pending trade.
    template <class F> static void Emit(F& f, Engine& E, const Params& P, int k, int d, int kind, double trig, double dist,
                                        double cancelPx, int expireTod, const char* what)
    {
        const double tick = f.Tick();
        dist = DMax(RoundNear(dist, tick), 2.0 * tick);
        ++E.sigSeq; ++E.tradesToday;
        E.sigBar = k; E.sigDir = d; E.sigKind = kind; E.sigRef = f.C(k); E.sigTrig = trig; E.sigStopDist = dist;
        E.sigCancelPx = cancelPx; E.sigExpireTod = expireTod; E.sigDate = f.Date(k); E.sigAdr = E.adr;
        E.sigKey = (int64_t)f.Date(k) * 86400 + f.Tod(k);
        VTrade& v = E.vt;
        std::memset(&v, 0, sizeof v);
        v.state = kind == KIND_MARKET ? VS_PEND_MKT : VS_PEND_STOP;
        v.dir = d; v.seq = E.sigSeq; v.dist = dist; v.trig = trig; v.cancelPx = cancelPx; v.expireTod = expireTod;
        f.Out(d > 0 ? SG_SIG_LONG : SG_SIG_SHORT, k) = d > 0 ? (float)(f.L(k) - 8.0 * tick) : (float)(f.H(k) + 8.0 * tick);
        if (P.Verbose || !E.quiet)
        {
            char ts[48], msg[500];
            StampStr(ts, sizeof ts, f.Date(k), f.Tod(k));
            std::snprintf(msg, sizeof msg, "@TAG@ SIGNAL #%d %s %s | %s | close %.2f | VWAP %.2f | open %.2f, prior close %.2f | avg range "
                          "%.2f | stop distance %.2f pts", E.sigSeq, DirName(d), ts, what, f.C(k), E.vwap, E.dOpen, E.prevClose, E.adr, dist);
            f.Log(msg, 0);
        }
    }

@STRATEGY@

    // ------------------------------------------------------------ bar driver
    template <class F> static void ProcessBar(F& f, Engine& E, const Params& P, int k)
    {
        const int date = f.Date(k), tod = f.Tod(k);
        const int bsec = f.BarSec() > 0 ? f.BarSec() : 300;
        for (int g = 0; g < SG_ORD_ENTRY; ++g) f.Out(g, k) = 0.0f;
        VTrade& v = E.vt;
        if (tod < P.SessOpen || tod >= P.SessClose) return;            // only the day session is used

        // (1) a new day session: bank yesterday, average the last Lookback full sessions
        if (date != E.curDate)
        {
            if (v.state == VS_OPEN) CloseVirtual(f, E, P, EX_EOD, E.dClose, k);   // data ended before the flat time
            v.state = VS_FLAT;
            if (E.curDate != 0)
            {
                if (E.barsSeen >= E.nb - 2 && !E.halt && E.dOpen > 0.0)
                {
                    for (int b = E.lastB + 1; b < E.nb; ++b) E.mv[b] = E.mv[E.lastB];
                    Sess& s = E.ring[E.ringHead];
                    s.rng = (float)(E.dHigh - E.dLow);
                    std::memcpy(s.mv, E.mv, sizeof s.mv);
                    E.ringHead = (E.ringHead + 1) % MAXLOOK;
                    if (E.ringN < MAXLOOK) ++E.ringN;
                }
                E.prevClose = E.barsSeen * 100 >= E.nb * 35 ? E.dClose : 0.0;
            }
            E.curDate = date; ++E.sessions;
            E.nb = IMin((P.SessClose - P.SessOpen) / bsec, MAXB);
            E.halt = ExchangeHaltTod(date);
            E.barsSeen = 0; E.lastB = -1;
            E.dOpen = f.O(k); E.dHigh = f.H(k); E.dLow = f.L(k); E.dClose = f.C(k); E.pv = 0.0; E.vol = 0.0; E.vwap = 0.0;
            E.trend = 0; E.tradesToday = 0; E.dayDead = 0; E.orh = 0.0; E.orl = 0.0;
            E.exitLong = E.exitShort = 0;
            std::memset(E.mv, 0, sizeof E.mv);
            E.adr = 0.0;
            if (E.ringN >= P.Lookback)
            {
                double r = 0.0;
                for (int b = 0; b < E.nb; ++b) E.sig[b] = 0.0f;
                for (int q = 1; q <= P.Lookback; ++q)
                {
                    const Sess& s = E.ring[(E.ringHead - q + 2 * MAXLOOK) % MAXLOOK];
                    r += s.rng;
                    for (int b = 0; b < E.nb; ++b) E.sig[b] += s.mv[b];
                }
                E.adr = r / P.Lookback;
                for (int b = 0; b < E.nb; ++b) E.sig[b] /= (float)P.Lookback;
            }
            // tradable: averages ready, a prior close, a normal (not holiday-shortened) session
            E.dayOK = (E.adr > 0.0 && E.prevClose > 0.0 && !E.halt && E.dOpen > 0.0) ? 1 : 0;
        }
        const int b = (tod - P.SessOpen) / bsec;
        if (b < 0 || b >= E.nb) return;
        const bool flatBar = tod >= P.EOD;

        // (2) the engine's own trade on this bar: fills at the open, stop first, flat at the EOD bar's open
        bool stopFilledHere = false;
        if (v.state == VS_PEND_MKT)
        {
            if (flatBar) v.state = VS_FLAT;
            else
            {
                v.state = VS_OPEN; v.px = f.O(k); v.stop = v.px - v.dir * v.dist; v.exitNext = 0;
                f.Out(SG_VENTRY, k) = (float)v.px;
            }
        }
        else if (v.state == VS_PEND_STOP)
        {
            const bool hit = v.dir > 0 ? f.H(k) >= v.trig : f.L(k) <= v.trig;
            const bool opp = v.dir > 0 ? f.L(k) <= v.cancelPx : f.H(k) >= v.cancelPx;
            if (flatBar || tod >= v.expireTod || opp) v.state = VS_FLAT;       // expired, or the other side broke first / same bar
            else if (hit)
            {
                v.state = VS_OPEN; v.exitNext = 0; stopFilledHere = true;
                v.px = v.dir > 0 ? DMax(v.trig, f.O(k)) : DMin(v.trig, f.O(k));
                v.stop = v.px - v.dir * v.dist;
                f.Out(SG_VENTRY, k) = (float)v.px;
            }
        }
        else if (v.state == VS_OPEN && v.exitNext && !flatBar) CloseVirtual(f, E, P, EX_TRAIL, f.O(k), k);
        if (v.state == VS_OPEN)
        {
            if (flatBar) CloseVirtual(f, E, P, EX_EOD, f.O(k), k);
            else if (v.dir > 0 ? f.L(k) <= v.stop : f.H(k) >= v.stop)
                CloseVirtual(f, E, P, EX_STOP, stopFilledHere ? v.stop : (v.dir > 0 ? DMin(v.stop, f.O(k)) : DMax(v.stop, f.O(k))), k);
        }

        // (3) session statistics including this bar
        if (E.lastB >= 0) for (int q = E.lastB + 1; q < b; ++q) E.mv[q] = E.mv[E.lastB];   // bars missing from the data
        E.dHigh = DMax(E.dHigh, f.H(k)); E.dLow = DMin(E.dLow, f.L(k)); E.dClose = f.C(k);
        const double w = DMax(f.V(k), 1.0);
        E.pv += w * ((f.H(k) + f.L(k) + f.C(k)) / 3.0 - E.dOpen);          // deviations from the open (lessons #31)
        E.vol += w;
        E.vwap = E.dOpen + E.pv / E.vol;
        E.mv[b] = (float)DAbs(f.C(k) / E.dOpen - 1.0);
        E.lastB = b; ++E.barsSeen;
        f.Out(SG_VWAP, k) = (float)E.vwap;
        double ub = 0.0, lb = 0.0;
        if (E.dayOK)
        {
            ub = DMax(E.dOpen, E.prevClose) * (1.0 + P.BandMult * E.sig[b]);
            lb = DMin(E.dOpen, E.prevClose) * (1.0 - P.BandMult * E.sig[b]);
            f.Out(SG_UB, k) = (float)ub; f.Out(SG_LB, k) = (float)lb;
        }

        // (4) the strategy's decision at this close
        if (E.dayOK && !flatBar) Decide(f, E, P, k, b, tod, bsec, ub, lb);
        f.Out(SG_STATE, k) = (float)(v.state == VS_OPEN ? v.dir : 0);
    }
}   // namespace @NS@
