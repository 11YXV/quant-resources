#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>

#define STUDY_VERSION      "v1"
#define STUDY_BUILD_STAMP  __DATE__ " " __TIME__

namespace dsu
{
    // sierrachart.h #defines min/max: typed helpers only.
    static inline int    IMax(int a, int b)       { return a > b ? a : b; }
    static inline int    IMin(int a, int b)       { return a < b ? a : b; }
    static inline double DMax(double a, double b) { return a > b ? a : b; }
    static inline double DMin(double a, double b) { return a < b ? a : b; }
    static inline double DAbs(double a)           { return a < 0.0 ? -a : a; }

    enum : int { DIR_BOTH = 0, DIR_LONG, DIR_SHORT };
    enum : int { MAXB = 400, MAXLOOK = 30, HB = 8192 };     // bars per day session, sessions kept, value-area bins
    enum : int { S_DWR = 0, S_VAR, S_CEB, S_EDF, NS };
    static const char* const kStrat[NS] = { "DWR", "VAR", "CEB", "EDF" };

    enum InputIdx : int
    {
        IN_DWR_ON = 0, IN_DWR_QTY, IN_DWR_RSI_LEN, IN_DWR_RSI_BUY, IN_DWR_RSI_SELL, IN_DWR_STOP,
        IN_VAR_ON, IN_VAR_QTY, IN_VAR_FRAC, IN_VAR_BIN, IN_VAR_MARK, IN_VAR_FIRST, IN_VAR_LAST, IN_VAR_STOP,
        IN_CEB_ON, IN_CEB_QTY, IN_CEB_NR, IN_CEB_OR, IN_CEB_LAST, IN_CEB_STOP,
        IN_EDF_ON, IN_EDF_QTY, IN_EDF_MOVE, IN_EDF_MARK, IN_EDF_STOP,
        IN_SESS_OPEN, IN_SESS_CLOSE, IN_EOD, IN_DIRECTIONS, IN_LOOKBACK, IN_TALLY_COST, IN_VERBOSE,
        IN_ORDER_MODE, IN_ALLOW_LIVE, IN_SIZE_MODE, IN_ACCOUNT, IN_RISK_PCT, IN_MAX_QTY, IN_MAX_RISK_USD, IN_MAX_OPEN_RISK,
        IN_DAILY_LOSS, IN_PROFIT_LOCK, IN_MAX_CONSEC, IN_SAFETY, IN_INHERITED, IN_ENTRY_DELAY, IN_EARLY_CLOSE
    };

    enum SubgraphIdx : int
    {
        SG_VAH = 0, SG_VAL, SG_POC, SG_ORH, SG_ORL, SG_EDF_UP, SG_EDF_DN, SG_RSI, SG_NR,
        SG_LONG0, SG_SHORT0 = SG_LONG0 + NS, SG_ENGINE_END = SG_SHORT0 + NS, SG_STOP0 = SG_ENGINE_END, NSG = SG_STOP0 + NS
    };

    struct Params
    {
        int    On[NS]     = { 1, 1, 1, 1 };
        int    Qty[NS]    = { 1, 1, 1, 1 };
        // DWR daily washout rebound
        int    RsiLen     = 2;                         // Wilder RSI length, day-session closes
        double RsiBuy     = 30.0;                      // long at the open when RSI is below this
        double RsiSell    = 0.0;                       // short when above this; 0 = off (measured: off)
        double DwrStop    = 0.75;                      // x average day range
        // VAR value-area re-entry
        double VaFrac     = 0.70;                      // value area = this share of the prior session's volume
        int    VaBinTicks = 4;                         // profile bin, ticks
        double VaBin      = 1.0;                       // the same in points (set from the tick size)
        int    VarMark    = 30;                        // decision marks, minutes from the open
        int    VarFirst   = 10 * 3600;                 // first decision (bar close time)
        int    VarLast    = 12 * 3600 + 30 * 60;       // last decision that may open a trade
        double VarStop    = 0.20;
        // CEB compression -> expansion breakout
        int    NrDays     = 4;                         // yesterday's range the smallest of this many full sessions
        int    OrMinutes  = 30;                        // opening range
        int    CebLast    = 13 * 3600;                 // last bar close that may open a trade
        double CebStop    = 0.25;
        // EDF end-of-day flow
        double EdfMove    = 1.0;                       // |decision close / prior close - 1|, percent
        int    EdfMark    = 15 * 3600;                 // decision bar close time
        double EdfStop    = 0.40;
        // common
        int    SessOpen   = 9 * 3600 + 30 * 60;        // day session, New York
        int    SessClose  = 16 * 3600;
        int    EOD        = 15 * 3600 + 55 * 60;       // flat time (measured 15:55)
        int    Directions = DIR_BOTH;
        int    Lookback   = 14;                        // full sessions in the average range
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
    enum : int { EX_STOP = 0, EX_EOD, EX_N };
    enum : int { VS_FLAT = 0, VS_PEND_MKT, VS_OPEN };

