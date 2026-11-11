#include "sierrachart.h"

SCDLLName("NoiseBandMomentum_NQ_MNQ")

/*============================================================================
  NoiseBandMomentum_NQ_MNQ  v3  --  engine + orders + hardening
  (Lessons #12: the two lines above stay at the very top of the file.)

  WHAT IT DOES
    Intraday momentum on MNQ, day session only. A "noise band" is drawn around
    the day's open: for each time of day, the average distance the price has
    been from the open at that time over the last 14 full sessions. A
    30-minute close outside the band AND on the same side of session VWAP is
    a move larger than the usual noise -> trade with it. The trade is held
    while 30-minute closes stay beyond max(band, VWAP) and is flat by 15:55.

  SOURCE OF THE IDEA
    Zarattini, Aziz, Barbon (2024), "Beat the Market: An Effective Intraday
    Momentum Strategy for S&P500 ETF (SPY)" (noise area, VWAP trailing stop);
    Gao, Han, Li, Zhou (2018), "Market intraday momentum", J. Financial
    Economics. Kaufman, Trading Systems and Methods 5th ed., ch.16
    (volatility-based intraday breakouts) and ch.15 (time-of-day patterns).
    Published claims; the numbers below are my own test on NQ.

  RULES (long; short is the mirror)
    R1 Session = 09:30-16:00 New York. open = first trade of the session;
       prior close = last day-session close. Holiday-shortened days: no trade.
    R2 move(t)  = average over the last 14 full sessions of |close(t)/open-1|
       upper(t) = max(open, prior close) x (1 + 1.0 x move(t))
       lower(t) = min(open, prior close) x (1 - 1.0 x move(t))
    R3 VWAP = session volume-weighted average of (H+L+C)/3 of the bars.
    R4 Decisions only at 30-minute marks, 10:00 .. 15:30:
         flat -> LONG  if close > upper and close > VWAP  (market, next open)
         long -> EXIT  if close < max(upper, VWAP)        (market, next open)
       An exit and a new entry never share a mark. At most 3 trades a day.
    R5 Attached disaster stop 0.5 x the 14-session average day range (hit on
       4% of trades; it bounds the loss between two 30-minute decisions).
    R6 Flat at 15:55.

  MEASURED (this file in the mock, NQ 1-minute data 2017-01 .. 2024-03, 1 MNQ,
  $2.50 per round trip = commission + 1 tick slippage per side)
    1,528 trades | 39.7% winners | profit factor 1.41 | net +$20,828 |
    $239 per month (median $147) | 55 of 87 months positive | best month
    +$1,226, worst -$451 | max drawdown -$1,189 | longest losing streak 16 |
    3 best trades = 11% of the net | long +$13,865, short +$6,963.
    By year: 2017 +98, 2018 +4,012, 2019 -169, 2020 +3,013, 2021 +3,907,
    2022 +4,300, 2023 +4,527, 2024 (to March) +1,141.
    Chosen on 2017-2021 ($181/month), checked on 2022-2024 ($369/month).
    Every neighbour tested stayed positive in both periods: band 0.8-1.5,
    with/without VWAP, stop 0.3-0.75 or none, 1-9 trades a day. The same
    rules on ES were also profitable (PF 1.23). Doubling the cost leaves
    about $203 per month. Quiet trend-less years (2017, 2019) made nothing.
    NOT TESTED: anything after 2024-03-07 -- no data here. Test it in Replay.

  ORDER LAYER (identical in the three scripts of this suite)
    O1  One trade at a time, one order: quantity N, MaximumPositionAllowed = N.
    O2  The entry carries its own ATTACHED stop (Stop1Offset = a distance from
        the actual fill, so slippage never changes the risk). No target.
    O3  Entry on the first live call after the signal bar closes, at most 120 s
        late (input). Every engine signal is logged "TRADED" or
        "NOT traded -- <reason>".
    O4  Size: fixed contracts, or risk % of the ACCOUNT SIZE INPUT (never the
        broker balance, lessons #5) / the stop distance, capped by Max
        contracts; optional max $ risk per trade (reduces, then skips).
    O5  Risk halts, each OFF at 0 (#36): daily loss incl. the open trade
        (flattens), daily profit lock, losing trades in a row, trades per day.
        Reset only on a new trading day (#43).
    O6  Flat time 15:55 New York: resting entries cancelled, position closed
        with FlattenAndCancelAllOrders(); re-issued after 30 s, MANUAL ACTION
        line after 10 tries (#69). CME holiday / early-close sessions: no
        entries, flat the same distance before the halt (#66).
    O7  Reconciliation on every call from ONE position read (#17): closed
        trades classified from the CumulativeProfitLoss / TotalTrades delta
        (#34) before the daily rollover (#35); FILL VERIFY once per fill
        (#60); every contract covered by a working stop or flatten after 5 s
        (#68); orphan and inherited positions (#38, #70); working orders
        while flat are cancelled (#41).
    O8  Full recalculation: trade state survives, the engine is rebuilt, the
        first live call re-adopts or discards (#37, #38, #63).
    O9  Trade-behaviour settings are written on every call (#15, #16).
    O10 Live orders need 'Allow LIVE orders' = Yes. CHART CHECK (New York time
        zone, 5-minute bars, MNQ/NQ symbol, enough sessions) and the feed
        checks (connected; not delayed for live, #65) block entries.

  THREE BUILD STEPS IN ONE FILE
    Step 1 (engine): 'Orders' = Signals only -> signals, lines and the TALLY,
    no orders. Step 2 (orders) and Step 3 (hardening): 'Orders' = Send orders.

  TEST RECORD (2026-11-10)
    * MinGW-w64 g++ 13, -std=c++17 -Wall -Wextra, public SDK headers (2414)
      plus the two time-zone members your SDK 2955 has: 0 errors, 0 warnings;
      the 64-bit DLL links and exports the study function. NOT yet built in
      your Sierra (lessons #59): the remote build is the authority.
    * This file run unchanged in a behavioural mock of the ACSIL calls it uses
      with a simulated broker: 726,315 one-minute NQ bars (2016-11 .. 2024-03)
      replayed as 4 ticks each on 5-minute chart bars, 3.0 million study
      calls, AddressSanitizer + UBSan clean. Trades match an independent
      Python model of the rules (see README: every difference has a cause).
    * Identical trades with 361 full recalculations injected, with the study
      called once a minute only, and with every call doubled.
    * Invariants on every tick: position <= MaximumPositionAllowed, flat
      outside 09:30 .. 15:56, nothing working at the end -- 0 violations.
    * Faults injected: wrong time zone (entries blocked), 12 failed flattens
      (re-issued every 30 s), a position the study did not open (flattened),
      $ caps, risk-% sizing, daily loss halt, losing-streak halt, profit lock.
    A mock tests the code against my reading of Sierra, not against Sierra.

  FIRST REPLAY CHECKS (unverified until seen in Sierra)
    1 A MARKET parent with Stop1Offset creates the stop at fill -/+ offset
      ("FILL VERIFY ok", difference 0.00).
    2 One "engine SIGNAL #n ... TRADED / NOT traded" line per SIGNAL line.
    3 Number of Open Trades = 0 at the end; every exit before 15:56.
    4 The TALLY line after the chart loads is close to the Replay result.
  BACKTEST IN SIERRA
    MNQ continuous contract (volume rollover, back-adjusted), 5-minute bars,
    New York time zone, at least 25 days loaded before the start date. Trade
    Simulation Mode ON, commission set (#55). Clear trade data first (#53,
    #71). Chart >> Replay Chart, Accurate Trading System Back Test mode
    (#54). One strategy per Trade Account (#72): give each script of this
    suite its own Sim account (Sim1, Sim2, Sim3).

  STORAGE: Persistent pointer 1 = Engine (heap, rebuilt on every full
    recalculation); pointer 2 = Trader (heap, kept). Both freed on
    LastCallToFunction. Closed bars are committed exactly once, in order.
  INSTALL: copy to ACS_Source (no spaces in the name), Build Custom Studies
    DLL, add the study to an MNQ 5-minute chart, New York time zone. Keep the
    Trade Window "Attached Orders" list EMPTY. Message Log lines start "NBM".
============================================================================*/

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>

