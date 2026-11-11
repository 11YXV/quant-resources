#include "sierrachart.h"

SCDLLName("DiversifiedSuite_NQ_MNQ")

/*============================================================================
  DiversifiedSuite_NQ_MNQ  v1  --  four strategies, ONE trade account
  (Lessons #12: the two lines above stay at the very top of the file.)

  WHAT IT DOES
    Runs the four non-momentum MNQ strategies of README_ReversionTrio.md in
    one study, so they can be Replay-tested tick by tick and traded on one
    account. Each has its own on/off switch and contract count, so each can
    also be tested alone (it then trades exactly like its single file):
      DWR  Daily washout rebound: 2-period RSI of the day-session closes
           below 30 -> buy the 09:30 open; stop 0.75 x average day range.
      VAR  Value-area re-entry: the session opens outside the prior value
           area (70% of its volume); the first 30-minute close back inside,
           10:00-12:30 -> trade towards the far side; stop 0.20 x range.
      CEB  Compression-expansion breakout: only after a narrow-range day
           (NR4); the first 5-minute close outside the 30-minute opening
           range, up to 13:00 -> follow it; stop 0.25 x range.
      EDF  End-of-day flow: at 15:00, at least 1.0% from the prior close ->
           trade with the move into the close; stop 0.40 x range.
    No targets. One trade per strategy per day. All flat at 15:55 New York.
    average day range = mean high-low of the last 14 full day sessions.
    Rules, sources and the full research record: README_ReversionTrio.md
    and the four single-strategy files (the engines here are the same code).

  HOW ONE ACCOUNT HOLDS FOUR STRATEGIES
    L1  Each strategy owns one LEG: its own market entry with its own
        ATTACHED stop (Stop1Offset from the actual fill). A leg is followed
        by its own order IDs, never by the account position (lessons #80).
    L2  The account position must always equal the sum of the open legs. A
        difference that lasts 5 s of trade time flattens everything.
    L3  A leg may open only if no other leg is open or working in the
        OPPOSITE direction. In 2017-2024 that refused 95 of 1,902 signals
        (VAR 42, EDF 30, CEB 23; nearly all against an open DWR long). They
        made +$1,018 net in seven years, so one account costs about $12 a
        month against four separate accounts.
    L4  A leg ends on its attached stop or at the flat time (one
        FlattenAndCancelAllOrders for all legs). No stop is ever modified.
    L5  Day limits use the ACCOUNT's realised P&L since the first call of
        the trading day plus the open legs: daily loss halt (flattens),
        profit lock, losing legs in a row. 0 = off (#36). Reset on a new
        trading day (#43).
    L6  Size per leg: fixed contracts (each strategy's input), or risk % of
        the account size input over that leg's stop, at most 'Max contracts
        per leg'. Optional caps: $ risk per leg, and $ risk of all open legs
        together at their stops (reduces the new leg, then skips it).
        MaximumPositionAllowed = the enabled legs' sizes added up.
    L7  Everything else follows the single-strategy files: one position read
        per call (#17), CHART CHECK and feed checks, late-signal limit, CME
        short sessions, flatten re-issue, settings written on every call.

  ORDER LIFECYCLE (per leg)
    SIGNAL (engine, at a bar close) -> ENTRY (market, own attached stop) ->
    FILL (parent order FILLED; fill price read from the order) -> STOP
    working (FILL VERIFY once; every leg's stop must cover its contracts) ->
    EXIT (attached stop filled, or the 15:55 flatten) -> leg P&L logged, leg
    reset. The account P&L used by the halts is Sierra's own.

  MEASURED (this file in the mock: NQ 1-minute data 2017-01 .. 2024-03 as 4
  ticks per bar, 1 MNQ per leg, $2.50 per contract round trip)
    1,792 legs | 48.5% winners | profit factor 1.45 | net +$40,400 |
    $464 per month (median $176) | 55 of 87 months positive | best month
    +$3,810, worst -$1,054 | max drawdown -$3,859 | worst day -$944 |
    longest losing streak 10 | average win $149, average loss -$96 |
    0.96 legs per day, at most 3 open at once.
    By year: 2017 +508, 2018 +2,142, 2019 +117, 2020 +10,540, 2021 +5,319,
    2022 +13,206, 2023 +7,780, 2024 (to March) +790.
    2017-2021: PF 1.41, $310/month. 2022-2024: PF 1.50, $830/month.
    At $5.00 per round trip: $413/month; at $7.50: $361/month.
    By leg ($/month, PF): DWR 181 (1.38) | VAR 63 (1.32) | CEB 88 (1.45) |
    EDF 133 (1.87).
    Monte Carlo, one year (5-day blocks): 4.2% of years lose; drawdown
    median -$1,651, 99th percentile -$5,006.
    Each leg alone reproduces its single-strategy file fill for fill; all
    four together equal those trades minus the 95 refused legs, to the cent.
    READ BEFORE SIZING: about 85 ideas were tried to find these four, so the
    survivors flatter themselves. EDF made half its profit in 2022 and is the
    least proven before 2022; VAR has a t-statistic of 1.7 on its own; CEB
    did not carry over to ES. 2017 and 2019 made almost nothing. Drawdowns
    scale with the index level: roughly 2.4 x the figures above at NQ
    25,000. A daily loss halt tighter than one full stop of the widest leg
    (DWR, 0.75 x range) cuts the washout trades it depends on.
    NOT TESTED: anything after 2024-03-07; anything in Sierra.

  FIRST REPLAY CHECKS (unverified until seen in Sierra)
    1 "DSU <leg> FILL VERIFY ok" for each leg (MARKET parent with
      Stop1Offset: the stop at fill -/+ offset, difference 0.00).
    2 Two legs open at once: two working stops, position = 2; one stop
      filling leaves the other leg's stop working and position = 1.
    3 s_SCTradeOrder::AvgFillPrice is non-zero for filled parents and stops
      (used for the per-leg P&L line only; the halts use the account P&L).
    4 One "engine SIGNAL #n ... TRADED / NOT traded" line per SIGNAL line;
      "opposite to a leg" refusals only when another leg is open.
    5 Number of Open Trades = 0 at the end; every exit before 15:56.

  BACKTEST IN SIERRA (tick by tick, one account)
    MNQ continuous contract (volume rollover, back-adjusted), 5-minute bars,
    New York time zone, overnight session loaded (DWR decides on the 09:25
    bar), at least 30 days loaded before the start. Trade Simulation Mode
    ON, commission set (#55). Clear trade data (#53, #71). Chart >> Replay
    Chart, Accurate Trading System Back Test mode (#54). No other study may
    trade this symbol on the same Trade Account (#72): do not add the four
    single-strategy studies or MomentumSuite to the same account.

  STORAGE: Persistent pointer 1 = Engine (heap, rebuilt on a full
    recalculation), pointer 2 = Trader (heap, kept). Freed on the last call.
  INSTALL: ACS_Source\DiversifiedSuite_NQ_MNQ.cpp, Build Custom Studies DLL,
    add "Diversified Suite NQ MNQ v1" to an MNQ 5-minute chart. Keep the
    Trade Window "Attached Orders" list EMPTY. Message Log lines start "DSU".
============================================================================*/

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

#ifndef DSU_CORE_ONLY
namespace dsu
{
    struct ScFeed
    {
        SCStudyInterfaceRef sc;
        explicit ScFeed(SCStudyInterfaceRef s) : sc(s) {}
        double O(int k) const { return sc.Open[k]; }
        double H(int k) const { return sc.High[k]; }
        double L(int k) const { return sc.Low[k]; }
        double C(int k) const { return sc.Close[k]; }
        double V(int k) const { return sc.Volume[k]; }
        int    Date(int k) const { return sc.BaseDateTimeIn.DateAt(k); }
        int    Tod(int k) const  { return sc.BaseDateTimeIn.TimeAt(k); }
        int    BarSec() const    { return sc.SecondsPerBar; }
        double Tick() const      { return sc.TickSize; }
        float& Out(int g, int k) { return sc.Subgraph[g][k]; }
        void   Log(const char* m, int level) { sc.AddMessageToLog(m, level); }
    };