    struct VTrade { int state, dir, seq; double px, stop, dist; };   // a strategy's own (virtual) trade, for the tally

    struct Strat                                            // one strategy's day state, signal interface and tally
    {
        int     tradesToday, dayDead, side;
        double  orh, orl;
        int     sigSeq, sigBar, sigDir;
        int64_t sigKey;
        double  sigStopDist;
        VTrade  vt;
        int     nTr, nWin, nEx[EX_N], nDir[2];
        double  sumPts, grossW, grossL;
    };

    struct Engine
    {
        int    lastCommitted, recalcOpen, bannerDone, tallyLogged, tickWarned, quiet, virtualLog;
        // the day session
        int    curDate, nb, dayOK, halt, barsSeen, lastB, sessions;
        double dOpen, dHigh, dLow, dClose, prevClose, adr, usdPerPt;
        // references for a date, built once at the first bar handled for it (the pre-open bar or the first session bar)
        int    refDate, bankedDate, refOK, ringN, ringHead;
        double ring[MAXLOOK];                              // ranges of the last full day sessions
        double rsiUp, rsiDn, rsiPrev, rsi;                 // Wilder RSI of day-session closes; rsi < 0 = not ready
        int    rsiCnt;
        int    nrCount;                                    // last session's range is the smallest of this many full sessions
        int    vaOK, histBase, histLo, histHi;             // prior full session's value area
        double vaPOC, vaH, vaL;
        double hist[HB];                                   // volume by price bin, the session in progress
        Strat  s[NS];

        void ResetAll()
        {
            std::memset(this, 0, sizeof(*this));
            lastCommitted = -1;
            for (int q = 0; q < NS; ++q) s[q].sigBar = -1;
            rsi = -1.0;
        }
    };

    static inline bool DirOn(const Params& P, int d)
    {
        return P.Directions == DIR_BOTH || (d > 0 && P.Directions == DIR_LONG) || (d < 0 && P.Directions == DIR_SHORT);
    }
    static inline const char* DirName(int d) { return d > 0 ? "LONG" : "SHORT"; }

    template <class F> static void CloseVirtual(F& f, Engine& E, const Params& P, int si, int how, double ex, int k)
    {
        Strat& S = E.s[si];
        VTrade& v = S.vt;
        const double pts = v.dir * (ex - v.px) - P.TallyCost;
        ++S.nTr; ++S.nEx[how]; ++S.nDir[v.dir > 0 ? 0 : 1];
        S.sumPts += pts;
        if (pts > 0.0) { ++S.nWin; S.grossW += pts; } else S.grossL -= pts;
        if (P.Verbose || (!E.quiet && E.virtualLog))
        {
            char ts[48], msg[320];
            StampStr(ts, sizeof ts, f.Date(k), f.Tod(k));
            std::snprintf(msg, sizeof msg, "DSU %s virtual #%d %s exit %s at %.2f (%s) -> %+.2f pts after %.2f cost | entry %.2f",
                          kStrat[si], v.seq, DirName(v.dir), how == EX_STOP ? "STOP" : "EOD", ex, ts, pts, P.TallyCost, v.px);
            f.Log(msg, 0);
        }
        v.state = VS_FLAT;
    }