#define STUDY_VERSION      "v3"
#define STUDY_BUILD_STAMP  __DATE__ " " __TIME__

namespace nbm
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
        IN_BAND_MULT = 0, IN_USE_VWAP, IN_FIRST_MARK, IN_LAST_ENTRY, IN_CAT_STOP,
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
        double BandMult   = 1.0;                       // x the average |move from the open| at that time of day
        int    UseVwap    = 1;
        int    FirstMark  = 10 * 3600;                 // first 30-minute decision (bar close time)
        int    LastEntry  = 15 * 3600 + 30 * 60;       // last decision that may open a trade
        double CatStop    = 0.5;                       // attached disaster stop, x average day range
        int    ChaseTicks = 0;
        int    SessOpen   = 9 * 3600 + 30 * 60;        // day session, New York
        int    SessClose  = 16 * 3600;
        int    EOD        = 15 * 3600 + 55 * 60;       // flat time (measured 15:55)
        int    Directions = DIR_BOTH;
        int    Lookback   = 14;                        // full sessions in the range / noise averages
        int    MaxTrades  = 3;               // per day, 0 = off
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
            std::snprintf(msg, sizeof msg, "NBM virtual #%d %s exit %s at %.2f (%s) -> %+.2f pts after %.2f cost | entry %.2f",
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
                      "NBM TALLY %s (diagnostic, not P&L: this chart's bars, one virtual 1-contract trade at a time, stop first on a "
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
            std::snprintf(msg, sizeof msg, "NBM SIGNAL #%d %s %s | %s | close %.2f | VWAP %.2f | open %.2f, prior close %.2f | avg range "
                          "%.2f | stop distance %.2f pts", E.sigSeq, DirName(d), ts, what, f.C(k), E.vwap, E.dOpen, E.prevClose, E.adr, dist);
            f.Log(msg, 0);
        }
    }

    // ---------------------------------------------------------------- decision
    // Noise-band momentum. At every 30-minute mark from 'First decision' on:
    //   exit   long  when the close is below max(upper band, VWAP); short mirrored;
    //   enter  long  when flat and the close is above the upper band and above VWAP; short mirrored.
    // An exit and a new entry never share a mark.
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec, double ub, double lb)
    {
        (void)b;
        const int endT = tod + bsec;
        if ((endT - P.SessOpen) % 1800 != 0 || endT < P.FirstMark) return;
        const double c   = f.C(k), vw = E.vwap;
        const double upT = P.UseVwap ? DMax(ub, vw) : ub;
        const double dnT = P.UseVwap ? DMin(lb, vw) : lb;
        E.exitKey   = (int64_t)f.Date(k) * 86400 + tod;                 // read by the order layer
        E.exitLong  = c < upT ? 1 : 0;
        E.exitShort = c > dnT ? 1 : 0;
        VTrade& v = E.vt;
        if (v.state == VS_OPEN) { if (v.dir > 0 ? E.exitLong : E.exitShort) v.exitNext = 1; return; }
        if (v.state != VS_FLAT || endT > P.LastEntry) return;
        if (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades) return;
        const int d = (c > ub && (!P.UseVwap || c > vw)) ? 1 : ((c < lb && (!P.UseVwap || c < vw)) ? -1 : 0);
        if (d == 0 || !DirOn(P, d)) return;
        Emit(f, E, P, k, d, KIND_MARKET, 0.0, P.CatStop * E.adr, 0.0, 0, "30-minute close beyond the noise band and VWAP");
    }

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
}   // namespace nbm

#ifndef NBM_CORE_ONLY
namespace nbm
{
    struct ScFeed
    {
        SCStudyInterfaceRef sc;
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
        int    OrderMode  = 0;        // 0 = send orders, 1 = signals only
        int    AllowLive  = 0;
        int    SizeMode   = 0;        // 0 = fixed contracts, 1 = % of the account input
        int    Qty        = 1;
        double Account    = 10000.0;  // used only by SizeMode 1 (an input, never the broker balance: lessons #5)
        double RiskPct    = 1.0;
        int    MaxQty     = 3;
        double MaxRiskUSD = 0.0;      // per trade, 0 = off
        double DailyLoss  = 0.0;      // $, 0 = off
        double ProfitLock = 0.0;      // $, 0 = off
        int    MaxConsec  = 0;        // losing trades in a row today, 0 = off
        int    Safety     = 0;        // 0 = block, 1 = warn only
        int    Inherited  = 0;        // 0 = flatten, 1 = leave alone
        int    EntryDelay = 120;      // s; input 0 = 120, -1 = off
        int    EarlyClose = 0;        // 0 = honour CME short sessions, 1 = ignore
        int    Fallbacks  = 0;
        int    PosCap() const { return SizeMode == 0 ? Qty : MaxQty; }
    };