    struct OParams
    {
        int    OrderMode   = 0;        // 0 = send orders, 1 = signals only
        int    AllowLive   = 0;
        int    SizeMode    = 0;        // 0 = fixed contracts per leg, 1 = risk % of the account size input per leg
        double Account     = 10000.0;  // used only by SizeMode 1 (an input, never the broker balance: lessons #5)
        double RiskPct     = 1.0;      // per leg
        int    MaxQty      = 3;        // per leg, SizeMode 1
        double MaxRiskUSD  = 0.0;      // per leg, 0 = off
        double MaxOpenRisk = 0.0;      // $ at the stops of all open and working legs together, 0 = off
        double DailyLoss   = 0.0;      // $, account, 0 = off
        double ProfitLock  = 0.0;      // $, account, 0 = off
        int    MaxConsec   = 0;        // losing legs in a row today, 0 = off
        int    Safety      = 0;        // 0 = block, 1 = warn only
        int    Inherited   = 0;        // 0 = flatten, 1 = leave alone
        int    EntryDelay  = 120;      // s; input 0 = 120, -1 = off
        int    EarlyClose  = 0;        // 0 = honour CME short sessions, 1 = ignore
        int    Fallbacks   = 0;
    };
    static inline int LegCap(const Params& P, const OParams& O, int s) { return O.SizeMode == 0 ? P.Qty[s] : O.MaxQty; }
    static inline int TotalCap(const Params& P, const OParams& O)
    {
        int n = 0;
        for (int s = 0; s < NS; ++s) if (P.On[s]) n += LegCap(P, O, s);
        return n;
    }

    struct InputReader                                       // range-checked reads; fallbacks are counted (lessons #15)
    {
        SCStudyInterfaceRef sc;
        int& fb;
        int GI(int idx, int lo, int hi, int def) { const int v = sc.Input[idx].GetInt(); if (v < lo || v > hi) { ++fb; return def; } return v; }
        double GF(int idx, double lo, double hi, double def)
        {
            const double v = (double)sc.Input[idx].GetFloat();
            if (!(v >= lo && v <= hi)) { ++fb; return def; }
            return v;
        }
        int GT(int idx, int def) { const int v = sc.Input[idx].GetTime(); if (v < 0 || v > 86399) { ++fb; return def; } return v; }
        int GX(int idx, int n, int def) { const int v = (int)sc.Input[idx].GetIndex(); if (v < 0 || v >= n) { ++fb; return def; } return v; }
    };

    static Params ReadParams(SCStudyInterfaceRef sc)
    {
        Params P;
        const Params D;
        InputReader R{ sc, P.Fallbacks };
        P.On[S_DWR]  = sc.Input[IN_DWR_ON].GetYesNo() ? 1 : 0;
        P.On[S_VAR]  = sc.Input[IN_VAR_ON].GetYesNo() ? 1 : 0;
        P.On[S_CEB]  = sc.Input[IN_CEB_ON].GetYesNo() ? 1 : 0;
        P.On[S_EDF]  = sc.Input[IN_EDF_ON].GetYesNo() ? 1 : 0;
        P.Qty[S_DWR] = R.GI(IN_DWR_QTY, 1, 100, D.Qty[S_DWR]);
        P.Qty[S_VAR] = R.GI(IN_VAR_QTY, 1, 100, D.Qty[S_VAR]);
        P.Qty[S_CEB] = R.GI(IN_CEB_QTY, 1, 100, D.Qty[S_CEB]);
        P.Qty[S_EDF] = R.GI(IN_EDF_QTY, 1, 100, D.Qty[S_EDF]);
        P.RsiLen     = R.GI(IN_DWR_RSI_LEN, 2, 14, D.RsiLen);
        P.RsiBuy     = R.GF(IN_DWR_RSI_BUY, 1.0, 99.0, D.RsiBuy);
        P.RsiSell    = R.GF(IN_DWR_RSI_SELL, 0.0, 99.0, D.RsiSell);
        P.DwrStop    = R.GF(IN_DWR_STOP, 0.05, 5.0, D.DwrStop);
        P.VaFrac     = R.GF(IN_VAR_FRAC, 0.30, 0.95, D.VaFrac);
        P.VaBinTicks = R.GI(IN_VAR_BIN, 1, 400, D.VaBinTicks);
        P.VaBin      = sc.TickSize > 0.0f ? P.VaBinTicks * (double)sc.TickSize : 1.0;
        P.VarMark    = R.GI(IN_VAR_MARK, 5, 120, D.VarMark);
        P.VarFirst   = R.GT(IN_VAR_FIRST, D.VarFirst);
        P.VarLast    = R.GT(IN_VAR_LAST, D.VarLast);
        P.VarStop    = R.GF(IN_VAR_STOP, 0.02, 5.0, D.VarStop);
        P.NrDays     = R.GI(IN_CEB_NR, 1, 20, D.NrDays);
        P.OrMinutes  = R.GI(IN_CEB_OR, 5, 180, D.OrMinutes);
        P.CebLast    = R.GT(IN_CEB_LAST, D.CebLast);
        P.CebStop    = R.GF(IN_CEB_STOP, 0.02, 5.0, D.CebStop);
        P.EdfMove    = R.GF(IN_EDF_MOVE, 0.05, 20.0, D.EdfMove);
        P.EdfMark    = R.GT(IN_EDF_MARK, D.EdfMark);
        P.EdfStop    = R.GF(IN_EDF_STOP, 0.02, 5.0, D.EdfStop);
        P.SessOpen   = R.GT(IN_SESS_OPEN, D.SessOpen);
        P.SessClose  = R.GT(IN_SESS_CLOSE, D.SessClose);
        P.EOD        = R.GT(IN_EOD, D.EOD);
        if (!(P.SessOpen < P.EOD && P.EOD < P.SessClose)) { ++P.Fallbacks; P.SessOpen = D.SessOpen; P.SessClose = D.SessClose; P.EOD = D.EOD; }
        P.Directions = R.GX(IN_DIRECTIONS, 3, D.Directions);
        P.Lookback   = R.GI(IN_LOOKBACK, 3, MAXLOOK, D.Lookback);
        P.TallyCost  = R.GF(IN_TALLY_COST, 0.0, 50.0, D.TallyCost);
        P.Verbose    = sc.Input[IN_VERBOSE].GetYesNo() ? 1 : 0;
        return P;
    }

    static OParams ReadOParams(SCStudyInterfaceRef sc)
    {
        OParams O;
        const OParams D;
        InputReader R{ sc, O.Fallbacks };
        O.OrderMode   = R.GX(IN_ORDER_MODE, 2, D.OrderMode);
        O.AllowLive   = sc.Input[IN_ALLOW_LIVE].GetYesNo() ? 1 : 0;
        O.SizeMode    = R.GX(IN_SIZE_MODE, 2, D.SizeMode);
        O.Account     = R.GF(IN_ACCOUNT, 100.0, 1.0e9, D.Account);
        O.RiskPct     = R.GF(IN_RISK_PCT, 0.01, 20.0, D.RiskPct);
        O.MaxQty      = R.GI(IN_MAX_QTY, 1, 100, D.MaxQty);
        O.MaxRiskUSD  = R.GF(IN_MAX_RISK_USD, 0.0, 1.0e7, D.MaxRiskUSD);
        O.MaxOpenRisk = R.GF(IN_MAX_OPEN_RISK, 0.0, 1.0e7, D.MaxOpenRisk);
        O.DailyLoss   = R.GF(IN_DAILY_LOSS, 0.0, 1.0e7, D.DailyLoss);
        O.ProfitLock  = R.GF(IN_PROFIT_LOCK, 0.0, 1.0e7, D.ProfitLock);
        O.MaxConsec   = R.GI(IN_MAX_CONSEC, 0, 50, D.MaxConsec);
        O.Safety      = R.GX(IN_SAFETY, 2, D.Safety);
        O.Inherited   = R.GX(IN_INHERITED, 2, D.Inherited);
        const int ed  = R.GI(IN_ENTRY_DELAY, -1, 3600, 0);
        O.EntryDelay  = ed == 0 ? D.EntryDelay : ed;                 // 0 = the safe default, -1 = off (lessons #64)
        O.EarlyClose  = R.GX(IN_EARLY_CLOSE, 2, D.EarlyClose);
        return O;
    }

    static void HHMM(char* out, size_t n, int t) { std::snprintf(out, n, "%02d:%02d", t / 3600, (t / 60) % 60); }   // n >= 16

    static int TradeDate(int date, int tod)                          // bars from 17:00 belong to the next weekday
    {
        const int wd = WeekDay(date);
        if (tod < 17 * 3600 && wd != 0 && wd != 6) return date;
        int n = date + 1;
        while (WeekDay(n) == 0 || WeekDay(n) == 6) ++n;
        return n;
    }

    enum : int { LG_FLAT = 0, LG_PENDING = 1, LG_OPEN = 2 };
    enum : int { FW_NONE = 0, FW_EOD, FW_UNPROTECTED, FW_WRONG_SIDE, FW_MISMATCH, FW_REISSUE, FW_INHERITED, FW_DAILY_LOSS, FW_COUNT };
    static const char* const kFlatWhy[FW_COUNT] =
    {
        "-", "flat time", "a leg's stop is not working (#14)", "attached stop on the wrong side of the fill",
        "account position differs from the open legs", "flatten re-issued", "untracked position outside trading hours",
        "daily loss limit reached"
    };
    static const int kTraderMagic = 0x44535531;