    template <class F> static void LogTally(F& f, const Engine& E, const Params& P)
    {
        for (int si = 0; si < NS; ++si)
        {
            const Strat& S = E.s[si];
            char msg[700];
            std::snprintf(msg, sizeof msg,
                          "DSU TALLY %s (diagnostic, not P&L: chart bars, 1 virtual contract, stop first on a shared bar, %.2f pt cost; "
                          "legs refused on one account are still counted) | sessions %d | trades %d (long %d, short %d) | winners %d (%.1f%%) | "
                          "profit factor %.2f | net %+.2f pts = $%.0f per contract | exits: stop %d, EOD %d", kStrat[si], P.TallyCost,
                          E.sessions, S.nTr, S.nDir[0], S.nDir[1], S.nWin, S.nTr > 0 ? 100.0 * S.nWin / S.nTr : 0.0,
                          S.grossL > 0.0 ? S.grossW / S.grossL : 0.0, S.sumPts, S.sumPts * E.usdPerPt, S.nEx[EX_STOP], S.nEx[EX_EOD]);
            f.Log(msg, 0);
        }
    }

    // Publish a strategy's entry signal (always a market entry at the next bar) and open its virtual trade.
    template <class F> static void Emit(F& f, Engine& E, const Params& P, int si, int k, int d, double dist, const char* what)
    {
        Strat& S = E.s[si];
        const double tick = f.Tick();
        dist = DMax(RoundNear(dist, tick), 2.0 * tick);
        ++S.sigSeq; ++S.tradesToday;
        S.sigBar = k; S.sigDir = d; S.sigStopDist = dist;
        S.sigKey = (int64_t)f.Date(k) * 86400 + f.Tod(k);
        VTrade& v = S.vt;
        std::memset(&v, 0, sizeof v);
        v.state = VS_PEND_MKT; v.dir = d; v.seq = S.sigSeq; v.dist = dist;
        f.Out((d > 0 ? SG_LONG0 : SG_SHORT0) + si, k) = d > 0 ? (float)(f.L(k) - (8.0 + 6.0 * si) * tick) : (float)(f.H(k) + (8.0 + 6.0 * si) * tick);
        if (P.Verbose || !E.quiet)
        {
            char ts[48], msg[520];
            StampStr(ts, sizeof ts, f.Date(k), f.Tod(k));
            std::snprintf(msg, sizeof msg, "DSU %s SIGNAL #%d %s %s | %s | close %.2f | prior close %.2f | avg range %.2f | stop distance "
                          "%.2f pts", kStrat[si], S.sigSeq, DirName(d), ts, what, f.C(k), E.prevClose, E.adr, dist);
            f.Log(msg, 0);
        }
    }

    // A strategy's virtual trade on bar k: fills at the open, stop first, flat at the EOD bar's open.
    template <class F> static void VirtualStep(F& f, Engine& E, const Params& P, int si, int k, bool flatBar)
    {
        VTrade& v = E.s[si].vt;
        if (v.state == VS_PEND_MKT)
        {
            if (flatBar) v.state = VS_FLAT;
            else { v.state = VS_OPEN; v.px = f.O(k); v.stop = v.px - v.dir * v.dist; }
        }
        if (v.state == VS_OPEN)
        {
            if (flatBar) CloseVirtual(f, E, P, si, EX_EOD, f.O(k), k);
            else if (v.dir > 0 ? f.L(k) <= v.stop : f.H(k) >= v.stop)
                CloseVirtual(f, E, P, si, EX_STOP, v.dir > 0 ? DMin(v.stop, f.O(k)) : DMax(v.stop, f.O(k)), k);
        }
    }

    // ---------------------------------------------------------- decisions
    // DWR daily washout rebound. Decided on the bar that ENDS at the day-session open, so the order goes in
    // at the open: RSI of the last day-session closes below 'RSI buy' -> long ('RSI sell' short side off by default).
    template <class F> static void PreDecideDWR(F& f, Engine& E, const Params& P, int k)
    {
        Strat& S = E.s[S_DWR];
        f.Out(SG_RSI, k) = (float)E.rsi;
        if (E.rsi < 0.0 || S.vt.state != VS_FLAT || S.tradesToday > 0) return;
        const int d = E.rsi < P.RsiBuy ? 1 : ((P.RsiSell > 0.0 && E.rsi > P.RsiSell) ? -1 : 0);
        if (d == 0 || !DirOn(P, d)) return;
        char what[160];
        std::snprintf(what, sizeof what, "RSI(%d) of day-session closes %.1f %s %.1f: enter at the open, stop %.2f x avg range",
                      P.RsiLen, E.rsi, d > 0 ? "below" : "above", d > 0 ? P.RsiBuy : P.RsiSell, P.DwrStop);
        Emit(f, E, P, S_DWR, k, d, P.DwrStop * E.adr, what);
    }