    // Range-checked reads; out-of-range values fall back to the default and are counted (lessons #15).
    struct InputReader
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
        P.BandMult   = R.GF(IN_BAND_MULT, 0.1, 10.0, D.BandMult);
        P.UseVwap    = sc.Input[IN_USE_VWAP].GetYesNo() ? 1 : 0;
        P.FirstMark  = R.GT(IN_FIRST_MARK, D.FirstMark);
        P.LastEntry  = R.GT(IN_LAST_ENTRY, D.LastEntry);
        P.CatStop    = R.GF(IN_CAT_STOP, 0.05, 5.0, D.CatStop);
        P.SessOpen   = R.GT(IN_SESS_OPEN, D.SessOpen);
        P.SessClose  = R.GT(IN_SESS_CLOSE, D.SessClose);
        P.EOD        = R.GT(IN_EOD, D.EOD);
        if (!(P.SessOpen < P.EOD && P.EOD < P.SessClose)) { ++P.Fallbacks; P.SessOpen = D.SessOpen; P.SessClose = D.SessClose; P.EOD = D.EOD; }
        P.Directions = R.GX(IN_DIRECTIONS, 3, D.Directions);
        P.Lookback   = R.GI(IN_LOOKBACK, 3, MAXLOOK, D.Lookback);
        P.MaxTrades  = R.GI(IN_MAX_TRADES, 0, 50, D.MaxTrades);
        P.TallyCost  = R.GF(IN_TALLY_COST, 0.0, 50.0, D.TallyCost);
        P.Verbose    = sc.Input[IN_VERBOSE].GetYesNo() ? 1 : 0;
        return P;
    }

    static OParams ReadOParams(SCStudyInterfaceRef sc)
    {
        OParams O;
        const OParams D;
        InputReader R{ sc, O.Fallbacks };
        O.OrderMode  = R.GX(IN_ORDER_MODE, 2, D.OrderMode);
        O.AllowLive  = sc.Input[IN_ALLOW_LIVE].GetYesNo() ? 1 : 0;
        O.SizeMode   = R.GX(IN_SIZE_MODE, 2, D.SizeMode);
        O.Qty        = R.GI(IN_QTY, 1, 100, D.Qty);
        O.Account    = R.GF(IN_ACCOUNT, 100.0, 1.0e9, D.Account);
        O.RiskPct    = R.GF(IN_RISK_PCT, 0.01, 20.0, D.RiskPct);
        O.MaxQty     = R.GI(IN_MAX_QTY, 1, 100, D.MaxQty);
        O.MaxRiskUSD = R.GF(IN_MAX_RISK_USD, 0.0, 1.0e7, D.MaxRiskUSD);
        O.DailyLoss  = R.GF(IN_DAILY_LOSS, 0.0, 1.0e7, D.DailyLoss);
        O.ProfitLock = R.GF(IN_PROFIT_LOCK, 0.0, 1.0e7, D.ProfitLock);
        O.MaxConsec  = R.GI(IN_MAX_CONSEC, 0, 50, D.MaxConsec);
        O.Safety     = R.GX(IN_SAFETY, 2, D.Safety);
        O.Inherited  = R.GX(IN_INHERITED, 2, D.Inherited);
        const int ed = R.GI(IN_ENTRY_DELAY, -1, 3600, 0);
        O.EntryDelay = ed == 0 ? D.EntryDelay : ed;                 // 0 = the safe default, -1 = off (lessons #64)
        O.EarlyClose = R.GX(IN_EARLY_CLOSE, 2, D.EarlyClose);
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

    enum : int { ST_FLAT = 0, ST_PENDING = 1, ST_IN_TRADE = 2 };
    static const char* const kStateName[3] = { "FLAT", "PENDING", "IN_TRADE" };
    enum : int
    {
        FW_NONE = 0, FW_EOD, FW_ENGINE, FW_UNPROTECTED, FW_WRONG_SIDE, FW_OPPOSITE, FW_REISSUE, FW_INHERITED, FW_ORPHAN,
        FW_DAILY_LOSS, FW_COUNT
    };
    static const char* const kFlatWhy[FW_COUNT] =
    {
        "-", "EOD flatten time", "engine exit (close back through the band / VWAP)", "contracts not covered by a working stop (#14)",
        "attached stop on the wrong side of the fill", "position opposite to the trade", "flatten re-issued",
        "untracked position at EOD / outside trading hours", "position appeared that this study did not open",
        "daily loss limit reached with the trade open"
    };
    static const int kTraderMagic = 0x51544D33;

    // Order-layer state. Heap, PersistentPointer 2. Survives full recalculations on purpose
    // (#38, #43); index-valued fields are cleared on every one (#37). Time keys are seconds
    // since 1899-12-30 in trade time and survive both.
    struct Trader
    {
        int     magic;
        // the trade
        int     state, dir, sigSeq, kind, qty, canceling, expireTod, tradeDate;
        double  entryRef, dist, usd, cancelPx, entryFill, baseCumPL;
        int64_t parentID, stopID;
        int     fillVerified, flattenPending, flattenWhy, classifyArmed, baseTotalTrades;
        char    cancelWhy[96];
        // time keys
        int64_t submitAbs, submitBarKey, fillAbs, flattenAbs, covShortAbs, cancelAbs, staleCancelAbs, exitKeyDone, sigKeyDone;
        int     flattenCount, covShort;
        // run / recalculation
        int     postRecalc, flatSeen, inheritedWarned, liveBlockWarned, bannerDone, recalcLogged, sigSeqDone;
        // checks
        int     cfgDone, cfgFail, tzState;
        char    cfgWhy[240];
        int     feedWarnDay, clockWarned;
        // trading day (reset only when the trading day changes, #43)
        int     lastDay, tradesToday, winsToday, lossesToday, consecLosses, sigToday, sigTradedToday, haltLogged;
        double  dailyPL;
        int     runTrades, runWins;
        double  runPL;

        void ResetTrade()
        {
            state = ST_FLAT; dir = 0; sigSeq = 0; kind = 0; qty = 0; canceling = 0; expireTod = 0; tradeDate = 0;
            entryRef = dist = usd = cancelPx = entryFill = baseCumPL = 0.0;
            parentID = stopID = 0;
            fillVerified = flattenPending = flattenWhy = classifyArmed = baseTotalTrades = 0;
            cancelWhy[0] = 0;
            submitAbs = submitBarKey = fillAbs = flattenAbs = covShortAbs = cancelAbs = 0;
            flattenCount = covShort = 0;
        }
        void Init()
        {
            std::memset(this, 0, sizeof(*this));
            magic = kTraderMagic;
            ResetTrade();
            postRecalc = 1;                         // a fresh Trader treats signals already on the chart as history (#63)
        }
    };

    static void OrderLayer(SCStudyInterfaceRef sc, const Engine& E, const Params& P, const OParams& O, Trader& T)
    {
        const int    i    = sc.Index;
        const double tick = sc.TickSize;

        // Full recalculation: every order function returns SCT_SKIPPED_FULL_RECALC. Trade state and
        // the day's counters are kept (#38, #43); the engine's signal numbers restart, so they are re-synced.
        if (sc.IsFullRecalculation)
        {
            T.postRecalc = 1; T.flatSeen = 0; T.inheritedWarned = 0; T.liveBlockWarned = 0; T.bannerDone = 0; T.cfgDone = 0;
            if (i == sc.ArraySize - 1 && !T.recalcLogged)
            {
                T.recalcLogged = 1;
                sc.AddMessageToLog("NBM ORDERS: full recalculation complete -- orders are sent only on real-time, Chart Replay or "
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
        bool          acted    = false;
        char          msg[1000], hm[16], hm2[16];

        // ---- THE one position read of this call, before any order action (#17)
        s_SCPositionData pos;
        sc.GetTradePosition(pos);
        const double posQ    = (double)pos.PositionQuantity;
        const int    posSign = posQ > 0.0 ? 1 : (posQ < 0.0 ? -1 : 0);
        const int    posAbs  = (int)(DAbs(posQ) + 0.5);
        const bool   working = pos.WorkingOrdersExist != 0;
        const bool   tradeHappened = T.classifyArmed &&
            (pos.TotalTrades != T.baseTotalTrades || DAbs(pos.CumulativeProfitLoss - T.baseCumPL) > 0.01);
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
                    std::snprintf(msg, sizeof msg, "NBM FEED: system time and last-trade time differ by %d s -- not comparable "
                                  "(different time zones?); the delayed-data check is inactive.", rawLag);
                    sc.AddMessageToLog(msg, 0);
                }
            }
        }
        const bool blockSafety = O.Safety == 0;
        const bool feedBlock   = blockSafety && (feedDown || (delayed && !sc.GlobalTradeSimulationIsOn));
        if ((feedDown || delayed) && T.feedWarnDay != nowTD)
        {
            T.feedWarnDay = nowTD;
            if (feedDown)
                std::snprintf(msg, sizeof msg, "NBM FEED: data feed not connected (state %d) -- %s.", sc.ServerConnectionState,
                              blockSafety ? "no new entries; an open trade keeps its attached stop and is still managed" : "warn only");
            else
                std::snprintf(msg, sizeof msg, "NBM FEED: the last trade is %d s behind the system clock -- the data looks DELAYED. %s", rawLag,
                              sc.GlobalTradeSimulationIsOn ? "Simulation: entries continue; results will not match live trading."
                                                           : (blockSafety ? "LIVE entries blocked until the feed is real time." : "warn only."));
            sc.AddMessageToLog(msg, 1);
        }

        auto ReadOrder = [&](int64_t id, s_SCTradeOrder& o) -> bool
        {
            return id > 0 && sc.GetOrderByOrderID(static_cast<uint64_t>(id), o) != SCTRADING_ORDER_ERROR;
        };
        auto OpenPL = [&]() -> double
        {
            return (T.state == ST_IN_TRADE && T.entryFill > 0.0) ? T.dir * (lastPx - T.entryFill) * usdPerPt * posAbs : 0.0;
        };

        // Path-independent closed-trade classifier (#34): reads only the top-of-call position.
        auto Classify = [&](const char* how)
        {
            double      pl  = pos.CumulativeProfitLoss - T.baseCumPL;
            const char* src = "CumPL delta";
            if (pos.TotalTrades < T.baseTotalTrades) { pl = pos.LastTradeProfitLoss; src = "LastTradeProfitLoss (Sierra stats reset mid-trade)"; }
            ++T.runTrades; ++T.tradesToday;
            if (pl > 0.0) { ++T.runWins; ++T.winsToday; T.consecLosses = 0; } else { ++T.lossesToday; ++T.consecLosses; }
            T.runPL += pl; T.dailyPL += pl;
            s_SCTradeOrder o;
            const bool  sHit = ReadOrder(T.stopID, o) && o.OrderStatusCode == SCT_OSC_FILLED;
            const int   q    = T.qty > 0 ? T.qty : 1;
            std::snprintf(msg, sizeof msg, "NBM TRADE CLOSED (%s) %s signal #%d | exit: %s | P&L %.2f [%s] = %.2f pts/contract if currency, "
                          "%d contract(s) | planned risk %.2f pts | day: %d trade(s), %d win(s), %d loss(es), %d in a row, P&L %.2f | run: %d "
                          "trades, %d wins (%.0f%%), P&L %.2f",
                          how, DirName(T.dir), T.sigSeq, sHit ? "ATTACHED STOP" : (T.flattenWhy ? kFlatWhy[T.flattenWhy] : "flattened"), pl, src,
                          usdPerPt > 0.0 ? pl / usdPerPt / q : 0.0, q, T.dist, T.tradesToday, T.winsToday, T.lossesToday, T.consecLosses,
                          T.dailyPL, T.runTrades, T.runWins, 100.0 * T.runWins / T.runTrades, T.runPL);
            sc.AddMessageToLog(msg, 0);
            T.classifyArmed = 0;
        };

        // Every script exit of the position. State is kept so reconciliation classifies it.
        auto Flatten = [&](int why)
        {
            const int r = sc.FlattenAndCancelAllOrders();
            T.flattenPending = 1; T.flattenAbs = nowAbs; ++T.flattenCount; acted = true;
            if (why != FW_REISSUE) T.flattenWhy = (why > FW_NONE && why < FW_COUNT) ? why : FW_NONE;
            std::snprintf(msg, sizeof msg, "NBM FLATTEN (%s) -- FlattenAndCancelAllOrders()=%d; classified once the broker reports flat.",
                          kFlatWhy[why > FW_NONE && why < FW_COUNT ? why : FW_NONE], r);
            sc.AddMessageToLog(msg, (why == FW_EOD || why == FW_ENGINE) ? 0 : 1);
        };

        auto CancelPending = [&](const char* why)
        {
            const int r = sc.CancelAllOrders();
            T.canceling = 1; T.cancelAbs = nowAbs; acted = true;
            std::snprintf(T.cancelWhy, sizeof T.cancelWhy, "%s", why);
            std::snprintf(msg, sizeof msg, "NBM ENTRY CANCEL signal #%d (%s) -- CancelAllOrders()=%d; a fill that wins the race is adopted "
                          "with its attached stop.", T.sigSeq, why, r);
            sc.AddMessageToLog(msg, 0);
        };

        // A resting stop entry is void once price has traded through the other side. The study is not called on
        // every trade, so the bars' extremes since the order went in are read, not only the last price.
        auto OppositeBroke = [&]() -> bool
        {
            if (T.dir * (lastPx - T.cancelPx) <= 0.0) return true;
            for (int k = nowBar; k >= nowBar - 1 && k >= 0; --k)
            {
                if ((int64_t)sc.BaseDateTimeIn.DateAt(k) * 86400 + sc.BaseDateTimeIn.TimeAt(k) < T.submitBarKey) break;
                if (T.dir * ((T.dir > 0 ? (double)sc.Low[k] : (double)sc.High[k]) - T.cancelPx) <= 0.0) return true;
            }
            return false;
        };

        // Plan and submit the entry for the engine signal. false = not submitted (reason in skip).
        char skip[320]; skip[0] = 0;
        auto Submit = [&]() -> bool
        {
            const int    d    = E.sigDir;
            const double dist = DMax(sc.RoundToTickSize(E.sigStopDist), 2.0 * tick);
            if (usdPerPt <= 0.0) { std::snprintf(skip, sizeof skip, "$ per point unknown (check the symbol settings)"); return false; }
            int qty = O.Qty;
            if (O.SizeMode == 1)
            {
                qty = (int)std::floor(O.Account * O.RiskPct / 100.0 / (dist * usdPerPt) + 1e-9);
                if (qty > O.MaxQty) qty = O.MaxQty;
                if (qty < 1) { std::snprintf(skip, sizeof skip, "risk %.2f%% of $%.0f buys less than 1 contract at a %.2f pt stop ($%.2f)",
                                             O.RiskPct, O.Account, dist, dist * usdPerPt); return false; }
            }
            if (O.MaxRiskUSD > 0.0)
            {
                while (qty > 1 && dist * usdPerPt * qty > O.MaxRiskUSD + 1e-9) --qty;
                if (dist * usdPerPt * qty > O.MaxRiskUSD + 1e-9)
                { std::snprintf(skip, sizeof skip, "risk $%.2f for 1 contract is over the $%.2f cap", dist * usdPerPt, O.MaxRiskUSD); return false; }
            }
            int    kind = E.sigKind;
            double trig = sc.RoundToTickSize(E.sigTrig);
            if (kind == KIND_STOP)
            {
                if (d * (lastPx - E.sigCancelPx) <= 0.0)
                { std::snprintf(skip, sizeof skip, "price %.2f is already through the opposite side %.2f", lastPx, E.sigCancelPx); return false; }
                const double through = d * (lastPx - trig);                 // price may open beyond the trigger (#42)
                if (through >= 0.0)
                {
                    if (through > P.ChaseTicks * tick + 1e-9)
                    { std::snprintf(skip, sizeof skip, "price %.2f already %.2f pts beyond the trigger %.2f (chase allowance %d ticks)",
                                    lastPx, through, trig, P.ChaseTicks); return false; }
                    kind = KIND_MARKET;                                     // same stop distance from the fill: risk unchanged (#20)
                }
            }
            T.baseCumPL       = pos.CumulativeProfitLoss;                   // classification baseline (#34)
            T.baseTotalTrades = pos.TotalTrades;
            s_SCNewOrder o;
            o.OrderType              = kind == KIND_STOP ? SCT_ORDERTYPE_STOP : SCT_ORDERTYPE_MARKET;
            if (kind == KIND_STOP) o.Price1 = trig;
            o.OrderQuantity          = qty;
            o.TimeInForce            = SCT_TIF_DAY;
            o.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP;
            o.Stop1Offset            = dist;                                // an offset from the actual fill keeps the risk (#20)
            SCString tag;
            tag.Format("NBM #%d", E.sigSeq);
            o.TextTag = tag;
            const int r = static_cast<int>(d > 0 ? sc.BuyEntry(o) : sc.SellEntry(o));
            acted = true;
            if (r <= 0)
            {
                std::snprintf(skip, sizeof skip, "%s entry REJECTED (%d: %s) -- see Trade >> Trade Service Log", d > 0 ? "BUY" : "SELL", r,
                              sc.GetTradingErrorTextMessage(r));
                return false;
            }
            T.ResetTrade();
            T.state = ST_PENDING; T.dir = d; T.sigSeq = E.sigSeq; T.kind = kind; T.qty = qty; T.dist = dist; T.tradeDate = nowTD;
            T.entryRef = kind == KIND_STOP ? trig : lastPx; T.cancelPx = E.sigCancelPx; T.expireTod = E.sigExpireTod;
            T.parentID = o.InternalOrderID; T.stopID = o.Stop1InternalOrderID;
            T.usd = dist * usdPerPt * qty; T.submitAbs = nowAbs; T.classifyArmed = 1;
            T.submitBarKey = (int64_t)barDate * 86400 + barTod;
            T.baseCumPL = pos.CumulativeProfitLoss; T.baseTotalTrades = pos.TotalTrades;
            if (T.stopID == 0)
                sc.AddMessageToLog("NBM: entry accepted but NO attached stop ID returned -- the position will be flattened if no stop works (#14).", 1);
            std::snprintf(msg, sizeof msg, "NBM ORDER %s signal #%d | %s x%d at %s%.2f | attached stop offset %.2f pts (stop ~%.2f) | $risk %.2f",
                          DirName(d), E.sigSeq, kind == KIND_STOP ? "STOP entry" : "MARKET", qty, kind == KIND_STOP ? "" : "~", T.entryRef, dist,
                          T.entryRef - d * dist, T.usd);
            sc.AddMessageToLog(msg, 0);
            return true;
        };

        // ---------------------------------------------------------- banner
        if (!T.bannerDone)
        {
            T.bannerDone = 1;
            const bool sim = sc.GlobalTradeSimulationIsOn != 0;
            const bool off = O.OrderMode != 0;
            std::snprintf(msg, sizeof msg, "NBM ORDERS %s | Trade Simulation Mode %s -> SendOrdersToTradeService=%d%s",
                          off ? "OFF -- signals only, NO ORDERS ARE SENT" : "SEND", sim ? "ON" : "OFF", (int)sc.SendOrdersToTradeService,
                          off ? "" : (liveBlocked ? " | LIVE ENTRIES BLOCKED: Sim is OFF and 'Allow LIVE orders' = No"
                                                  : (!sim ? " | *** LIVE ORDERS ENABLED ***" : "")));
            sc.AddMessageToLog(msg, (off || liveBlocked || !sim) ? 1 : 0);
            HHMM(hm, sizeof hm, P.EOD);
            char sz[120];
            if (O.SizeMode == 0) std::snprintf(sz, sizeof sz, "%d contract(s) fixed", O.Qty);
            else std::snprintf(sz, sizeof sz, "%.2f%% of the $%.0f account input per trade, at most %d contract(s)", O.RiskPct, O.Account, O.MaxQty);
            std::snprintf(msg, sizeof msg, "NBM plan: %s | MaximumPositionAllowed %d | max $ risk per trade %s | daily loss halt %s | daily "
                          "profit lock %s | max losses in a row %s | max trades per day %d | EOD flatten %s (CME short sessions: same distance "
                          "before the halt) | %s $/pt %.2f%s",
                          sz, O.PosCap(), O.MaxRiskUSD > 0.0 ? "ON" : "OFF", O.DailyLoss > 0.0 ? "ON" : "OFF", O.ProfitLock > 0.0 ? "ON" : "OFF",
                          O.MaxConsec > 0 ? "ON" : "OFF", P.MaxTrades, hm, sc.Symbol.GetChars(), usdPerPt,
                          usdPerPt > 0.0 ? "" : " *** $/pt UNKNOWN: no entries ***");
            sc.AddMessageToLog(msg, usdPerPt > 0.0 ? 0 : 1);
            if (O.Fallbacks + P.Fallbacks > 0)
            {
                std::snprintf(msg, sizeof msg, "NBM: %d input(s) out of range -> defaults used. Re-enter them in Study Settings "
                              "(an input added after the study was placed reads 0, lessons #15).", O.Fallbacks + P.Fallbacks);
                sc.AddMessageToLog(msg, 1);
            }
        }
        if (liveBlocked && O.OrderMode == 0 && !T.liveBlockWarned)
        {
            T.liveBlockWarned = 1;
            sc.AddMessageToLog("NBM: Trade Simulation Mode is OFF and 'Allow LIVE orders' = No -> no new entries. "
                               "An open position is still managed and exited.", 1);
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
            std::snprintf(msg, sizeof msg, "NBM CHART CHECK %s | time zone %s (%s; chart setting '%s') | bars %d s | symbol %s | day sessions "
                          "loaded %d | %s%s", T.cfgFail ? "FAILED" : "ok", T.tzState == 1 ? "New York" : (T.tzState == 2 ? "NOT New York" : "unknown"),
                          tzInfo, tzName.GetChars(), sc.SecondsPerBar, sym != nullptr ? sym : "?", E.sessions,
                          T.cfgFail ? (O.Safety == 0 ? "NO NEW ENTRIES: " : "warn only: ") : "", T.cfgWhy);
            sc.AddMessageToLog(msg, (T.cfgFail || T.tzState == 0) ? 1 : 0);
            char ed[24];
            if (O.EntryDelay > 0) std::snprintf(ed, sizeof ed, "%d s", O.EntryDelay); else std::snprintf(ed, sizeof ed, "OFF");
            std::snprintf(msg, sizeof msg, "NBM ENVIRONMENT | trade account '%s' | Auto Trading Enabled %d | trading locked %d | %s | data feed "
                          "state %d (2 = connected) | system time - last trade %d s | safety checks %s | untracked positions %s | max entry "
                          "delay %s | CME early closes %s", sc.SelectedTradeAccount.GetChars(), (int)sc.IsAutoTradingEnabled,
                          (int)sc.TradingIsLocked, realTime ? "REAL TIME" : "Chart Replay / back test", (int)sc.ServerConnectionState, rawLag,
                          O.Safety == 0 ? "BLOCK" : "WARN ONLY", O.Inherited == 0 ? "flattened" : "left alone", ed,
                          O.EarlyClose == 0 ? "honoured" : "IGNORED");
            sc.AddMessageToLog(msg, (sc.IsAutoTradingEnabled == 0 || sc.TradingIsLocked != 0 || O.Safety != 0 || O.Inherited != 0 ||
                                     O.EntryDelay < 0 || O.EarlyClose != 0) ? 1 : 0);
        }
        const bool cfgBlock = O.Safety == 0 && T.cfgFail != 0;

        // ---- first live call after a recalculation: signals on the chart are history; re-adopt or discard the trade (#38)
        if (T.postRecalc)
        {
            T.postRecalc = 0;
            if (E.sigKey <= T.sigKeyDone || E.sigBar != nowBar - 1) { T.sigSeqDone = E.sigSeq; T.sigKeyDone = E.sigKey; }   // history
            if (T.state != ST_FLAT)
            {
                if (posSign != 0 && posSign == T.dir)
                {
                    if (T.state == ST_PENDING) { T.fillAbs = nowAbs; T.entryFill = pos.AveragePrice > 0.0 ? pos.AveragePrice : T.entryRef; }
                    T.state = ST_IN_TRADE; T.covShort = 0; T.canceling = 0;        // fillVerified kept (#60)
                    std::snprintf(msg, sizeof msg, "NBM: %s position (signal #%d) carried across a recalculation -- management resumes.",
                                  DirName(T.dir), T.sigSeq);
                    sc.AddMessageToLog(msg, 1);
                }
                else if (posSign == 0 && T.state == ST_PENDING && working && !T.canceling && T.kind == KIND_STOP)
                {
                    sc.AddMessageToLog("NBM: resting entry order carried across a recalculation -- still managed.", 0);
                }
                else if (posSign == 0)
                {
                    if (tradeHappened) Classify("closed while the chart recalculated");
                    if (working) { sc.CancelAllOrders(); acted = true; }
                    std::snprintf(msg, sizeof msg, "NBM: %s state from before the recalculation cleared (broker flat).", kStateName[T.state % 3]);
                    sc.AddMessageToLog(msg, 0);
                    T.ResetTrade();
                }
                else
                {
                    sc.AddMessageToLog("NBM: the broker position is opposite to the trade from before the recalculation -- treated as untracked.", 1);
                    T.ResetTrade();
                }
            }
        }
        if (posSign == 0 && !working) T.flatSeen = 1;

        // ---- a flatten in flight: re-issue after 30 s, then every 5 minutes with a loud line (#69)
        if (T.flattenPending)
        {
            if (posSign == 0) { T.flattenPending = 0; T.flattenCount = 0; }
            else if (!acted)
            {
                const int64_t wait = T.flattenCount < 10 ? 30 : 300;
                if (nowAbs - T.flattenAbs >= wait)
                {
                    if (T.flattenCount >= 10)
                        sc.AddMessageToLog("NBM *** MANUAL ACTION NEEDED *** the position is still open after 10 flatten attempts -- "
                                           "close it in the Trade Window. Retrying every 5 minutes.", 1);
                    Flatten(FW_REISSUE);
                }
            }
        }

        // ---- reconciliation, before the daily rollover (#35)
        if (T.state == ST_IN_TRADE)
        {
            if (posSign == 0)
            {
                Classify("broker flat");
                if (working && !acted) { sc.CancelAllOrders(); acted = true; }      // nothing may outlive the trade (#41)
                T.ResetTrade();
            }
            else if (posSign != T.dir && !T.flattenPending && !acted) Flatten(FW_OPPOSITE);
        }
        else if (T.state == ST_PENDING)
        {
            if (posSign == T.dir && posSign != 0)
            {
                T.state = ST_IN_TRADE; T.canceling = 0; T.fillAbs = nowAbs;
                T.entryFill = pos.AveragePrice > 0.0 ? (double)pos.AveragePrice : T.entryRef;    // never assign a 0 average (#30)
                std::snprintf(msg, sizeof msg, "NBM FILL %s signal #%d | %d contract(s) at %.2f (planned %.2f, slippage %+.2f pts)",
                              DirName(T.dir), T.sigSeq, posAbs, T.entryFill, T.entryRef, T.dir * (T.entryFill - T.entryRef));
                sc.AddMessageToLog(msg, 0);
            }
            else if (posSign != 0) { if (!T.flattenPending && !acted) Flatten(FW_OPPOSITE); }
            else if (tradeHappened)                                          // filled and closed between two calls (#34, #40)
            {
                Classify("filled and closed between two calls");
                if (working && !acted) { sc.CancelAllOrders(); acted = true; }
                T.ResetTrade();
            }
            else if (T.canceling)
            {
                if (!working)
                {
                    std::snprintf(msg, sizeof msg, "NBM ENTRY signal #%d not filled -- cancelled (%s).", T.sigSeq, T.cancelWhy);
                    sc.AddMessageToLog(msg, 0);
                    T.ResetTrade();
                }
                else if (nowAbs - T.cancelAbs >= 5 && !acted) CancelPending(T.cancelWhy);
            }
            else if (!acted)
            {
                if (T.kind == KIND_MARKET) { if (nowAbs - T.submitAbs >= 60) CancelPending("market entry not filled after 60 s"); }
                else if (!inHours)                                   CancelPending("outside trading hours");
                else if (nowTod >= T.expireTod)                      CancelPending("entry window ended");
                else if (OppositeBroke())                            CancelPending("the opposite side of the opening range broke first");
                else if (!working && nowAbs - T.submitAbs >= 5)      CancelPending("the resting entry is no longer working");
            }
        }
        else                                                                 // ST_FLAT
        {
            if (posSign != 0 && !T.flattenPending)
            {
                if (T.flatSeen && !acted) Flatten(FW_ORPHAN);                // appeared while this study was flat (#72: one strategy per account)
                else if (!T.flatSeen)
                {
                    if (!T.inheritedWarned)
                    {
                        T.inheritedWarned = 1;
                        std::snprintf(msg, sizeof msg, "NBM: the account already holds %+.0f contract(s) this study did not open -- no new "
                                      "entries; %s.", posQ, O.Inherited == 0 ? "it is flattened at the EOD time, or at once outside trading hours"
                                                                           : "left alone (input)");
                        sc.AddMessageToLog(msg, 1);
                    }
                    if (O.Inherited == 0 && !inHours && !acted) Flatten(FW_INHERITED);
                }
            }
            else if (posSign == 0 && working && !acted && nowAbs - T.staleCancelAbs >= 5)
            {
                T.staleCancelAbs = nowAbs;
                sc.CancelAllOrders(); acted = true;                           // working orders can outlive the state (#41)
                sc.AddMessageToLog("NBM: working orders found while flat -- cancelled.", 1);
            }
        }

        // ---- trading-day rollover: counters and halts reset only here (#43)
        if (nowTD != T.lastDay)
        {
            if (T.lastDay != 0 && (T.sigToday > 0 || T.tradesToday > 0))
            {
                std::snprintf(msg, sizeof msg, "NBM DAY SUMMARY | signals %d, traded %d | trades closed %d (%d wins) | P&L %.2f | run P&L %.2f",
                              T.sigToday, T.sigTradedToday, T.tradesToday, T.winsToday, T.dailyPL, T.runPL);
                sc.AddMessageToLog(msg, 0);
            }
            T.lastDay = nowTD; T.tradesToday = T.winsToday = T.lossesToday = T.consecLosses = T.sigToday = T.sigTradedToday = 0;
            T.dailyPL = 0.0; T.haltLogged = 0;
            HHMM(hm, sizeof hm, effEOD);
            std::snprintf(msg, sizeof msg, "NBM STATUS | session %s | flat time %s | engine: averages %s (avg range %.2f, prior close %.2f) | "
                          "order state %s | broker position %+.0f", haltTod > 0 ? "SHORT (CME halt) -- no entries" : "normal", hm,
                          E.ringN >= P.Lookback ? "ready" : "NOT ready", E.adr, E.prevClose, kStateName[T.state % 3], posQ);
            sc.AddMessageToLog(msg, 0);
        }

        // ---- risk halts (each isolated, one gate; a limit of 0 is OFF, never "trip at once": #36)
        const double dayPLNow = T.dailyPL + OpenPL();
        const bool haltLoss   = O.DailyLoss > 0.0 && dayPLNow <= -O.DailyLoss;
        const bool haltProfit = O.ProfitLock > 0.0 && T.dailyPL >= O.ProfitLock;
        const bool haltStreak = O.MaxConsec > 0 && T.consecLosses >= O.MaxConsec;
        const bool haltCount  = P.MaxTrades > 0 && T.tradesToday >= P.MaxTrades;
        if ((haltLoss || haltProfit || haltStreak) && !T.haltLogged)
        {
            T.haltLogged = 1;
            std::snprintf(msg, sizeof msg, "NBM RISK HALT for the rest of the day: %s | day P&L %.2f (open %.2f), losses in a row %d. "
                          "Resets on the next trading day.", haltLoss ? "daily loss limit" : (haltProfit ? "daily profit lock" : "losing streak"),
                          dayPLNow, OpenPL(), T.consecLosses);
            sc.AddMessageToLog(msg, 1);
        }

        // ---- the open trade
        if (T.state == ST_IN_TRADE && posSign == T.dir && !T.flattenPending && !acted)
        {
            s_SCTradeOrder so;
            const bool haveStop = ReadOrder(T.stopID, so) && IsWorkingOrderStatusIgnorePendingChildren(so.OrderStatusCode);
            // FILL VERIFY, once per fill and kept through a recalculation (#60)
            if (!T.fillVerified && haveStop)
            {
                T.fillVerified = 1;
                const double want = T.entryFill - T.dir * T.dist;
                const bool   wrong = T.dir * (T.entryFill - so.Price1) <= 0.0;
                std::snprintf(msg, sizeof msg, "NBM FILL VERIFY %s | fill %.2f | stop %.2f (planned %.2f, difference %+.2f) | stop quantity %.0f "
                              "of %d", wrong ? "*** WRONG SIDE ***" : "ok", T.entryFill, so.Price1, want, so.Price1 - want,
                              (double)so.OrderQuantity, posAbs);
                sc.AddMessageToLog(msg, wrong ? 1 : 0);
                if (wrong) Flatten(FW_WRONG_SIDE);
            }
            // every contract covered by a working stop; 5 s of trade time before acting (#68)
            const int cov = haveStop ? (int)((double)so.OrderQuantity + 0.5) : 0;
            if (!acted)
            {
                if (cov >= posAbs) T.covShort = 0;
                else if (!T.covShort) { T.covShort = 1; T.covShortAbs = nowAbs; }
                else if (nowAbs - T.covShortAbs >= 5)
                {
                    std::snprintf(msg, sizeof msg, "NBM: %d contract(s) open but only %d covered by a working stop for 5 s -- flattening.", posAbs, cov);
                    sc.AddMessageToLog(msg, 1);
                    Flatten(FW_UNPROTECTED);
                }
            }
            if (!acted && !inHours) Flatten(FW_EOD);
            if (!acted && haltLoss) Flatten(FW_DAILY_LOSS);
            // engine exit: decided on a bar that closed after the fill, handled once
            if (!acted && engOK && E.exitKey != T.exitKeyDone && E.exitKey + bsec > T.fillAbs)
            {
                T.exitKeyDone = E.exitKey;
                if ((T.dir > 0 && E.exitLong) || (T.dir < 0 && E.exitShort)) Flatten(FW_ENGINE);
            }
        }

        // ---- a new engine signal: trade it or say why not
        if (engOK && E.sigSeq > 0 && E.sigKey > T.sigKeyDone)                // keyed by time: survives recalculations
        {
            T.sigSeqDone = E.sigSeq; T.sigKeyDone = E.sigKey;
            ++T.sigToday;
            const int64_t due = E.sigKey + bsec;                             // the signal bar's close
            const char*   why = nullptr;
            char          wbuf[160];
            if (O.OrderMode != 0)                         why = "Orders = signals only";
            else if (E.sigBar != nowBar - 1)              why = "the signal bar is not the bar that just closed (late call)";
            else if (O.EntryDelay > 0 && nowAbs - due > O.EntryDelay)
            { std::snprintf(wbuf, sizeof wbuf, "%d s after the signal bar closed (max %d s)", (int)(nowAbs - due), O.EntryDelay); why = wbuf; }
            else if (!inHours)                            why = "outside trading hours / past the flat time";
            else if (haltTod > 0)                         why = "CME short session";
            else if (T.state != ST_FLAT)                  why = "a trade or entry order is already open";
            else if (posSign != 0)                        why = "the account holds a position this study does not track";
            else if (T.flattenPending)                    why = "a flatten is in flight";
            else if (acted)                               why = "another order action was taken in this call (#17)";
            else if (liveBlocked)                         why = "live trading not allowed (input)";
            else if (cfgBlock)                            why = "CHART CHECK failed";
            else if (feedBlock)                           why = "data feed down or delayed";
            else if (haltLoss)                            why = "daily loss halt";
            else if (haltProfit)                          why = "daily profit lock";
            else if (haltStreak)                          why = "losing-streak halt";
            else if (haltCount)                           why = "max trades per day reached";
            else if (!DirOn(P, E.sigDir))                 why = "direction switched off";
            if (why == nullptr && !Submit()) why = skip;
            if (why == nullptr) ++T.sigTradedToday;
            std::snprintf(msg, sizeof msg, "NBM engine SIGNAL #%d %s -> %s%s", E.sigSeq, DirName(E.sigDir),
                          why == nullptr ? "TRADED" : "NOT traded -- ", why == nullptr ? "" : why);
            sc.AddMessageToLog(msg, (why != nullptr && O.OrderMode == 0) ? 1 : 0);
        }
    }
}   // namespace nbm