    struct Leg                                              // one strategy's order pair; time keys in trade-time seconds
    {
        int     state, dir, sigSeq, qty, canceling, fillVerified, covShort;
        double  entryRef, dist, entryFill;
        int64_t parentID, stopID, submitAbs, fillAbs, cancelAbs, covShortAbs;
        char    cancelWhy[80];
    };

    // Order-layer state. Heap, PersistentPointer 2. Survives full recalculations (#38, #43);
    // it holds order IDs and time keys only, never bar indices (#37).
    struct Trader
    {
        int     magic;
        Leg     leg[NS];
        int     sigSeqDone[NS];
        int64_t sigKeyDone[NS];
        int     flattenPending, flattenWhy, flattenCount, mismatch;
        int64_t flattenAbs, mismatchAbs, staleCancelAbs;
        double  flattenPx;
        int     postRecalc, flatSeen, inheritedWarned, liveBlockWarned, bannerDone, recalcLogged;
        int     cfgDone, cfgFail, tzState, feedWarnDay, clockWarned;
        char    cfgWhy[240];
        // trading day (reset only when the trading day changes, #43)
        int     lastDay, legsToday, winsToday, lossesToday, consecLosses, sigToday, sigTradedToday, haltLogged, baseTrades;
        double  baseCumPL, legPLToday;
        int     runLegs, runWins;
        double  runPL;
        void Init()
        {
            std::memset(this, 0, sizeof(*this));
            magic = kTraderMagic;
            postRecalc = 1;                         // signals already on the chart are history (#63)
        }
    };