    // VAR value-area re-entry (the "80% rule"): the session opens OUTSIDE the prior value area; the first
    // 'VarMark' close back INSIDE it, from 'VarFirst' to 'VarLast', is traded towards the far side of value.
    template <class F> static void DecideVAR(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec)
    {
        Strat& S = E.s[S_VAR];
        if (E.vaOK) { f.Out(SG_VAH, k) = (float)E.vaH; f.Out(SG_VAL, k) = (float)E.vaL; f.Out(SG_POC, k) = (float)E.vaPOC; }
        if (S.dayDead) return;
        if (E.barsSeen == 1)                                            // the session's first bar: where did it open?
        {
            if (b != 0 || !E.vaOK) { S.dayDead = 1; return; }           // first bar missing, or the prior session was not a full one
            S.side = E.dOpen > E.vaH ? -1 : (E.dOpen < E.vaL ? 1 : 0);
            if (S.side == 0) { S.dayDead = 1; return; }                 // opened inside value: no trade today
        }
        const int endT = tod + bsec;
        if (endT > P.VarLast) { S.dayDead = 1; return; }
        if (endT < P.VarFirst || (endT - P.SessOpen) % (P.VarMark * 60) != 0) return;
        const double c = f.C(k);
        if (!(c > E.vaL && c < E.vaH)) return;
        S.dayDead = 1;                                                  // one decision per day
        if (!DirOn(P, S.side) || S.vt.state != VS_FLAT) return;
        char what[200];
        std::snprintf(what, sizeof what, "opened %.2f %s the prior value area %.2f-%.2f (POC %.2f), %d-minute close back inside",
                      E.dOpen, S.side < 0 ? "above" : "below", E.vaL, E.vaH, E.vaPOC, P.VarMark);
        Emit(f, E, P, S_VAR, k, S.side, P.VarStop * E.adr, what);
    }

    // CEB compression -> expansion: only after a narrow-range day (NR 'NrDays'); the first bar that CLOSES
    // outside the 'OrMinutes' opening range, up to 'CebLast', is followed either way.
    template <class F> static void DecideCEB(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec)
    {
        Strat& S = E.s[S_CEB];
        const int endT = tod + bsec, orEnd = P.SessOpen + P.OrMinutes * 60;
        f.Out(SG_NR, k) = (float)E.nrCount;
        if (tod >= orEnd && S.orh > 0.0) { f.Out(SG_ORH, k) = (float)S.orh; f.Out(SG_ORL, k) = (float)S.orl; }
        if (S.dayDead) return;
        if (E.nrCount < P.NrDays) { S.dayDead = 1; return; }            // yesterday was not a narrow-range day
        if (tod < orEnd)
        {
            if (E.barsSeen != b + 1) { S.dayDead = 1; return; }         // an opening-range bar is missing
            if (b == 0) { S.orh = f.H(k); S.orl = f.L(k); }
            else { S.orh = DMax(S.orh, f.H(k)); S.orl = DMin(S.orl, f.L(k)); }
            return;
        }
        if (S.orh <= 0.0 || endT > P.CebLast) { S.dayDead = 1; return; }
        const double c = f.C(k);
        const int d = c > S.orh ? 1 : (c < S.orl ? -1 : 0);
        if (d == 0) return;
        S.dayDead = 1;                                                  // one decision per day
        if (!DirOn(P, d) || S.vt.state != VS_FLAT) return;
        char what[200];
        std::snprintf(what, sizeof what, "prior range the smallest of %d sessions; close %.2f %s the %d-minute opening range %.2f-%.2f",
                      E.nrCount, c, d > 0 ? "above" : "below", P.OrMinutes, S.orl, S.orh);
        Emit(f, E, P, S_CEB, k, d, P.CebStop * E.adr, what);
    }