SCSFExport scsf_NoiseBandMomentum_NQ_MNQ(SCStudyInterfaceRef sc)
{
    using namespace nbm;

    if (sc.SetDefaults)
    {
        sc.GraphName        = "Noise Band Momentum NQ MNQ " STUDY_VERSION;
        sc.StudyDescription = "Intraday momentum on MNQ: at each 30-minute mark, long when the close is above the upper noise band (open / prior close +- the 14-session average move for that time of day) and above session VWAP, short mirrored; exit when a 30-minute close is back through max(band, VWAP); attached disaster stop; flat at 15:55 New York. MNQ 5-minute chart, New York time.";
        sc.AutoLoop         = 1;
        sc.GraphRegion      = 0;
        sc.ValueFormat      = VALUEFORMAT_INHERITED;
        sc.DrawZeros        = 0;

        auto Style = [&](int idx, const char* name, int style, COLORREF col, int width)
        {
            SCSubgraphRef s = sc.Subgraph[idx];
            s.Name = name; s.DrawStyle = style; s.PrimaryColor = col; s.LineWidth = width; s.DrawZeros = 0;
        };
        Style(SG_VWAP,      "Session VWAP (from the day-session open)", DRAWSTYLE_LINE,          RGB(200, 140, 40),  2);
        Style(SG_UB,        "Noise band, upper",                         DRAWSTYLE_LINE,            RGB(63, 185, 80),   1);
        Style(SG_LB,        "Noise band, lower",                         DRAWSTYLE_LINE,            RGB(248, 81, 73),   1);
        Style(SG_L1,        "(unused)",                                 DRAWSTYLE_IGNORE,               RGB(88, 166, 255),  2);
        Style(SG_L2,        "(unused)",                                 DRAWSTYLE_IGNORE,               RGB(150, 150, 150), 2);
        Style(SG_SIG_LONG,  "LONG signal",                               DRAWSTYLE_ARROW_UP,      RGB(63, 185, 80),   3);
        Style(SG_SIG_SHORT, "SHORT signal",                              DRAWSTYLE_ARROW_DOWN,    RGB(248, 81, 73),   3);
        Style(SG_VENTRY,    "Virtual entry (tally)",                     DRAWSTYLE_POINT,         RGB(88, 166, 255),  4);
        Style(SG_VEXIT,     "Virtual exit (tally)",                      DRAWSTYLE_POINT,         RGB(255, 150, 0),   4);
        Style(SG_STATE,     "Engine trade +1/-1 (stored)",               DRAWSTYLE_IGNORE,        RGB(200, 200, 200), 1);
        Style(SG_ORD_ENTRY, "Order: entry / fill",                       DRAWSTYLE_DASH,          RGB(88, 166, 255),  2);
        Style(SG_ORD_STOP,  "Order: attached stop",                      DRAWSTYLE_DASH,          RGB(248, 81, 73),   2);
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
        DefFL(IN_BAND_MULT,   "Noise band width, x the average move for the time of day (measured 1.0)", D.BandMult, 0.1, 10.0);
        DefYN(IN_USE_VWAP,    "Use session VWAP in the entry and the trailing exit (measured Yes)", D.UseVwap);
        DefTM(IN_FIRST_MARK,  "First 30-minute decision at (bar close, New York; measured 10:00)", D.FirstMark);
        DefTM(IN_LAST_ENTRY,  "Last decision that may open a trade (measured 15:30)", D.LastEntry);
        DefFL(IN_CAT_STOP,    "Attached disaster stop, x average day range (measured 0.5)", D.CatStop, 0.05, 5.0);
        DefTM(IN_SESS_OPEN,   "Day session opens (New York)", D.SessOpen);
        DefTM(IN_SESS_CLOSE,  "Day session closes (New York)", D.SessClose);
        DefTM(IN_EOD,         "Flat time: cancel entries, close the position (New York; measured 15:55)", D.EOD);
        DefCX(IN_DIRECTIONS,  "Directions", "Long and short (measured);Long only;Short only", D.Directions);
        DefIN(IN_LOOKBACK,    "Sessions in the average range / noise profile (measured 14)", D.Lookback, 3, MAXLOOK);
        DefIN(IN_MAX_TRADES,  "Max trades per day (0 = off)", D.MaxTrades, 0, 50);
        DefFL(IN_TALLY_COST,  "Tally cost per contract per round trip, points (diagnostic only)", D.TallyCost, 0.0, 50.0);
        DefYN(IN_VERBOSE,     "Verbose log (every virtual entry and exit during the history load)", D.Verbose);

        const OParams DO;
        DefCX(IN_ORDER_MODE,  "Orders", "Send orders (Trade Simulation; live needs the next input);Signals only -- no orders", DO.OrderMode);
        DefYN(IN_ALLOW_LIVE,  "Allow LIVE orders (when Trade Simulation Mode is OFF)", DO.AllowLive);
        DefCX(IN_SIZE_MODE,   "Position size", "Fixed contracts;Risk % of the account size input", DO.SizeMode);
        DefIN(IN_QTY,         "Fixed contracts", DO.Qty, 1, 100);
        DefFL(IN_ACCOUNT,     "Account size, $ (for Risk % sizing; typed here, not read from the broker)", DO.Account, 100.0, 1.0e9);
        DefFL(IN_RISK_PCT,    "Risk per trade, % of the account size (Risk % sizing)", DO.RiskPct, 0.01, 20.0);
        DefIN(IN_MAX_QTY,     "Max contracts (Risk % sizing)", DO.MaxQty, 1, 100);
        DefFL(IN_MAX_RISK_USD, "Max $ risk per trade (0 = off; reduces contracts, skips if 1 is over)", DO.MaxRiskUSD, 0.0, 1.0e7);
        DefFL(IN_DAILY_LOSS,  "Daily loss halt, $ incl. the open trade (0 = off)", DO.DailyLoss, 0.0, 1.0e7);
        DefFL(IN_PROFIT_LOCK, "Daily profit lock, $: no new entries after (0 = off)", DO.ProfitLock, 0.0, 1.0e7);
        DefIN(IN_MAX_CONSEC,  "Max losing trades in a row per day (0 = off)", DO.MaxConsec, 0, 50);
        DefCX(IN_SAFETY,      "Safety checks (chart: New York time, 5-min bars, MNQ; feed: connected, real time)",
              "Block new entries when a check fails (recommended);Warn only", DO.Safety);
        DefCX(IN_INHERITED,   "Untracked position on this symbol/account (study re-added, restart)",
              "Flatten at the flat time, or at once outside trading hours (recommended);Leave alone", DO.Inherited);
        DefIN(IN_ENTRY_DELAY, "Max entry delay after the signal bar closes, seconds (0 = 120 default, -1 = off)", 0, -1, 3600);
        DefCX(IN_EARLY_CLOSE, "CME holiday halts / early closes (13:00 / 13:15 New York)",
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

    // Trade-behaviour settings on EVERY call, never only in SetDefaults (lessons #15/#16).
    sc.SendOrdersToTradeService                       = !sc.GlobalTradeSimulationIsOn;  // read, never assigned
    sc.MaximumPositionAllowed                         = O.PosCap();                      // lessons #8
    sc.SupportReversals                               = 0;
    sc.AllowOppositeEntryWithOpposingPositionOrOrders = 0;
    sc.SupportAttachedOrdersForTrading                = 1;
    sc.UseGUIAttachedOrderSetting                     = 0;
    sc.CancelAllOrdersOnEntriesAndReversals           = 0;
    sc.CancelAllOrdersOnEntries                       = 0;
    sc.CancelAllOrdersOnReversals                     = 0;
    sc.AllowEntryWithWorkingOrders                    = 0;   // one entry order at a time
    sc.AllowOnlyOneTradePerBar                        = 0;   // an exit and a later entry may share a bar
    sc.AllowMultipleEntriesInSameDirection            = 0;
    sc.CancelAllWorkingOrdersOnExit                   = 0;   // exits use FlattenAndCancelAllOrders
    sc.SupportTradingScaleIn                          = 0;
    sc.SupportTradingScaleOut                         = 0;
    sc.MaintainTradeStatisticsAndTradesData           = 1;

    // lessons #37: bar indices restart, so reset the engine once at the start of every full recalculation.
    if (sc.IsFullRecalculation && i == 0 && !E.recalcOpen) { E.ResetAll(); E.recalcOpen = 1; }
    if (!E.bannerDone)
    {
        E.bannerDone = 1;
        char msg[400];
        std::snprintf(msg, sizeof msg, "NBM Noise Band Momentum NQ MNQ " STUDY_VERSION " | built " STUDY_BUILD_STAMP " | chart #%d %s | %d s bars | "
                      "inputs out of range: %d", sc.ChartNumber, sc.Symbol.GetChars(), sc.SecondsPerBar, P.Fallbacks + O.Fallbacks);
        sc.AddMessageToLog(msg, 0);
    }
    if (sc.TickSize <= 0.0f)
    {
        if (!E.tickWarned) { E.tickWarned = 1; sc.AddMessageToLog("NBM: sc.TickSize is 0 -- engine idle. Check the symbol settings.", 1); }
        return;
    }
    E.usdPerPt   = sc.CurrencyValuePerTick > 0.0f ? (double)sc.CurrencyValuePerTick / sc.TickSize : 0.0;
    E.virtualLog = O.OrderMode != 0 ? 1 : 0;      // live virtual-trade lines only when no orders are sent (or Verbose)

    ScFeed f{ sc };
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
        for (int g = 0; g <= SG_L2; ++g) sc.Subgraph[g][i] = i > 0 ? sc.Subgraph[g][i - 1] : 0.0f;

    if (!E.tallyLogged && E.lastCommitted >= sc.ArraySize - 2)
    {
        E.tallyLogged = 1;
        LogTally(f, E, P, "(all chart history committed)");
    }
    if (!sc.IsFullRecalculation || i == sc.ArraySize - 1) E.recalcOpen = 0;

    OrderLayer(sc, E, P, O, T);

    const bool showOrd = !sc.IsFullRecalculation && (T.state == ST_PENDING || T.state == ST_IN_TRADE);
    const double ent = T.state == ST_IN_TRADE && T.entryFill > 0.0 ? T.entryFill : T.entryRef;
    sc.Subgraph[SG_ORD_ENTRY][i] = showOrd ? (float)ent : 0.0f;
    sc.Subgraph[SG_ORD_STOP][i]  = showOrd ? (float)(ent - T.dir * T.dist) : 0.0f;
}
#endif