    static void OrderLayer(SCStudyInterfaceRef sc, const Engine& E, const Params& P, const OParams& O, Trader& T)
    {
        const int    i    = sc.Index;
        const double tick = sc.TickSize;

        if (sc.IsFullRecalculation)                                   // order functions are no-ops here
        {
            T.postRecalc = 1; T.flatSeen = 0; T.inheritedWarned = 0; T.liveBlockWarned = 0; T.bannerDone = 0; T.cfgDone = 0;
            if (i == sc.ArraySize - 1 && !T.recalcLogged)
            {
                T.recalcLogged = 1;
                sc.AddMessageToLog("DSU ORDERS: full recalculation complete -- orders are sent only on real-time, Chart Replay or "
                                   "back-test bars; a quiet historical load is expected, not a fault.", 0);
            }
            return;
        }
        T.recalcLogged = 0;
        if (tick <= 0.0 || i < sc.ArraySize - 2) return;                     // only the live edge acts
        if (sc.ChartIsDownloadingHistoricalData(sc.ChartNumber)) return;

        const int     nowBar  = sc.ArraySize - 1;
        const int     barDate = sc.BaseDateTimeIn.DateAt(nowBar);
        const int     barTod  = sc.BaseDateTimeIn.TimeAt(nowBar);
        const int     bsec    = sc.SecondsPerBar > 0 ? sc.SecondsPerBar : 300;
        int           nowTod  = barTod;
        if (sc.LatestDateTimeForLastBar.IsDateSet() && sc.LatestDateTimeForLastBar.GetDate() == barDate)
        {
            const int lt = sc.LatestDateTimeForLastBar.GetTime();             // time of the last trade in the live bar
            if (lt > nowTod) nowTod = lt;
        }
        const int     nowTD    = TradeDate(barDate, nowTod);
        const int64_t nowAbs   = (int64_t)barDate * 86400 + nowTod;           // trade time, survives midnight
        const double  lastPx   = sc.Close[nowBar];
        const double  usdPerPt = sc.CurrencyValuePerTick > 0.0f ? (double)sc.CurrencyValuePerTick / tick : 0.0;
        const bool    engOK    = E.lastCommitted >= nowBar - 1;               // the engine has every closed bar
        // CME holiday session / early close: the halt is the day's close; the flat time keeps its distance to it.
        const int     haltTod  = O.EarlyClose == 0 ? ExchangeHaltTod(nowTD) : 0;
        const int     effEOD   = haltTod > 0 ? haltTod - IMax(P.SessClose - P.EOD, 60) : P.EOD;
        const bool    inHours  = nowTod >= P.SessOpen && nowTod < effEOD;
        bool          acted    = false;                               // a flatten / cancel-all was issued in this call
        char          msg[1000], hm[16], hm2[16];

        // ---- THE one position read of this call, before any order action (#17)
        s_SCPositionData pos;
        sc.GetTradePosition(pos);
        const double posQ    = (double)pos.PositionQuantity;
        const int    posNet  = (int)(posQ > 0.0 ? posQ + 0.5 : posQ - 0.5);
        const bool   working = pos.WorkingOrdersExist != 0;
        const bool   liveBlocked = !sc.GlobalTradeSimulationIsOn && !O.AllowLive;

        // ---- data feed, real time only (Chart Replay and back tests have no feed to check)
        const bool realTime = sc.IsReplayRunning() == 0;
        const bool feedDown = realTime && sc.ServerConnectionState != SCS_CONNECTED;
        int        rawLag   = 0;
        bool       delayed  = false;
        if (realTime && sc.CurrentSystemDateTime.IsDateSet() && sc.LatestDateTimeForLastBar.IsDateSet())
        {
            rawLag = (sc.CurrentSystemDateTime.GetDate() - sc.LatestDateTimeForLastBar.GetDate()) * 86400
                   + (sc.CurrentSystemDateTime.GetTime() - sc.LatestDateTimeForLastBar.GetTime());
            if (!feedDown && inHours)
            {
                if (rawLag > 90 && rawLag <= 3600) delayed = true;             // e.g. a 10-minute delayed feed (#65)
                else if ((rawLag > 3600 || rawLag < -3600) && !T.clockWarned)
                {
                    T.clockWarned = 1;
                    std::snprintf(msg, sizeof msg, "DSU FEED: system time and last-trade time differ by %d s -- not comparable; the "
                                  "delayed-data check is inactive.", rawLag);
                    sc.AddMessageToLog(msg, 0);
                }
            }
        }
        const bool blockSafety = O.Safety == 0;
        const bool feedBlock   = blockSafety && (feedDown || (delayed && !sc.GlobalTradeSimulationIsOn));
        if ((feedDown || delayed) && T.feedWarnDay != nowTD)
        {
            T.feedWarnDay = nowTD;
            if (feedDown) std::snprintf(msg, sizeof msg, "DSU FEED: data feed not connected (state %d) -- %s.", sc.ServerConnectionState,
                                        blockSafety ? "no new entries; open legs keep their attached stops" : "warn only");
            else std::snprintf(msg, sizeof msg, "DSU FEED: the last trade is %d s behind the system clock -- the data looks DELAYED. %s", rawLag,
                               sc.GlobalTradeSimulationIsOn ? "Simulation: entries continue; results will not match live trading."
                                                            : (blockSafety ? "LIVE entries blocked until the feed is real time." : "warn only."));
            sc.AddMessageToLog(msg, 1);
        }

        auto ReadOrder = [&](int64_t id, s_SCTradeOrder& o) -> bool
        {
            return id > 0 && sc.GetOrderByOrderID(static_cast<uint64_t>(id), o) != SCTRADING_ORDER_ERROR;
        };
        auto AnyLeg = [&](int st) -> bool { for (int s = 0; s < NS; ++s) if (T.leg[s].state == st) return true; return false; };
        auto ExpectedNet = [&]() -> int
        {
            int n = 0;
            for (int s = 0; s < NS; ++s) if (T.leg[s].state == LG_OPEN) n += T.leg[s].dir * T.leg[s].qty;
            return n;
        };
        auto OpenPL = [&]() -> double
        {
            double pl = 0.0;
            for (int s = 0; s < NS; ++s)
                if (T.leg[s].state == LG_OPEN) pl += T.leg[s].dir * (lastPx - T.leg[s].entryFill) * usdPerPt * T.leg[s].qty;
            return pl;
        };
        auto OpenRiskUSD = [&]() -> double                            // planned risk at the stops of every open or working leg
        {
            double r = 0.0;
            for (int s = 0; s < NS; ++s) if (T.leg[s].state != LG_FLAT) r += T.leg[s].dist * usdPerPt * T.leg[s].qty;
            return r;
        };

        // A leg has ended: log its P&L from the order fill prices (an estimate; the account P&L is Sierra's) and reset it.
        auto CloseLeg = [&](int s, double exitPx, const char* how)
        {
            Leg& g = T.leg[s];
            const double pl = g.dir * (exitPx - g.entryFill) * usdPerPt * g.qty;
            ++T.runLegs; ++T.legsToday;
            if (pl > 0.0) { ++T.runWins; ++T.winsToday; T.consecLosses = 0; } else { ++T.lossesToday; ++T.consecLosses; }
            T.runPL += pl; T.legPLToday += pl;
            std::snprintf(msg, sizeof msg, "DSU %s LEG CLOSED (%s) %s signal #%d | %d contract(s) %.2f -> %.2f | P&L ~%.2f before costs "
                          "(%+.2f pts) | day: %d leg(s), %d win(s), %d loss(es), %d in a row, ~%.2f | run: %d legs, %d wins, ~%.2f",
                          kStrat[s], how, DirName(g.dir), g.sigSeq, g.qty, g.entryFill, exitPx, pl, g.dir * (exitPx - g.entryFill),
                          T.legsToday, T.winsToday, T.lossesToday, T.consecLosses, T.legPLToday, T.runLegs, T.runWins, T.runPL);
            sc.AddMessageToLog(msg, 0);
            std::memset(&g, 0, sizeof g);
        };

        auto Flatten = [&](int why)
        {
            const int r = sc.FlattenAndCancelAllOrders();
            T.flattenPending = 1; T.flattenAbs = nowAbs; ++T.flattenCount; T.flattenPx = lastPx; acted = true;
            if (why != FW_REISSUE) T.flattenWhy = (why > FW_NONE && why < FW_COUNT) ? why : FW_NONE;
            std::snprintf(msg, sizeof msg, "DSU FLATTEN (%s) -- FlattenAndCancelAllOrders()=%d; legs are closed once the broker reports flat.",
                          kFlatWhy[why > FW_NONE && why < FW_COUNT ? why : FW_NONE], r);
            sc.AddMessageToLog(msg, why == FW_EOD ? 0 : 1);
        };

        auto CancelEntry = [&](int s, const char* why)               // one leg's unfilled parent only, never another leg's orders
        {
            Leg& g = T.leg[s];
            const int r = sc.CancelOrder(static_cast<uint64_t>(g.parentID));
            g.canceling = 1; g.cancelAbs = nowAbs;
            if (why != g.cancelWhy) std::snprintf(g.cancelWhy, sizeof g.cancelWhy, "%s", why);   // a retry passes the stored text itself (#91)
            std::snprintf(msg, sizeof msg, "DSU %s ENTRY CANCEL signal #%d (%s) -- CancelOrder()=%d; a fill that wins the race is kept "
                          "with its attached stop.", kStrat[s], g.sigSeq, g.cancelWhy, r);
            sc.AddMessageToLog(msg, 0);
        };

        char skip[320]; skip[0] = 0;
        auto Submit = [&](int s) -> bool
        {
            const Strat& S = E.s[s];
            const int    d    = S.sigDir;
            const double dist = DMax(sc.RoundToTickSize(S.sigStopDist), 2.0 * tick);
            if (usdPerPt <= 0.0) { std::snprintf(skip, sizeof skip, "$ per point unknown (check the symbol settings)"); return false; }
            const double r1 = dist * usdPerPt;                          // $ risk of one contract at the stop
            int qty = P.Qty[s];
            if (O.SizeMode == 1)
            {
                qty = (int)std::floor(O.Account * O.RiskPct / 100.0 / r1 + 1e-9);
                if (qty > O.MaxQty) qty = O.MaxQty;
                if (qty < 1) { std::snprintf(skip, sizeof skip, "risk %.2f%% of $%.0f buys less than 1 contract at a %.2f pt stop ($%.2f)",
                                             O.RiskPct, O.Account, dist, r1); return false; }
            }
            if (O.MaxRiskUSD > 0.0)
            {
                while (qty > 1 && r1 * qty > O.MaxRiskUSD + 1e-9) --qty;
                if (r1 * qty > O.MaxRiskUSD + 1e-9)
                { std::snprintf(skip, sizeof skip, "risk $%.2f for 1 contract is over the $%.2f cap per leg", r1, O.MaxRiskUSD); return false; }
            }
            if (O.MaxOpenRisk > 0.0)
            {
                const double open = OpenRiskUSD();
                while (qty > 1 && open + r1 * qty > O.MaxOpenRisk + 1e-9) --qty;
                if (open + r1 * qty > O.MaxOpenRisk + 1e-9)
                { std::snprintf(skip, sizeof skip, "open legs already risk $%.2f; 1 more contract ($%.2f) would pass the $%.2f cap on open risk",
                                open, r1, O.MaxOpenRisk); return false; }
            }
            s_SCNewOrder o;                                                 // a fresh struct per leg
            o.OrderType              = SCT_ORDERTYPE_MARKET;
            o.OrderQuantity          = qty;
            o.TimeInForce            = SCT_TIF_DAY;
            o.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP;
            o.Stop1Offset            = dist;                                // from the actual fill, so slippage never changes the risk (#20)
            SCString tag;
            tag.Format("DSU %s #%d", kStrat[s], S.sigSeq);
            o.TextTag = tag;
            const int r = static_cast<int>(d > 0 ? sc.BuyEntry(o) : sc.SellEntry(o));
            if (r <= 0)
            {
                std::snprintf(skip, sizeof skip, "%s entry REJECTED (%d: %s) -- see Trade >> Trade Service Log", d > 0 ? "BUY" : "SELL", r,
                              sc.GetTradingErrorTextMessage(r));
                return false;
            }
            Leg& g = T.leg[s];
            std::memset(&g, 0, sizeof g);
            g.state = LG_PENDING; g.dir = d; g.sigSeq = S.sigSeq; g.qty = qty; g.dist = dist; g.entryRef = lastPx;
            g.parentID = o.InternalOrderID; g.stopID = o.Stop1InternalOrderID; g.submitAbs = nowAbs;
            if (g.stopID == 0)
                sc.AddMessageToLog("DSU: entry accepted but NO attached stop ID returned -- flattened if no stop works after the fill (#14).", 1);
            std::snprintf(msg, sizeof msg, "DSU %s ORDER %s signal #%d | MARKET x%d at ~%.2f | attached stop offset %.2f pts (stop ~%.2f) | "
                          "$risk %.2f | open risk incl. this leg %.2f", kStrat[s], DirName(d), S.sigSeq, qty, g.entryRef, dist,
                          g.entryRef - d * dist, r1 * qty, OpenRiskUSD());
            sc.AddMessageToLog(msg, 0);
            return true;
        };

        // ---------------------------------------------------------- banner
        if (!T.bannerDone)
        {
            T.bannerDone = 1;
            const bool sim = sc.GlobalTradeSimulationIsOn != 0;
            const bool off = O.OrderMode != 0;
            std::snprintf(msg, sizeof msg, "DSU ORDERS %s | Trade Simulation Mode %s -> SendOrdersToTradeService=%d%s",
                          off ? "OFF -- signals only, NO ORDERS ARE SENT" : "SEND", sim ? "ON" : "OFF", (int)sc.SendOrdersToTradeService,
                          off ? "" : (liveBlocked ? " | LIVE ENTRIES BLOCKED: Sim is OFF and 'Allow LIVE orders' = No"
                                                  : (!sim ? " | *** LIVE ORDERS ENABLED ***" : "")));
            sc.AddMessageToLog(msg, (off || liveBlocked || !sim) ? 1 : 0);
            HHMM(hm, sizeof hm, P.EOD);
            char legs[200]; legs[0] = 0;
            for (int s = 0; s < NS; ++s)
            {
                const size_t n = std::strlen(legs);
                std::snprintf(legs + n, sizeof legs - n, "%s%s %s x%d", n ? " | " : "", kStrat[s], P.On[s] ? "ON" : "off", LegCap(P, O, s));
            }
            char sz[160];
            if (O.SizeMode == 0) std::snprintf(sz, sizeof sz, "fixed contracts per leg");
            else std::snprintf(sz, sizeof sz, "%.2f%% of the $%.0f account input per leg, at most %d contract(s) per leg", O.RiskPct, O.Account, O.MaxQty);
            std::snprintf(msg, sizeof msg, "DSU plan: %s | size: %s | MaximumPositionAllowed %d | one account, one leg per strategy, each "
                          "with its own attached stop | max $ risk per leg %s | max open risk %s | daily loss halt %s | daily profit lock %s | "
                          "max losing legs in a row %s | flat time %s | %s $/pt %.2f%s",
                          legs, sz, TotalCap(P, O), O.MaxRiskUSD > 0.0 ? "ON" : "OFF", O.MaxOpenRisk > 0.0 ? "ON" : "OFF",
                          O.DailyLoss > 0.0 ? "ON" : "OFF", O.ProfitLock > 0.0 ? "ON" : "OFF", O.MaxConsec > 0 ? "ON" : "OFF", hm,
                          sc.Symbol.GetChars(), usdPerPt, usdPerPt > 0.0 ? "" : " *** $/pt UNKNOWN: no entries ***");
            sc.AddMessageToLog(msg, (usdPerPt > 0.0 && TotalCap(P, O) > 0) ? 0 : 1);
            if (O.Fallbacks + P.Fallbacks > 0)
            {
                std::snprintf(msg, sizeof msg, "DSU: %d input(s) out of range -> defaults used. Re-enter them in Study Settings (#15).",
                              O.Fallbacks + P.Fallbacks);
                sc.AddMessageToLog(msg, 1);
            }
        }
        if (liveBlocked && O.OrderMode == 0 && !T.liveBlockWarned)
        {
            T.liveBlockWarned = 1;
            sc.AddMessageToLog("DSU: Trade Simulation Mode is OFF and 'Allow LIVE orders' = No -> no new entries. Open legs are still managed.", 1);
        }

        // ---- chart checks, once after every recalculation (a settings change recalculates)
        if (!T.cfgDone)
        {
            T.cfgDone = 1; T.cfgFail = 0; T.tzState = 0; T.cfgWhy[0] = 0;
            char why[240], tzInfo[120];
            why[0] = 0; tzInfo[0] = 0;
            SCString tzName;
            auto AddWhy = [&](const char* w) { const size_t n = std::strlen(why); std::snprintf(why + n, sizeof why - n, "%s", w); };
            if (sc.ConvertDateTimeUTCToChartTimeZone != nullptr)             // two fixed UTC instants must read 12:00 in New York
            {
                const int        dJan = 46037, dJul = 46218;                 // 2026-01-15, 2026-07-15
                const SCDateTime a    = sc.ConvertDateTimeUTCToChartTimeZone(SCDateTime(dJan, 17 * 3600));
                const SCDateTime b    = sc.ConvertDateTimeUTCToChartTimeZone(SCDateTime(dJul, 16 * 3600));
                const bool       ok   = a.GetDate() == dJan && a.GetTime() == 12 * 3600 && b.GetDate() == dJul && b.GetTime() == 12 * 3600;
                T.tzState = ok ? 1 : 2;
                HHMM(hm, sizeof hm, a.GetTime()); HHMM(hm2, sizeof hm2, b.GetTime());
                std::snprintf(tzInfo, sizeof tzInfo, "UTC 17:00 on 15 Jan -> %s, 16:00 on 15 Jul -> %s", hm, hm2);
                if (!ok) { T.cfgFail |= 1; AddWhy("chart time zone is not New York (Chart Settings >> Session Times >> Time Zone); "); }
            }
            else std::snprintf(tzInfo, sizeof tzInfo, "conversion function unavailable -- not checked");
            if (sc.GetChartTimeZoneStringRef != nullptr) sc.GetChartTimeZoneStringRef(tzName, sc.ChartNumber);
            if (sc.SecondsPerBar != 300)
            {
                char w[80]; std::snprintf(w, sizeof w, "bars are %d s, measured on 300 s (5 minutes); ", sc.SecondsPerBar);
                T.cfgFail |= 2; AddWhy(w);
            }
            const char* sym = sc.Symbol.GetChars();
            auto pre = [&](const char* p) { return sym != nullptr && std::strncmp(sym, p, std::strlen(p)) == 0; };
            if (!(pre("MNQ") || pre("NQ"))) { T.cfgFail |= 4; AddWhy("symbol is not MNQ / NQ (the only market this was measured on); "); }
            if (E.sessions < P.Lookback + 1)
            {
                char w[120]; std::snprintf(w, sizeof w, "only %d day sessions loaded, %d needed (Chart Settings >> Days to Load); ", E.sessions, P.Lookback + 1);
                T.cfgFail |= 8; AddWhy(w);
            }
            std::snprintf(T.cfgWhy, sizeof T.cfgWhy, "%s", why);
            std::snprintf(msg, sizeof msg, "DSU CHART CHECK %s | time zone %s (%s; chart setting '%s') | bars %d s | symbol %s | day sessions "
                          "loaded %d | %s%s", T.cfgFail ? "FAILED" : "ok", T.tzState == 1 ? "New York" : (T.tzState == 2 ? "NOT New York" : "unknown"),
                          tzInfo, tzName.GetChars(), sc.SecondsPerBar, sym != nullptr ? sym : "?", E.sessions,
                          T.cfgFail ? (O.Safety == 0 ? "NO NEW ENTRIES: " : "warn only: ") : "", T.cfgWhy);
            sc.AddMessageToLog(msg, (T.cfgFail || T.tzState == 0) ? 1 : 0);
            std::snprintf(msg, sizeof msg, "DSU ENVIRONMENT | trade account '%s' | Auto Trading Enabled %d | trading locked %d | %s | data feed "
                          "state %d (2 = connected) | system time - last trade %d s | safety checks %s | untracked positions %s | max entry "
                          "delay %d s | CME early closes %s", sc.SelectedTradeAccount.GetChars(), (int)sc.IsAutoTradingEnabled,
                          (int)sc.TradingIsLocked, realTime ? "REAL TIME" : "Chart Replay / back test", (int)sc.ServerConnectionState, rawLag,
                          O.Safety == 0 ? "BLOCK" : "WARN ONLY", O.Inherited == 0 ? "flattened" : "left alone", O.EntryDelay,
                          O.EarlyClose == 0 ? "honoured" : "IGNORED");
            sc.AddMessageToLog(msg, (sc.IsAutoTradingEnabled == 0 || sc.TradingIsLocked != 0 || O.Safety != 0 || O.Inherited != 0 ||
                                     O.EntryDelay < 0 || O.EarlyClose != 0) ? 1 : 0);
        }
        const bool cfgBlock = O.Safety == 0 && T.cfgFail != 0;

        // ---- first live call after a recalculation: the engine's signal numbers restarted; legs keep their order IDs
        if (T.postRecalc)
        {
            T.postRecalc = 0;
            for (int s = 0; s < NS; ++s)                                 // history: already handled, or older than the bar that just closed (#89)
                if (E.s[s].sigKey <= T.sigKeyDone[s] || E.s[s].sigBar != nowBar - 1) { T.sigSeqDone[s] = E.s[s].sigSeq; T.sigKeyDone[s] = E.s[s].sigKey; }
            if (AnyLeg(LG_OPEN) || AnyLeg(LG_PENDING))
                sc.AddMessageToLog("DSU: open or working legs carried across a recalculation -- still managed by their order IDs.", 0);
        }
        if (posNet == 0 && !working) T.flatSeen = 1;

        // ---- a flatten in flight: finished when the broker is flat; re-issued after 30 s, then every 5 minutes (#69)
        if (T.flattenPending)
        {
            if (posNet == 0)
            {
                for (int s = 0; s < NS; ++s)
                {
                    if (T.leg[s].state == LG_OPEN) CloseLeg(s, T.flattenPx, kFlatWhy[T.flattenWhy]);
                    else if (T.leg[s].state == LG_PENDING) std::memset(&T.leg[s], 0, sizeof(Leg));
                }
                T.flattenPending = 0; T.flattenCount = 0; T.mismatch = 0;
            }
            else
            {
                const int64_t wait = T.flattenCount < 10 ? 30 : 300;
                if (nowAbs - T.flattenAbs >= wait)
                {
                    if (T.flattenCount >= 10)
                        sc.AddMessageToLog("DSU *** MANUAL ACTION NEEDED *** the position is still open after 10 flatten attempts -- "
                                           "close it in the Trade Window. Retrying every 5 minutes.", 1);
                    Flatten(FW_REISSUE);
                }
                acted = true;                                         // nothing else while a flatten is in flight
            }
        }

        // ---- legs, by their own orders (#80)
        if (!T.flattenPending)
            for (int s = 0; s < NS; ++s)
            {
                Leg& g = T.leg[s];
                if (g.state == LG_PENDING)
                {
                    s_SCTradeOrder po;
                    const bool have = ReadOrder(g.parentID, po);
                    if (have && po.OrderStatusCode == SCT_OSC_FILLED)
                    {
                        g.state = LG_OPEN; g.canceling = 0; g.fillAbs = nowAbs;
                        g.entryFill = po.AvgFillPrice > 0.0 ? (double)po.AvgFillPrice : g.entryRef;      // never a 0 price (#30)
                        std::snprintf(msg, sizeof msg, "DSU %s FILL %s signal #%d | %d contract(s) at %.2f (planned %.2f, slippage %+.2f pts)",
                                      kStrat[s], DirName(g.dir), g.sigSeq, g.qty, g.entryFill, g.entryRef, g.dir * (g.entryFill - g.entryRef));
                        sc.AddMessageToLog(msg, 0);
                    }
                    else if (!have || !IsWorkingOrderStatus(po.OrderStatusCode))
                    {
                        std::snprintf(msg, sizeof msg, "DSU %s ENTRY signal #%d not filled -- %s.", kStrat[s], g.sigSeq,
                                      g.canceling ? g.cancelWhy : "the order is no longer working (rejected or cancelled outside the study)");
                        sc.AddMessageToLog(msg, g.canceling ? 0 : 1);
                        std::memset(&g, 0, sizeof g);
                    }
                    else if (g.canceling) { if (nowAbs - g.cancelAbs >= 5) CancelEntry(s, g.cancelWhy); }
                    else if (nowAbs - g.submitAbs >= 60) CancelEntry(s, "market entry not filled after 60 s");
                    else if (!inHours)                   CancelEntry(s, "outside trading hours");
                }
                if (g.state == LG_OPEN)
                {
                    s_SCTradeOrder so;
                    const bool have = ReadOrder(g.stopID, so);
                    if (have && so.OrderStatusCode == SCT_OSC_FILLED)
                    {
                        CloseLeg(s, so.AvgFillPrice > 0.0 ? (double)so.AvgFillPrice : (double)so.Price1, "attached stop");
                        continue;
                    }
                    const bool stopWorks = have && IsWorkingOrderStatusIgnorePendingChildren(so.OrderStatusCode);
                    if (stopWorks && !g.fillVerified)                         // once per fill (#60)
                    {
                        g.fillVerified = 1;
                        const double want  = g.entryFill - g.dir * g.dist;
                        const bool   wrong = g.dir * (g.entryFill - so.Price1) <= 0.0;
                        std::snprintf(msg, sizeof msg, "DSU %s FILL VERIFY %s | fill %.2f | stop %.2f (planned %.2f, difference %+.2f) | stop "
                                      "quantity %.0f of %d", kStrat[s], wrong ? "*** WRONG SIDE ***" : "ok", g.entryFill, so.Price1, want,
                                      so.Price1 - want, (double)so.OrderQuantity, g.qty);
                        sc.AddMessageToLog(msg, wrong ? 1 : 0);
                        if (wrong && !acted) Flatten(FW_WRONG_SIDE);
                    }
                    const bool covered = stopWorks && (int)((double)so.OrderQuantity + 0.5) >= g.qty;
                    if (covered) g.covShort = 0;                               // 5 s of trade time before acting (#68)
                    else if (!g.covShort) { g.covShort = 1; g.covShortAbs = nowAbs; }
                    else if (nowAbs - g.covShortAbs >= 5 && !acted)
                    {
                        std::snprintf(msg, sizeof msg, "DSU %s: the leg's stop has not been working for 5 s -- flattening everything.", kStrat[s]);
                        sc.AddMessageToLog(msg, 1);
                        Flatten(FW_UNPROTECTED);
                    }
                }
            }

        // ---- the account position must equal the open legs (L2); also catches positions the study did not open
        const int expNet = ExpectedNet();
        if (!T.flattenPending && !acted)
        {
            if (posNet == expNet) T.mismatch = 0;
            else if (expNet == 0 && !AnyLeg(LG_PENDING) && !T.flatSeen)        // already there when the study started
            {
                if (!T.inheritedWarned)
                {
                    T.inheritedWarned = 1;
                    std::snprintf(msg, sizeof msg, "DSU: the account already holds %+d contract(s) this study did not open -- no new "
                                  "entries; %s.", posNet, O.Inherited == 0 ? "flattened at the flat time or at once outside trading hours"
                                                                           : "left alone (input)");
                    sc.AddMessageToLog(msg, 1);
                }
                if (O.Inherited == 0 && !inHours) Flatten(FW_INHERITED);
            }
            else if (!T.mismatch) { T.mismatch = 1; T.mismatchAbs = nowAbs; }
            else if (nowAbs - T.mismatchAbs >= 5)
            {
                std::snprintf(msg, sizeof msg, "DSU: account position %+d but the open legs add up to %+d for 5 s -- flattening everything "
                              "(another study or a manual trade on this account? lessons #72).", posNet, expNet);
                sc.AddMessageToLog(msg, 1);
                Flatten(FW_MISMATCH);
            }
            if (posNet == 0 && expNet == 0 && !AnyLeg(LG_PENDING) && working && !acted && nowAbs - T.staleCancelAbs >= 5)
            {
                T.staleCancelAbs = nowAbs;
                sc.CancelAllOrders(); acted = true;                           // working orders can outlive the state (#41)
                sc.AddMessageToLog("DSU: working orders found with no leg open or pending -- cancelled.", 1);
            }
        }

        // ---- trading-day rollover: the account P&L baseline, counters and halts (#43)
        if (nowTD != T.lastDay)
        {
            if (T.lastDay != 0 && (T.sigToday > 0 || T.legsToday > 0))
            {
                std::snprintf(msg, sizeof msg, "DSU DAY SUMMARY | signals %d, traded %d | legs closed %d (%d wins) | leg P&L ~%.2f | run ~%.2f",
                              T.sigToday, T.sigTradedToday, T.legsToday, T.winsToday, T.legPLToday, T.runPL);
                sc.AddMessageToLog(msg, 0);
            }
            T.lastDay = nowTD; T.legsToday = T.winsToday = T.lossesToday = T.consecLosses = T.sigToday = T.sigTradedToday = 0;
            T.legPLToday = 0.0; T.haltLogged = 0;
            T.baseCumPL = pos.CumulativeProfitLoss; T.baseTrades = pos.TotalTrades;
            HHMM(hm, sizeof hm, effEOD);
            std::snprintf(msg, sizeof msg, "DSU STATUS | session %s | flat time %s | averages %s (avg range %.2f, prior close %.2f, RSI %.1f) | "
                          "account position %+d | account realised P&L baseline %.2f", haltTod > 0 ? "SHORT (CME halt) -- no entries" : "normal",
                          hm, E.ringN >= P.Lookback ? "ready" : "NOT ready", E.adr, E.prevClose, E.rsi, posNet, T.baseCumPL);
            sc.AddMessageToLog(msg, 0);
        }
        if (pos.TotalTrades < T.baseTrades)                                   // Sierra's statistics were reset during the day
        {
            T.baseCumPL = pos.CumulativeProfitLoss - T.legPLToday; T.baseTrades = pos.TotalTrades;
            sc.AddMessageToLog("DSU: trade statistics were reset during the day -- the day's P&L baseline was rebuilt from the leg log.", 1);
        }

        // ---- risk halts on the ACCOUNT's P&L (a limit of 0 is OFF: #36)
        const double realised   = pos.CumulativeProfitLoss - T.baseCumPL;
        const double dayPLNow   = realised + OpenPL();
        const bool   haltLoss   = O.DailyLoss > 0.0 && dayPLNow <= -O.DailyLoss;
        const bool   haltProfit = O.ProfitLock > 0.0 && realised >= O.ProfitLock;
        const bool   haltStreak = O.MaxConsec > 0 && T.consecLosses >= O.MaxConsec;
        if ((haltLoss || haltProfit || haltStreak) && !T.haltLogged)
        {
            T.haltLogged = 1;
            std::snprintf(msg, sizeof msg, "DSU RISK HALT for the rest of the day: %s | account day P&L %.2f (realised %.2f, open ~%.2f), "
                          "losing legs in a row %d. Resets on the next trading day.",
                          haltLoss ? "daily loss limit" : (haltProfit ? "daily profit lock" : "losing streak"), dayPLNow, realised, OpenPL(),
                          T.consecLosses);
            sc.AddMessageToLog(msg, 1);
        }

        // ---- exits the study decides: the flat time and the daily loss halt (the legs have no engine exits)
        if (!T.flattenPending && !acted && AnyLeg(LG_OPEN))
        {
            if (!inHours) Flatten(FW_EOD);
            else if (haltLoss) Flatten(FW_DAILY_LOSS);
        }

        // ---- new engine signals: trade them or say why not
        for (int s = 0; s < NS; ++s)
        {
            const Strat& S = E.s[s];
            if (!engOK || S.sigSeq <= 0 || S.sigKey <= T.sigKeyDone[s]) continue;      // keyed by time: survives recalculations
            T.sigSeqDone[s] = S.sigSeq; T.sigKeyDone[s] = S.sigKey;
            if (!P.On[s]) continue;                                           // switched off: tally only, no log line
            ++T.sigToday;
            const int64_t due = S.sigKey + bsec;                             // the signal bar's close
            const char*   why = nullptr;
            char          wbuf[160];
            bool opposite = false;
            for (int q = 0; q < NS; ++q) if (T.leg[q].state != LG_FLAT && T.leg[q].dir != S.sigDir) opposite = true;
            if (O.OrderMode != 0)                         why = "Orders = signals only";
            else if (S.sigBar != nowBar - 1)              why = "the signal bar is not the bar that just closed (late call)";
            else if (O.EntryDelay > 0 && nowAbs - due > O.EntryDelay)
            { std::snprintf(wbuf, sizeof wbuf, "%d s after the signal bar closed (max %d s)", (int)(nowAbs - due), O.EntryDelay); why = wbuf; }
            else if (!inHours)                            why = "outside trading hours / past the flat time";
            else if (haltTod > 0)                         why = "CME short session";
            else if (T.leg[s].state != LG_FLAT)           why = "this strategy's leg is already open or working";
            else if (opposite)                            why = "opposite to a leg that is open or working (one net position, L3)";
            else if (T.flattenPending || acted)           why = "a flatten or cancel is in flight";
            else if (posNet != expNet)                    why = "the account position does not match the open legs";
            else if (liveBlocked)                         why = "live trading not allowed (input)";
            else if (cfgBlock)                            why = "CHART CHECK failed";
            else if (feedBlock)                           why = "data feed down or delayed";
            else if (haltLoss)                            why = "daily loss halt";
            else if (haltProfit)                          why = "daily profit lock";
            else if (haltStreak)                          why = "losing-streak halt";
            if (why == nullptr && !Submit(s)) why = skip;
            if (why == nullptr) ++T.sigTradedToday;
            std::snprintf(msg, sizeof msg, "DSU %s engine SIGNAL #%d %s -> %s%s", kStrat[s], S.sigSeq, DirName(S.sigDir),
                          why == nullptr ? "TRADED" : "NOT traded -- ", why == nullptr ? "" : why);
            sc.AddMessageToLog(msg, (why != nullptr && O.OrderMode == 0) ? 1 : 0);
        }
    }
}   // namespace dsu