    // EDF end-of-day flow: at the 'EdfMark' bar close, a move of at least 'EdfMove' percent from the prior
    // day-session close is followed into the close (leveraged-ETF rebalancing, short-gamma hedging).
    template <class F> static void DecideEDF(F& f, Engine& E, const Params& P, int k, int tod, int bsec)
    {
        Strat& S = E.s[S_EDF];
        const double band = P.EdfMove / 100.0;
        f.Out(SG_EDF_UP, k) = (float)(E.prevClose * (1.0 + band));
        f.Out(SG_EDF_DN, k) = (float)(E.prevClose * (1.0 - band));
        if (S.dayDead) return;
        const int endT = tod + bsec;
        if (endT < P.EdfMark) return;
        S.dayDead = 1;                                                  // one decision per day
        if (endT != P.EdfMark) return;                                  // the decision bar is missing from the data
        const double mv = f.C(k) / E.prevClose - 1.0;
        if (DAbs(mv) * 100.0 < P.EdfMove) return;
        const int d = mv > 0.0 ? 1 : -1;
        if (!DirOn(P, d) || S.vt.state != VS_FLAT) return;
        char what[200];
        std::snprintf(what, sizeof what, "%+.2f%% from the prior close %.2f at the decision bar (min %.2f%%): trade with the move into the close",
                      100.0 * mv, E.prevClose, P.EdfMove);
        Emit(f, E, P, S_EDF, k, d, P.EdfStop * E.adr, what);
    }

    // ------------------------------------------------------- session banking
    // Value area of the finished session: the price bin with the most volume, widened one bin at a
    // time towards the larger neighbour (ties go up) until 'VaFrac' of the volume is inside.
    static void ValueArea(Engine& E, const Params& P)
    {
        E.vaOK = 0;
        if (E.histHi < E.histLo) return;
        int    p = E.histLo;
        double tot = 0.0;
        for (int q = E.histLo; q <= E.histHi; ++q) { tot += E.hist[q]; if (E.hist[q] > E.hist[p]) p = q; }
        if (tot <= 0.0) return;
        int    lo = p, hi = p;
        double acc = E.hist[p];
        while (acc < P.VaFrac * tot)
        {
            const double up = hi + 1 <= E.histHi ? E.hist[hi + 1] : -1.0;
            const double dn = lo - 1 >= E.histLo ? E.hist[lo - 1] : -1.0;
            if (up < 0.0 && dn < 0.0) break;
            if (up >= dn) { ++hi; acc += up; } else { --lo; acc += dn; }
        }
        E.vaPOC = (E.histBase + p + 0.5) * P.VaBin;
        E.vaH   = (E.histBase + hi + 1) * P.VaBin;
        E.vaL   = (E.histBase + lo) * P.VaBin;
        E.vaOK  = 1;
    }

    // The finished day session becomes history: range ring, RSI of closes, value area, narrow-range count.
    static void BankSession(Engine& E, const Params& P)
    {
        const bool full  = E.barsSeen >= E.nb - 2 && !E.halt && E.dOpen > 0.0;
        const bool valid = E.barsSeen * 100 >= E.nb * 35;              // enough of the session for its close to count
        E.nrCount = 0; E.vaOK = 0;
        if (full)
        {
            E.ring[E.ringHead] = E.dHigh - E.dLow;
            E.ringHead = (E.ringHead + 1) % MAXLOOK;
            if (E.ringN < MAXLOOK) ++E.ringN;
            const double last = E.ring[(E.ringHead - 1 + MAXLOOK) % MAXLOOK];
            for (int q = 1; q <= E.ringN; ++q)
            {
                if (E.ring[(E.ringHead - q + 2 * MAXLOOK) % MAXLOOK] < last) break;
                E.nrCount = q;
            }
            ValueArea(E, P);
        }
        E.prevClose = valid ? E.dClose : 0.0;
        if (valid)
        {
            if (E.rsiPrev > 0.0)
            {
                const double r = E.dClose / E.rsiPrev - 1.0, u = r > 0.0 ? r : 0.0, d = r < 0.0 ? -r : 0.0;
                if (E.rsiCnt == 0) { E.rsiUp = u; E.rsiDn = d; }
                else { E.rsiUp = (E.rsiUp * (P.RsiLen - 1) + u) / P.RsiLen; E.rsiDn = (E.rsiDn * (P.RsiLen - 1) + d) / P.RsiLen; }
                ++E.rsiCnt;
            }
            E.rsiPrev = E.dClose;
        }
        E.rsi = E.rsiCnt >= 10 ? (E.rsiDn <= 0.0 ? 100.0 : 100.0 - 100.0 / (1.0 + E.rsiUp / E.rsiDn)) : -1.0;
    }