SCSFExport scsf_DiversifiedSuite_NQ_MNQ(SCStudyInterfaceRef sc)
{
    using namespace dsu;

    if (sc.SetDefaults)
    {
        sc.GraphName        = "Diversified Suite NQ MNQ " STUDY_VERSION;
        sc.StudyDescription = "Four MNQ day-session strategies in one study on one trade account: daily washout rebound (DWR), value-area re-entry (VAR), "
                              "compression-expansion breakout (CEB), end-of-day flow (EDF). One leg each with its own attached stop; all flat at 15:55 "
                              "New York. MNQ 5-minute chart with the overnight session loaded, New York time. Replay-testable tick by tick.";
        sc.AutoLoop         = 1;
        sc.GraphRegion      = 0;
        sc.ValueFormat      = VALUEFORMAT_INHERITED;
        sc.DrawZeros        = 0;

        auto Style = [&](int idx, const char* name, int style, COLORREF col, int width)
        {
            SCSubgraphRef s = sc.Subgraph[idx];
            s.Name = name; s.DrawStyle = style; s.PrimaryColor = col; s.LineWidth = width; s.DrawZeros = 0;
        };
        Style(SG_VAH,    "VAR prior value area high",                   DRAWSTYLE_DASH,   RGB(63, 185, 80),   1);
        Style(SG_VAL,    "VAR prior value area low",                    DRAWSTYLE_DASH,   RGB(248, 81, 73),   1);
        Style(SG_POC,    "VAR prior point of control",                  DRAWSTYLE_DASH,   RGB(200, 140, 40),  1);
        Style(SG_ORH,    "CEB opening range high (narrow-range days)",  DRAWSTYLE_DASH,   RGB(88, 166, 255),  2);
        Style(SG_ORL,    "CEB opening range low (narrow-range days)",   DRAWSTYLE_DASH,   RGB(88, 166, 255),  2);
        Style(SG_EDF_UP, "EDF prior close + min move",                  DRAWSTYLE_IGNORE, RGB(150, 150, 150), 1);
        Style(SG_EDF_DN, "EDF prior close - min move",                  DRAWSTYLE_IGNORE, RGB(150, 150, 150), 1);
        Style(SG_RSI,    "DWR RSI of day-session closes (stored)",      DRAWSTYLE_IGNORE, RGB(200, 200, 200), 1);
        Style(SG_NR,     "CEB narrow-range count (stored)",             DRAWSTYLE_IGNORE, RGB(200, 200, 200), 1);
        static const COLORREF kCol[NS] = { RGB(63, 185, 80), RGB(200, 140, 40), RGB(88, 166, 255), RGB(255, 105, 180) };
        for (int q = 0; q < NS; ++q)
        {
            SCString nm;
            nm.Format("%s long signal", kStrat[q]);        Style(SG_LONG0 + q,  nm.GetChars(), DRAWSTYLE_ARROW_UP,   kCol[q], 3);
            nm.Format("%s short signal", kStrat[q]);       Style(SG_SHORT0 + q, nm.GetChars(), DRAWSTYLE_ARROW_DOWN, kCol[q], 3);
            nm.Format("Order: %s leg stop", kStrat[q]);    Style(SG_STOP0 + q,  nm.GetChars(), DRAWSTYLE_DASH,       kCol[q], 2);
        }
        sc.MaintainTradeStatisticsAndTradesData = 1;

        const Params D;
        auto DefYN = [&](int idx, const char* n, int v) { sc.Input[idx].Name = n; sc.Input[idx].SetYesNo(v); };
        auto DefTM = [&](int idx, const char* n, int v) { sc.Input[idx].Name = n; sc.Input[idx].SetTime(v); };
        auto DefIN = [&](int idx, const char* n, int v, int lo, int hi)
        {
            sc.Input[idx].Name = n; sc.Input[idx].SetInt(v); sc.Input[idx].SetIntLimits(lo, hi);
        };
        auto DefFL = [&](int idx, const char* n, double v, double lo, double hi)
        {
            sc.Input[idx].Name = n; sc.Input[idx].SetFloat((float)v); sc.Input[idx].SetFloatLimits((float)lo, (float)hi);
        };
        auto DefCX = [&](int idx, const char* n, const char* items, int v)
        {
            sc.Input[idx].Name = n; sc.Input[idx].SetCustomInputStrings(items); sc.Input[idx].SetCustomInputIndex(v);
        };
        DefYN(IN_DWR_ON,       "DWR daily washout rebound: trade it", D.On[S_DWR]);
        DefIN(IN_DWR_QTY,      "DWR contracts (fixed size)", D.Qty[S_DWR], 1, 100);
        DefIN(IN_DWR_RSI_LEN,  "DWR RSI length, day-session closes (measured 2)", D.RsiLen, 2, 14);
        DefFL(IN_DWR_RSI_BUY,  "DWR buy the open when RSI is below (measured 30; positive 20-40)", D.RsiBuy, 1.0, 99.0);
        DefFL(IN_DWR_RSI_SELL, "DWR sell the open when RSI is above (0 = off; measured OFF)", D.RsiSell, 0.0, 99.0);
        DefFL(IN_DWR_STOP,     "DWR attached stop, x average day range (measured 0.75)", D.DwrStop, 0.05, 5.0);
        DefYN(IN_VAR_ON,       "VAR value-area re-entry: trade it", D.On[S_VAR]);
        DefIN(IN_VAR_QTY,      "VAR contracts (fixed size)", D.Qty[S_VAR], 1, 100);
        DefFL(IN_VAR_FRAC,     "VAR value area, share of the prior session volume (measured 0.70)", D.VaFrac, 0.30, 0.95);
        DefIN(IN_VAR_BIN,      "VAR volume profile bin, ticks (measured 4 = 1 point)", D.VaBinTicks, 1, 400);
        DefIN(IN_VAR_MARK,     "VAR decision marks every, minutes from the open (measured 30)", D.VarMark, 5, 120);
        DefTM(IN_VAR_FIRST,    "VAR first decision at (bar close, New York; measured 10:00)", D.VarFirst);
        DefTM(IN_VAR_LAST,     "VAR last decision that may open a trade (measured 12:30)", D.VarLast);
        DefFL(IN_VAR_STOP,     "VAR attached stop, x average day range (measured 0.20)", D.VarStop, 0.02, 5.0);
        DefYN(IN_CEB_ON,       "CEB compression-expansion breakout: trade it", D.On[S_CEB]);
        DefIN(IN_CEB_QTY,      "CEB contracts (fixed size)", D.Qty[S_CEB], 1, 100);
        DefIN(IN_CEB_NR,       "CEB narrow-range test: prior range the smallest of, sessions (measured 4)", D.NrDays, 1, 20);
        DefIN(IN_CEB_OR,       "CEB opening range, minutes (measured 30)", D.OrMinutes, 5, 180);
        DefTM(IN_CEB_LAST,     "CEB last bar close that may open a trade (measured 13:00)", D.CebLast);
        DefFL(IN_CEB_STOP,     "CEB attached stop, x average day range (measured 0.25)", D.CebStop, 0.02, 5.0);
        DefYN(IN_EDF_ON,       "EDF end-of-day flow: trade it", D.On[S_EDF]);
        DefIN(IN_EDF_QTY,      "EDF contracts (fixed size)", D.Qty[S_EDF], 1, 100);
        DefFL(IN_EDF_MOVE,     "EDF min move from the prior close at the decision, % (measured 1.0)", D.EdfMove, 0.05, 20.0);
        DefTM(IN_EDF_MARK,     "EDF decision at (bar close, New York; measured 15:00)", D.EdfMark);
        DefFL(IN_EDF_STOP,     "EDF attached stop, x average day range (measured 0.40)", D.EdfStop, 0.02, 5.0);
        DefTM(IN_SESS_OPEN,    "Day session opens (New York)", D.SessOpen);
        DefTM(IN_SESS_CLOSE,   "Day session closes (New York)", D.SessClose);
        DefTM(IN_EOD,          "Flat time: close everything (New York; measured 15:55)", D.EOD);
        DefCX(IN_DIRECTIONS,   "Directions allowed", "Long and short;Long only;Short only", D.Directions);
        DefIN(IN_LOOKBACK,     "Sessions in the average day range (measured 14)", D.Lookback, 3, MAXLOOK);
        DefFL(IN_TALLY_COST,   "Tally cost per contract per round trip, points (diagnostic only)", D.TallyCost, 0.0, 50.0);
        DefYN(IN_VERBOSE,      "Verbose log", D.Verbose);
        const OParams DO;
        DefCX(IN_ORDER_MODE,   "Orders", "Send orders (Trade Simulation; live needs the next input);Signals only -- no orders", DO.OrderMode);
        DefYN(IN_ALLOW_LIVE,   "Allow LIVE orders (when Trade Simulation Mode is OFF)", DO.AllowLive);
        DefCX(IN_SIZE_MODE,    "Position size per leg", "Fixed contracts (each strategy's input);Risk % of the account size input", DO.SizeMode);
        DefFL(IN_ACCOUNT,      "Account size, $ (for Risk % sizing; typed here, not read from the broker)", DO.Account, 100.0, 1.0e9);
        DefFL(IN_RISK_PCT,     "Risk per leg, % of the account size (Risk % sizing)", DO.RiskPct, 0.01, 20.0);
        DefIN(IN_MAX_QTY,      "Max contracts per leg (Risk % sizing)", DO.MaxQty, 1, 100);
        DefFL(IN_MAX_RISK_USD, "Max $ risk per leg (0 = off; reduces contracts, skips if 1 is over)", DO.MaxRiskUSD, 0.0, 1.0e7);
        DefFL(IN_MAX_OPEN_RISK, "Max $ risk of all open legs together, at their stops (0 = off; reduces, then skips)", DO.MaxOpenRisk, 0.0, 1.0e7);
        DefFL(IN_DAILY_LOSS,   "Daily loss halt, $ on the account incl. open legs (0 = off; wider than one full stop)", DO.DailyLoss, 0.0, 1.0e7);
        DefFL(IN_PROFIT_LOCK,  "Daily profit lock, $ realised: no new entries after (0 = off)", DO.ProfitLock, 0.0, 1.0e7);
        DefIN(IN_MAX_CONSEC,   "Max losing legs in a row per day (0 = off)", DO.MaxConsec, 0, 50);
        DefCX(IN_SAFETY,       "Safety checks (chart: New York time, 5-min bars, MNQ; feed: connected, real time)",
              "Block new entries when a check fails (recommended);Warn only", DO.Safety);
        DefCX(IN_INHERITED,    "Position already on the account when the study starts",
              "Flatten at the flat time, or at once outside trading hours (recommended);Leave alone", DO.Inherited);
        DefIN(IN_ENTRY_DELAY,  "Max entry delay after the signal bar closes, seconds (0 = 120 default, -1 = off)", 0, -1, 3600);
        DefCX(IN_EARLY_CLOSE,  "CME holiday halts / early closes (13:00 / 13:15 New York)",
              "No entries; flatten before the halt (recommended);Ignore", DO.EarlyClose);
        return;
    }

    void*& enginePtr = sc.GetPersistentPointer(1);
    void*& traderPtr = sc.GetPersistentPointer(2);
    if (sc.LastCallToFunction)
    {
        if (enginePtr != nullptr) { delete static_cast<Engine*>(enginePtr); enginePtr = nullptr; }
        if (traderPtr != nullptr) { delete static_cast<Trader*>(traderPtr); traderPtr = nullptr; }
        return;
    }
    if (enginePtr == nullptr)
    {
        enginePtr = new Engine;
        static_cast<Engine*>(enginePtr)->ResetAll();
    }
    Engine& E = *static_cast<Engine*>(enginePtr);
    if (traderPtr == nullptr || static_cast<Trader*>(traderPtr)->magic != kTraderMagic)
    {
        if (traderPtr == nullptr) traderPtr = new Trader;
        static_cast<Trader*>(traderPtr)->Init();
    }
    Trader& T = *static_cast<Trader*>(traderPtr);

    const Params  P = ReadParams(sc);
    const OParams O = ReadOParams(sc);
    const int     i = sc.Index;

    // Trade-behaviour settings on EVERY call (lessons #15/#16). Several legs share the position (#80).
    sc.SendOrdersToTradeService                       = !sc.GlobalTradeSimulationIsOn;  // read, never assigned
    sc.MaximumPositionAllowed                         = IMax(TotalCap(P, O), 1);         // lessons #8
    sc.SupportReversals                               = 0;
    sc.AllowOppositeEntryWithOpposingPositionOrOrders = 0;   // L3 is also Sierra's own rule
    sc.SupportAttachedOrdersForTrading                = 1;
    sc.UseGUIAttachedOrderSetting                     = 0;
    sc.CancelAllOrdersOnEntriesAndReversals           = 0;   // lessons #2: a new leg must not cancel another leg's stop
    sc.CancelAllOrdersOnEntries                       = 0;
    sc.CancelAllOrdersOnReversals                     = 0;
    sc.AllowEntryWithWorkingOrders                    = 1;   // other legs' stops are working orders
    sc.AllowOnlyOneTradePerBar                        = 0;   // two strategies may enter on one bar
    sc.AllowMultipleEntriesInSameDirection            = 1;
    sc.CancelAllWorkingOrdersOnExit                   = 0;
    sc.SupportTradingScaleIn                          = 0;   // each leg carries its own bracket
    sc.SupportTradingScaleOut                         = 0;
    sc.MaintainTradeStatisticsAndTradesData           = 1;

    if (sc.IsFullRecalculation && i == 0 && !E.recalcOpen) { E.ResetAll(); E.recalcOpen = 1; }   // lessons #37
    if (!E.bannerDone)
    {
        E.bannerDone = 1;
        char msg[400];
        std::snprintf(msg, sizeof msg, "DSU Diversified Suite NQ MNQ " STUDY_VERSION " | built " STUDY_BUILD_STAMP " | chart #%d %s | %d s "
                      "bars | inputs out of range: %d", sc.ChartNumber, sc.Symbol.GetChars(), sc.SecondsPerBar, P.Fallbacks + O.Fallbacks);
        sc.AddMessageToLog(msg, 0);
    }
    if (sc.TickSize <= 0.0f)
    {
        if (!E.tickWarned) { E.tickWarned = 1; sc.AddMessageToLog("DSU: sc.TickSize is 0 -- engine idle. Check the symbol settings.", 1); }
        return;
    }
    E.usdPerPt   = sc.CurrencyValuePerTick > 0.0f ? (double)sc.CurrencyValuePerTick / sc.TickSize : 0.0;
    E.virtualLog = O.OrderMode != 0 ? 1 : 0;      // live virtual-trade lines only when no orders are sent (or Verbose)

    ScFeed f(sc);
    // Commit every closed bar exactly once, in order; a repeated call for a committed bar changes nothing (lessons #26/#27).
    const int lastClosed = sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_CLOSED ? i : i - 1;
    if (lastClosed > E.lastCommitted)
        for (int k = IMax(E.lastCommitted + 1, 0); k <= lastClosed; ++k)
        {
            E.quiet = (sc.IsFullRecalculation || k < sc.ArraySize - 2) ? 1 : 0;   // lessons #63: per bar, not per call
            ProcessBar(f, E, P, k);
            E.lastCommitted = k;
        }
    if (lastClosed < i)                                     // the forming bar: carry the lines, no decisions
        for (int g = 0; g <= SG_EDF_DN; ++g) sc.Subgraph[g][i] = i > 0 ? sc.Subgraph[g][i - 1] : 0.0f;

    if (!E.tallyLogged && E.lastCommitted >= sc.ArraySize - 2) { E.tallyLogged = 1; LogTally(f, E, P); }
    if (!sc.IsFullRecalculation || i == sc.ArraySize - 1) E.recalcOpen = 0;

    OrderLayer(sc, E, P, O, T);

    for (int s = 0; s < NS; ++s)                             // each leg's stop: the working order's price, else the planned one
    {
        const Leg& g = T.leg[s];
        float stop = 0.0f;
        if (!sc.IsFullRecalculation && g.state != LG_FLAT)
        {
            s_SCTradeOrder o;
            if (g.state == LG_OPEN && g.stopID > 0 && sc.GetOrderByOrderID(static_cast<uint64_t>(g.stopID), o) != SCTRADING_ORDER_ERROR &&
                IsWorkingOrderStatus(o.OrderStatusCode)) stop = (float)o.Price1;
            else stop = (float)((g.entryFill > 0.0 ? g.entryFill : g.entryRef) - g.dir * g.dist);
        }
        sc.Subgraph[SG_STOP0 + s][i] = stop;
    }
}
#endif