    // ------------------------------------------------------------ bar driver
    template <class F> static void ProcessBar(F& f, Engine& E, const Params& P, int k)
    {
        const int date = f.Date(k), tod = f.Tod(k);
        const int bsec = f.BarSec() > 0 ? f.BarSec() : 300;
        for (int g = 0; g < SG_ENGINE_END; ++g) f.Out(g, k) = 0.0f;
        if (tod >= P.SessClose) return;
        const bool pre = tod < P.SessOpen;
        if (pre && tod + bsec != P.SessOpen) return;                   // only the day session, and the bar that ends at its open

        // (1) once per date, at the first bar handled for it: bank the last session, build today's references
        if (date != E.refDate)
        {
            for (int si = 0; si < NS; ++si)
            {
                Strat& S = E.s[si];
                if (S.vt.state == VS_OPEN) CloseVirtual(f, E, P, si, EX_EOD, E.dClose, k);   // data ended before the flat time
                S.vt.state = VS_FLAT;
                S.tradesToday = 0; S.dayDead = 0; S.side = 0; S.orh = 0.0; S.orl = 0.0;
            }
            if (E.curDate != 0 && E.curDate != E.bankedDate) { BankSession(E, P); E.bankedDate = E.curDate; }
            E.refDate = date;
            E.adr = 0.0;
            if (E.ringN >= P.Lookback)
            {
                double r = 0.0;
                for (int q = 1; q <= P.Lookback; ++q) r += E.ring[(E.ringHead - q + 2 * MAXLOOK) % MAXLOOK];
                E.adr = r / P.Lookback;
            }
            // tradable: averages ready, a prior close, a normal (not holiday-shortened) session
            E.refOK = (E.adr > 0.0 && E.prevClose > 0.0 && ExchangeHaltTod(date) == 0) ? 1 : 0;
        }
        if (pre)
        {
            if (E.refOK) PreDecideDWR(f, E, P, k);
            return;
        }

        // (2) a new day session
        if (date != E.curDate)
        {
            E.curDate = date; ++E.sessions;
            E.nb = IMin((P.SessClose - P.SessOpen) / bsec, MAXB);
            E.halt = ExchangeHaltTod(date);
            E.barsSeen = 0; E.lastB = -1;
            E.dOpen = f.O(k); E.dHigh = f.H(k); E.dLow = f.L(k); E.dClose = f.C(k);
            E.histBase = (int)std::floor(E.dOpen / P.VaBin + 1e-9) - HB / 2;
            E.histLo = HB; E.histHi = -1;
            std::memset(E.hist, 0, sizeof E.hist);
            E.dayOK = (E.refOK && !E.halt && E.dOpen > 0.0) ? 1 : 0;
        }
        const int b = (tod - P.SessOpen) / bsec;
        if (b < 0 || b >= E.nb) return;
        const bool flatBar = tod >= P.EOD;

        // (3) the strategies' virtual trades on this bar
        for (int si = 0; si < NS; ++si) VirtualStep(f, E, P, si, k, flatBar);

        // (4) session statistics including this bar
        E.dHigh = DMax(E.dHigh, f.H(k)); E.dLow = DMin(E.dLow, f.L(k)); E.dClose = f.C(k);
        {
            const double w = DMax(f.V(k), 1.0);
            int a = (int)std::floor(f.L(k) / P.VaBin + 1e-9) - E.histBase, e = (int)std::floor(f.H(k) / P.VaBin + 1e-9) - E.histBase;
            a = IMax(0, IMin(a, HB - 1)); e = IMax(a, IMin(e, HB - 1));
            const double part = w / (e - a + 1);                           // the bar's volume spread evenly over its range
            for (int q = a; q <= e; ++q) E.hist[q] += part;
            E.histLo = IMin(E.histLo, a); E.histHi = IMax(E.histHi, e);
        }
        E.lastB = b; ++E.barsSeen;

        // (5) decisions at this close (a switched-off strategy still runs its tally)
        if (E.dayOK && !flatBar)
        {
            f.Out(SG_RSI, k) = (float)E.rsi;
            DecideVAR(f, E, P, k, b, tod, bsec);
            DecideCEB(f, E, P, k, b, tod, bsec);
            DecideEDF(f, E, P, k, tod, bsec);
        }
    }
}   // namespace dsu
