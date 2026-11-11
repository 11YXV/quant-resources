#include "sierrachart.h"

SCDLLName("MomentumSuite_NQ_MNQ")

/*============================================================================
  MomentumSuite_NQ_MNQ  v2  --  six strategies, ONE trade account
  (Lessons #12: the two lines above stay at the very top of the file.)

  WHAT IT DOES
    Runs six day-session strategies in a single study, so they can be
    Replay-tested tick by tick and traded on one account. Each has its own
    on/off switch and contract count, so each can also be tested alone:
      NBM  Noise-band momentum: at each 30-minute mark 10:00-15:30, long when
           the close is above the upper noise band and above VWAP (short
           mirrored); exit when a 30-minute close is back through
           max(band, VWAP); attached disaster stop 0.5 x average day range.
      GOD  Gap opening drive: gap >= 0.05 x average range -> stop entry one
           tick beyond the first 5-minute range on the gap's side; cancelled
           when the other side trades first or at 10:30; stop 0.10 x range.
      TVP  Trend-day VWAP pullback: after a 30-minute close beyond a band,
           the first 15-minute block that touches VWAP and closes back on the
           trend side; stop 0.15 x average range; one trade a day.
      RSL  Relative-strength leader (v2): at 10:00, z = (NQ's move from its
           open - 1.2 x the reference market's move) / NQ's average move at
           10:00. z >= 0.45 -> long MNQ, z <= -0.45 -> short; stop 0.15 x
           average range; one trade a day. NQ leading or lagging the S&P in
           the first half hour tended to keep doing so into the close.
      PYR  Pyramid (v2): when an NBM trade is ahead by 0.15 x average range at
           a later 30-minute mark and the close is still beyond the band, add
           one contract with its own stop 0.25 x average range; it leaves when
           the NBM trade ends. One add per NBM trade.
      ESL  Reference market leads (v2): at a 30-minute mark 10:00-15:00 the
           reference market has closed beyond ITS OWN noise band while NQ is
           still inside its band but on that side of VWAP -> enter MNQ that
           way; stop 0.20 x average range; one trade a day.
    RSL and ESL read a second chart: MES or ES, 5-minute bars, same time zone,
    same days loaded, its chart number in the input 'Reference chart'. They
    decide once the reference bar for the mark is final (its next bar has
    started). Without that chart those two legs make no signals.
    Noise band = max/min(open, prior close) x (1 +/- average |close/open - 1|
    for that time of day over the last 14 full sessions). All flat at 15:55
    New York. Rules, sources and measurements: README.md and the three
    single-strategy files (the engines here are the same code).

  HOW ONE ACCOUNT HOLDS THREE STRATEGIES
    L1  Each strategy owns one LEG: its own entry order with its own ATTACHED
        stop (Stop1Offset from the actual fill). A leg is followed by its own
        order IDs, never by the account position.
    L2  The account position must always equal the sum of the open legs. A
        difference that lasts 5 s of trade time flattens everything.
    L3  A leg may open only if no other leg is open or working in the OPPOSITE
        direction. Among NBM, GOD, TVP and PYR this never happened in
        2017-2024. RSL and ESL can disagree with an open leg: about 30 RSL
        and 75 ESL signals in 7 years were refused for that reason.
    L4  A leg ends on its attached stop, on the engine exit (NBM), or at the
        flat time. The engine exit of one leg while others stay open: that
        leg's stop is moved to 2 ticks behind the market and follows it until
        it fills (never an unprotected moment, no separate exit order). When
        it is the only leg, FlattenAndCancelAllOrders().
    L5  Day limits use the ACCOUNT's realised P&L since the first call of the
        trading day plus the open legs: daily loss halt (flattens), profit
        lock, losing legs in a row. 0 = off (#36). Reset on a new day (#43).
    L6  MaximumPositionAllowed = the enabled strategies' contracts added up.
    L7  Everything else follows the single-strategy files: one position read
        per call (#17), CHART CHECK and feed checks, late-signal limit, CME
        short sessions, flatten re-issue, settings written on every call.

  ORDER LIFECYCLE (per leg)
    SIGNAL -> ENTRY (market, or GOD's resting stop) -> FILL (parent order
    FILLED; fill price read from the order) -> ATTACHED STOP working (FILL
    VERIFY once) -> EXIT (stop filled / stop tightened by the engine exit /
    flatten) -> leg P&L logged, leg reset. Account P&L is Sierra's own.

  MEASURED (this file in the mock: NQ 1-minute data 2017-01 .. 2024-03 as 4
  ticks per bar, 1 MNQ per strategy, $2.50 per contract round trip)
    Six legs: 4,925 legs | net +$76,121 | $875 per month (median $588) | 61 of
    87 months positive | best month +$8,246, worst -$1,130 | max drawdown
    -$3,200 | worst day -$884 | 2017-2021 $588 per month, 2022-2024 $1,512.
    By leg ($ per month, 2017-21 / 2022-24): NBM 256 (195 / 393), PF 1.45 |
    GOD 123 (94 / 189), PF 1.49 | TVP 154 (101 / 273), PF 1.56 | RSL 154
    (89 / 297), PF 1.40 | PYR 108 (67 / 198), PF 1.47 | ESL 79 (42 / 162),
    PF 1.39. By year: 2017 +438, 2018 +8,292, 2019 -166, 2020 +10,257,
    2021 +16,483, 2022 +26,779, 2023 +10,590, 2024 (to March) +3,447.
    At $5.00 per round trip: $733 per month.
    The first three legs alone (v1): $527 per month, max drawdown -$2,144.
    Each leg switched on alone matches its independent Python model (same
    entry price on 97-99% of trades). Identical trades with 361 full
    recalculations injected and with every call doubled; called once a minute
    only, same fills and 1.5% less net (the tightened stop of L4).
    WEAKEST PARTS: ESL made little in 2017-2021 ($42 per month); GOD depends
    on the order of ticks in its entry minute and lost on ES; PYR adds size
    on the same days NBM wins, so it raises drawdown as well as profit.
    About 35 ideas were tried to find these six: expect less than measured.
    A mock is not Sierra. Nothing after 2024-03-07 has been tested.

  FIRST REPLAY CHECKS (unverified until seen in Sierra)
    0 Replay with the reference chart: "MSU RSL SIGNAL" lines appear at 10:00
      and show a plausible reference move; no "MSU REFERENCE CHART" warning.
      sc.GetChartBaseData / sc.GetChartDateTimeArray during a multi-chart
      Replay are NOT yet verified here.
    1 "FILL VERIFY ok" for a MARKET parent and for GOD's STOP parent.
    2 Two legs open at once: two working stops, position = 2.
    3 "ENGINE EXIT ... stop -> " lines move one stop only; the other leg's
      stop is untouched.
    4 s_SCTradeOrder::AvgFillPrice is non-zero for filled parents and stops
      (used for the per-leg P&L line only; the halts use the account P&L).
    5 Number of Open Trades = 0 at the end; every exit before 15:56.

  BACKTEST IN SIERRA (tick by tick, one account)
    MNQ continuous contract, 5-minute bars, New York time zone, 25+ days
    loaded before the start. For RSL / ESL: an MES (or ES) 5-minute chart in
    the same chartbook, same settings, and replay ALL charts of the chartbook
    together so both markets advance in step. Trade Simulation Mode ON, commission set (#55).
    Clear trade data (#53, #71). Chart >> Replay Chart, Accurate Trading
    System Back Test mode (#54). No other study may trade this symbol on the
    same Trade Account (#72); do not add the single-strategy studies as well.

  STORAGE: Persistent pointer 1 = Engine (heap, rebuilt on a full
    recalculation), pointer 2 = Trader (heap, kept). Freed on the last call.
  INSTALL: ACS_Source\MomentumSuite_NQ_MNQ.cpp, Build Custom Studies DLL,
    REMOVE a v1 study from the chart, add "Momentum Suite NQ MNQ v2" to an MNQ
    5-minute chart and set 'Reference chart' to the MES chart's number. Keep the Trade
    Window "Attached Orders" list EMPTY. Message Log lines start "MSU".
============================================================================*/

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>

#define STUDY_VERSION      "v2"
#define STUDY_BUILD_STAMP  __DATE__ " " __TIME__

namespace msu
{
    static inline int    IMax(int a, int b)       { return a > b ? a : b; }
    static inline int    IMin(int a, int b)       { return a < b ? a : b; }
    static inline double DMax(double a, double b) { return a > b ? a : b; }
    static inline double DMin(double a, double b) { return a < b ? a : b; }
    static inline double DAbs(double a)           { return a < 0.0 ? -a : a; }

    enum : int { DIR_BOTH = 0, DIR_LONG, DIR_SHORT };
    enum : int { MAXB = 400, MAXLOOK = 30 };
    enum : int { KIND_MARKET = 0, KIND_STOP = 1 };
    enum : int { S_NBM = 0, S_GOD, S_TVP, S_RSL, S_PYR, S_ESL, NS };
    static const char* const kStrat[NS] = { "NBM", "GOD", "TVP", "RSL", "PYR", "ESL" };

    enum InputIdx : int
    {
        IN_NBM_ON = 0, IN_NBM_QTY, IN_NBM_BAND, IN_NBM_CAT, IN_NBM_MAXTR,
        IN_GOD_ON, IN_GOD_QTY, IN_GOD_MINGAP, IN_GOD_STOP, IN_GOD_VALID, IN_GOD_CHASE,
        IN_TVP_ON, IN_TVP_QTY, IN_TVP_BAND, IN_TVP_STOP, IN_TVP_CONFIRM,
        IN_SESS_OPEN, IN_SESS_CLOSE, IN_EOD, IN_DIRECTIONS, IN_LOOKBACK, IN_TALLY_COST, IN_VERBOSE,
        IN_ORDER_MODE, IN_ALLOW_LIVE, IN_MAX_RISK_USD, IN_DAILY_LOSS, IN_PROFIT_LOCK, IN_MAX_CONSEC,
        IN_SAFETY, IN_INHERITED, IN_ENTRY_DELAY, IN_EARLY_CLOSE,
        // v2 (appended; an older placed study reads 0 = off / default for these, lessons #15, #64)
        IN_REF_CHART, IN_RSL_ON, IN_RSL_QTY, IN_RSL_K, IN_RSL_BETA, IN_RSL_STOP,
        IN_PYR_ON, IN_PYR_QTY, IN_PYR_PROFIT, IN_PYR_STOP, IN_ESL_ON, IN_ESL_QTY, IN_ESL_STOP
    };

    enum SubgraphIdx : int
    {
        SG_VWAP = 0, SG_UB, SG_LB, SG_TRIG, SG_CANCEL, SG_TREND,
        SG_LONG0, SG_LONG1, SG_LONG2, SG_LONG3, SG_LONG4, SG_LONG5, SG_SHORT0, SG_SHORT1, SG_SHORT2, SG_SHORT3, SG_SHORT4, SG_SHORT5,
        SG_ENGINE_END, SG_STOP0 = SG_ENGINE_END, SG_STOP1, SG_STOP2, SG_STOP3, SG_STOP4, SG_STOP5, NSG
    };

    struct Params
    {
        int    On[NS]     = { 1, 1, 1, 1, 1, 1 };
        int    Qty[NS]    = { 1, 1, 1, 1, 1, 1 };
        int    RefChart   = 0;                         // chart number of the MES / ES 5-minute chart (0 = none: RSL, ESL idle)
        double RslK       = 0.45;                      // min |NQ move - beta x ES move|, x the average NQ move at 10:00
        double RslBeta    = 1.2;
        double RslStop    = 0.15;                      // x average day range
        double PyrProfit  = 0.15;                      // NBM trade ahead by this, x average day range
        double PyrStop    = 0.25;
        double EslStop    = 0.20;
        double NbmBand    = 1.0;                       // noise band width, x the average move
        double NbmCat     = 0.5;                       // NBM disaster stop, x average day range
        int    NbmMaxTr   = 3;                         // NBM trades per day
        double GodMinGap  = 0.05;                      // x average day range
        double GodStop    = 0.10;
        int    GodValid   = 60;                        // minutes after the open
        int    GodChase   = 4;                         // ticks
        double TvpBand    = 1.0;
        double TvpStop    = 0.15;
        int    TvpConfirm = 3;                         // chart bars per confirmation block
        int    SessOpen   = 9 * 3600 + 30 * 60;
        int    SessClose  = 16 * 3600;
        int    EOD        = 15 * 3600 + 55 * 60;
        int    Directions = DIR_BOTH;
        int    Lookback   = 14;
        double TallyCost  = 1.25;                      // points per contract per round trip (tally only)
        int    Verbose    = 0;
        int    Fallbacks  = 0;
        // fixed by the measurements
        int    FirstMark() const    { return 10 * 3600; }
        int    NbmLastEntry() const { return 15 * 3600 + 30 * 60; }
        int    TvpLastEntry() const { return 15 * 3600; }
        int    GodOrSec() const     { return 300; }
        int    RslTime() const      { return 10 * 3600; }
        int    EslLastEntry() const { return 15 * 3600; }
        int    TotalQty() const     { int n = 0; for (int q = 0; q < NS; ++q) if (On[q]) n += Qty[q]; return n; }
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

    // CME equity-index halt on a trading day, New York time (0 = normal day); lessons #66.
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
        if (m == 11 && wd == 5 && d >= 23 && d <= 29) return H1315;
        if (m == 12 && d == 24 && wd >= 1 && wd <= 4) return H1315;
        return 0;
    }

    // ----------------------------------------------------------------- engine
    enum : int { EX_STOP = 0, EX_TRAIL, EX_EOD, EX_N };
    enum : int { VS_FLAT = 0, VS_PEND_MKT, VS_PEND_STOP, VS_OPEN };

    struct VTrade { int state, dir, seq, exitNext, expireTod; double px, stop, dist, trig, cancelPx; };

    struct Strat                                            // one strategy's day state, signal interface and tally
    {
        int     trend, tradesToday, dayDead;
        double  orh, orl;
        int     sigSeq, sigBar, sigDir, sigKind, sigExpireTod;
        int64_t sigKey, exitKey;
        double  sigTrig, sigStopDist, sigCancelPx;
        int     exitLong, exitShort;
        VTrade  vt;
        int     nTr, nWin, nEx[EX_N];
        double  sumPts, grossW, grossL;
    };

    struct Sess { float rng; float mv[MAXB]; };

    // The reference market's day session (MES / ES chart): open, prior close and its own noise profile.
    struct RefSess
    {
        int    cursor, curDate, barsSeen, lastB, lastTod, halt, ringN, ringHead, ok;
        double dOpen, dHigh, dLow, dClose, prevClose;
        float  mv[MAXB], sig[MAXB];
        Sess   ring[MAXLOOK];
    };

    struct Engine
    {
        int    lastCommitted, recalcOpen, bannerDone, tallyLogged, tickWarned, quiet, virtualLog;
        int    curDate, nb, dayOK, halt, barsSeen, lastB, sessions;
        double dOpen, dHigh, dLow, dClose, pv, vol, vwap, prevClose, adr, usdPerPt;
        float  mv[MAXB], sig[MAXB];
        Sess   ring[MAXLOOK];
        int    ringN, ringHead;
        Strat  s[NS];
        RefSess es;
        int     pyrFor, refPendBar, refMiss;
        int64_t refPendKey;
        void ResetAll()
        {
            std::memset(this, 0, sizeof(*this));
            lastCommitted = -1;
            for (int q = 0; q < NS; ++q) s[q].sigBar = -1;
            refPendBar = -1;
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
        ++S.nTr; ++S.nEx[how];
        S.sumPts += pts;
        if (pts > 0.0) { ++S.nWin; S.grossW += pts; } else S.grossL -= pts;
        if (P.Verbose || (!E.quiet && E.virtualLog))
        {
            static const char* const kExit[EX_N] = { "STOP", "TRAIL (band / VWAP)", "EOD" };
            char ts[48], msg[320];
            StampStr(ts, sizeof ts, f.Date(k), f.Tod(k));
            std::snprintf(msg, sizeof msg, "MSU %s virtual #%d %s exit %s at %.2f (%s) -> %+.2f pts after %.2f cost | entry %.2f",
                          kStrat[si], v.seq, DirName(v.dir), kExit[how], ex, ts, pts, P.TallyCost, v.px);
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
                          "MSU TALLY %s (diagnostic, not P&L: chart bars, 1 virtual contract, stop first on a shared bar, %.2f pt cost; "
                          "GOD is understated on 5-minute bars) | sessions %d | trades %d | winners %d (%.1f%%) | profit factor %.2f | net "
                          "%+.2f pts = $%.0f per contract | exits: stop %d, trail %d, EOD %d", kStrat[si], P.TallyCost, E.sessions, S.nTr,
                          S.nWin, S.nTr > 0 ? 100.0 * S.nWin / S.nTr : 0.0, S.grossL > 0.0 ? S.grossW / S.grossL : 0.0, S.sumPts,
                          S.sumPts * E.usdPerPt, S.nEx[EX_STOP], S.nEx[EX_TRAIL], S.nEx[EX_EOD]);
            f.Log(msg, 0);
        }
    }

    // Publish an entry signal to the order layer and open the engine's own pending trade.
    template <class F> static void Emit(F& f, Engine& E, const Params& P, int si, int k, int d, int kind, double trig, double dist,
                                        double cancelPx, int expireTod, const char* what)
    {
        Strat& S = E.s[si];
        const double tick = f.Tick();
        dist = DMax(RoundNear(dist, tick), 2.0 * tick);
        ++S.sigSeq; ++S.tradesToday;
        S.sigBar = k; S.sigDir = d; S.sigKind = kind; S.sigTrig = trig; S.sigStopDist = dist;
        S.sigCancelPx = cancelPx; S.sigExpireTod = expireTod;
        S.sigKey = (int64_t)f.Date(k) * 86400 + f.Tod(k);
        VTrade& v = S.vt;
        std::memset(&v, 0, sizeof v);
        v.state = kind == KIND_MARKET ? VS_PEND_MKT : VS_PEND_STOP;
        v.dir = d; v.seq = S.sigSeq; v.dist = dist; v.trig = trig; v.cancelPx = cancelPx; v.expireTod = expireTod;
        f.Out((d > 0 ? SG_LONG0 : SG_SHORT0) + si, k) = d > 0 ? (float)(f.L(k) - (8.0 + 6.0 * si) * tick) : (float)(f.H(k) + (8.0 + 6.0 * si) * tick);
        if (P.Verbose || !E.quiet)
        {
            char ts[48], msg[500];
            StampStr(ts, sizeof ts, f.Date(k), f.Tod(k));
            std::snprintf(msg, sizeof msg, "MSU %s SIGNAL #%d %s %s | %s | close %.2f | VWAP %.2f | open %.2f, prior close %.2f | avg range "
                          "%.2f | stop distance %.2f pts", kStrat[si], S.sigSeq, DirName(d), ts, what, f.C(k), E.vwap, E.dOpen, E.prevClose,
                          E.adr, dist);
            f.Log(msg, 0);
        }
    }

    // The engine's own trade of one strategy on bar k: fills at the open, stop first, flat at the EOD bar's open.
    template <class F> static void VirtualStep(F& f, Engine& E, const Params& P, int si, int k, int tod, bool flatBar)
    {
        VTrade& v = E.s[si].vt;
        bool stopFilledHere = false;
        if (v.state == VS_PEND_MKT)
        {
            if (flatBar) v.state = VS_FLAT;
            else { v.state = VS_OPEN; v.px = f.O(k); v.stop = v.px - v.dir * v.dist; v.exitNext = 0; }
        }
        else if (v.state == VS_PEND_STOP)
        {
            const bool hit = v.dir > 0 ? f.H(k) >= v.trig : f.L(k) <= v.trig;
            const bool opp = v.dir > 0 ? f.L(k) <= v.cancelPx : f.H(k) >= v.cancelPx;
            if (flatBar || tod >= v.expireTod || opp) v.state = VS_FLAT;
            else if (hit)
            {
                v.state = VS_OPEN; v.exitNext = 0; stopFilledHere = true;
                v.px = v.dir > 0 ? DMax(v.trig, f.O(k)) : DMin(v.trig, f.O(k));
                v.stop = v.px - v.dir * v.dist;
            }
        }
        else if (v.state == VS_OPEN && v.exitNext && !flatBar) CloseVirtual(f, E, P, si, EX_TRAIL, f.O(k), k);
        if (v.state == VS_OPEN)
        {
            if (flatBar) CloseVirtual(f, E, P, si, EX_EOD, f.O(k), k);
            else if (v.dir > 0 ? f.L(k) <= v.stop : f.H(k) >= v.stop)
                CloseVirtual(f, E, P, si, EX_STOP, stopFilledHere ? v.stop : (v.dir > 0 ? DMin(v.stop, f.O(k)) : DMax(v.stop, f.O(k))), k);
        }
    }

    // NBM: 30-minute marks; exit a long below max(upper, VWAP); enter flat above upper and VWAP. Mirror for shorts.
    template <class F> static void DecideNBM(F& f, Engine& E, const Params& P, int k, int tod, int bsec)
    {
        Strat& S = E.s[S_NBM];
        const int endT = tod + bsec;
        if ((endT - P.SessOpen) % 1800 != 0 || endT < P.FirstMark()) return;
        const int    b  = (tod - P.SessOpen) / bsec;
        const double c  = f.C(k), vw = E.vwap;
        const double ub = DMax(E.dOpen, E.prevClose) * (1.0 + P.NbmBand * E.sig[b]);
        const double lb = DMin(E.dOpen, E.prevClose) * (1.0 - P.NbmBand * E.sig[b]);
        S.exitKey   = (int64_t)f.Date(k) * 86400 + tod;
        S.exitLong  = c < DMax(ub, vw) ? 1 : 0;
        S.exitShort = c > DMin(lb, vw) ? 1 : 0;
        VTrade& v = S.vt;
        if (v.state == VS_OPEN) { if (v.dir > 0 ? S.exitLong : S.exitShort) v.exitNext = 1; return; }
        if (v.state != VS_FLAT || endT > P.NbmLastEntry()) return;
        if (P.NbmMaxTr > 0 && S.tradesToday >= P.NbmMaxTr) return;
        const int d = (c > ub && c > vw) ? 1 : ((c < lb && c < vw) ? -1 : 0);
        if (d == 0 || !DirOn(P, d)) return;
        Emit(f, E, P, S_NBM, k, d, KIND_MARKET, 0.0, P.NbmCat * E.adr, 0.0, 0, "30-minute close beyond the noise band and VWAP");
    }

    // GOD: when the first 5 minutes are complete, a stop entry beyond the range on the gap's side only.
    template <class F> static void DecideGOD(F& f, Engine& E, const Params& P, int k, int tod, int bsec)
    {
        Strat& S = E.s[S_GOD];
        const double tick = f.Tick();
        VTrade& v = S.vt;
        if (v.state == VS_PEND_STOP) { f.Out(SG_TRIG, k) = (float)v.trig; f.Out(SG_CANCEL, k) = (float)v.cancelPx; }
        if (S.dayDead) return;
        const int endT = tod + bsec, orEnd = P.SessOpen + P.GodOrSec();
        if (tod == P.SessOpen || S.orh == 0.0) { S.orh = f.H(k); S.orl = f.L(k); }
        else if (tod < orEnd) { S.orh = DMax(S.orh, f.H(k)); S.orl = DMin(S.orl, f.L(k)); }
        if (endT < orEnd) return;
        S.dayDead = 1;                                                          // one decision per day
        if (endT != orEnd || E.barsSeen * bsec != P.GodOrSec()) return;         // an opening-range bar is missing
        const double gap = (E.dOpen - E.prevClose) / E.adr;
        if (DAbs(gap) < P.GodMinGap) return;
        const int d = gap > 0.0 ? 1 : -1;
        if (!DirOn(P, d) || v.state != VS_FLAT) return;
        const double trig   = d > 0 ? S.orh + tick : S.orl - tick;
        const double cancel = d > 0 ? S.orl - tick : S.orh + tick;
        char what[160];
        std::snprintf(what, sizeof what, "gap %+.2f x avg range, opening range %.2f-%.2f: stop entry %.2f, void through %.2f",
                      gap, S.orl, S.orh, trig, cancel);
        Emit(f, E, P, S_GOD, k, d, KIND_STOP, trig, P.GodStop * E.adr, cancel, P.SessOpen + P.GodValid * 60, what);
        f.Out(SG_TRIG, k) = (float)trig; f.Out(SG_CANCEL, k) = (float)cancel;
    }

    // TVP: trend from 30-minute closes beyond a band; entry on a confirmation block that touched VWAP and closed back.
    template <class F> static void DecideTVP(F& f, Engine& E, const Params& P, int k, int tod, int bsec)
    {
        Strat& S = E.s[S_TVP];
        const int endT = tod + bsec;
        if (endT < P.FirstMark()) return;
        const int    b = (tod - P.SessOpen) / bsec;
        const double c = f.C(k), vw = E.vwap;
        if ((endT - P.SessOpen) % 1800 == 0)
        {
            if (c > DMax(E.dOpen, E.prevClose) * (1.0 + P.TvpBand * E.sig[b])) S.trend = 1;
            else if (c < DMin(E.dOpen, E.prevClose) * (1.0 - P.TvpBand * E.sig[b])) S.trend = -1;
        }
        f.Out(SG_TREND, k) = (float)S.trend;
        if (S.trend == 0 || (endT - P.SessOpen) % (P.TvpConfirm * bsec) != 0 || endT > P.TvpLastEntry()) return;
        if (S.vt.state != VS_FLAT || S.tradesToday >= 1 || b < P.TvpConfirm - 1) return;
        double lo = f.L(k), hi = f.H(k);
        for (int q = 1; q < P.TvpConfirm && k - q >= 0; ++q)
        {
            if (f.Date(k - q) != f.Date(k) || f.Tod(k - q) < P.SessOpen) break;
            lo = DMin(lo, f.L(k - q)); hi = DMax(hi, f.H(k - q));
        }
        const int d = (S.trend > 0 && lo <= vw && c > vw) ? 1 : ((S.trend < 0 && hi >= vw && c < vw) ? -1 : 0);
        if (d == 0 || !DirOn(P, d)) return;
        Emit(f, E, P, S_TVP, k, d, KIND_MARKET, 0.0, P.TvpStop * E.adr, 0.0, 0, "trend day, pullback to VWAP rejected");
    }

    // PYR: one add-on contract to an NBM trade that is ahead and still beyond its band at a later 30-minute mark.
    // It leaves with the NBM trade (or on its own stop).
    template <class F> static void DecidePYR(F& f, Engine& E, const Params& P, int k, int tod, int bsec)
    {
        Strat& S = E.s[S_PYR];
        const VTrade& n = E.s[S_NBM].vt;
        VTrade& v = S.vt;
        const bool nbmOn = n.state == VS_OPEN && !n.exitNext;
        if (v.state != VS_FLAT && !nbmOn)                               // the NBM trade is over: leave with it
        {
            if (v.state == VS_OPEN) v.exitNext = 1; else v.state = VS_FLAT;
            S.exitKey = (int64_t)f.Date(k) * 86400 + tod; S.exitLong = S.exitShort = 1;
            return;
        }
        const int endT = tod + bsec;
        if (v.state != VS_FLAT || !nbmOn || E.pyrFor == n.seq) return;
        if ((endT - P.SessOpen) % 1800 != 0 || endT > P.NbmLastEntry()) return;
        const int    b  = (tod - P.SessOpen) / bsec;
        const double c  = f.C(k);
        const double ub = DMax(E.dOpen, E.prevClose) * (1.0 + P.NbmBand * E.sig[b]);
        const double lb = DMin(E.dOpen, E.prevClose) * (1.0 - P.NbmBand * E.sig[b]);
        if (!(n.dir * (c - n.px) >= P.PyrProfit * E.adr) || !(n.dir > 0 ? c > ub : c < lb) || !DirOn(P, n.dir)) return;
        E.pyrFor = n.seq;
        S.exitLong = S.exitShort = 0;
        Emit(f, E, P, S_PYR, k, n.dir, KIND_MARKET, 0.0, P.PyrStop * E.adr, 0.0, 0, "NBM trade ahead and still beyond the band: add one");
    }

    // Feed the reference chart's FINAL bars (a later bar exists) up to and including time key 'upTo'.
    template <class F> static void FeedRef(F& f, Engine& E, const Params& P, int64_t upTo)
    {
        RefSess& R = E.es;
        const int n = f.RefSize(), bsec = f.BarSec() > 0 ? f.BarSec() : 300;
        while (R.cursor < n - 1)
        {
            const int j = R.cursor, date = f.RefDate(j), tod = f.RefTod(j);
            if ((int64_t)date * 86400 + tod > upTo) break;
            ++R.cursor;
            if (tod < P.SessOpen || tod >= P.SessClose) continue;
            const int nb = IMin((P.SessClose - P.SessOpen) / bsec, MAXB);
            if (date != R.curDate)
            {
                if (R.curDate != 0)
                {
                    if (R.barsSeen >= nb - 2 && !R.halt && R.dOpen > 0.0)
                    {
                        for (int b = R.lastB + 1; b < nb; ++b) R.mv[b] = R.mv[R.lastB];
                        Sess& s = R.ring[R.ringHead];
                        s.rng = (float)(R.dHigh - R.dLow);
                        std::memcpy(s.mv, R.mv, sizeof s.mv);
                        R.ringHead = (R.ringHead + 1) % MAXLOOK;
                        if (R.ringN < MAXLOOK) ++R.ringN;
                    }
                    R.prevClose = R.barsSeen * 100 >= nb * 35 ? R.dClose : 0.0;
                }
                R.curDate = date; R.halt = ExchangeHaltTod(date); R.barsSeen = 0; R.lastB = -1;
                R.dOpen = f.RefO(j); R.dHigh = f.RefH(j); R.dLow = f.RefL(j);
                std::memset(R.mv, 0, sizeof R.mv);
                R.ok = 0;
                if (R.ringN >= P.Lookback)
                {
                    for (int b = 0; b < nb; ++b) R.sig[b] = 0.0f;
                    for (int q = 1; q <= P.Lookback; ++q)
                    {
                        const Sess& s = R.ring[(R.ringHead - q + 2 * MAXLOOK) % MAXLOOK];
                        for (int b = 0; b < nb; ++b) R.sig[b] += s.mv[b];
                    }
                    for (int b = 0; b < nb; ++b) R.sig[b] /= (float)P.Lookback;
                    R.ok = (R.prevClose > 0.0 && !R.halt && R.dOpen > 0.0) ? 1 : 0;
                }
            }
            const int b = (tod - P.SessOpen) / bsec;
            if (b < 0 || b >= nb) continue;
            if (R.lastB >= 0) for (int q = R.lastB + 1; q < b; ++q) R.mv[q] = R.mv[R.lastB];
            R.dHigh = DMax(R.dHigh, f.RefH(j)); R.dLow = DMin(R.dLow, f.RefL(j)); R.dClose = f.RefC(j);
            R.mv[b] = (float)DAbs(f.RefC(j) / R.dOpen - 1.0);
            R.lastB = b; R.lastTod = tod; ++R.barsSeen;
        }
    }

    // RSL and ESL decide on the reference market's bar for the same time, once that bar is final.
    // Called on every study call until the pending decision is made or its next bar has closed.
    template <class F> static void RefDecide(F& f, Engine& E, const Params& P, int lastClosed)
    {
        if (E.refPendBar < 0) return;
        const int k = E.refPendBar, tod = f.Tod(k), date = f.Date(k);
        const int bsec = f.BarSec() > 0 ? f.BarSec() : 300;
        RefSess& R = E.es;
        FeedRef(f, E, P, E.refPendKey);
        const bool have = R.curDate == date && R.lastTod == tod && R.cursor > 0 &&
                          (int64_t)f.RefDate(R.cursor - 1) * 86400 + f.RefTod(R.cursor - 1) == E.refPendKey;
        if (!have)
        {
            if (lastClosed > k) { E.refPendBar = -1; ++E.refMiss; }     // its bar never came: no decision for this mark
            return;
        }
        E.refPendBar = -1;
        const int    b    = (tod - P.SessOpen) / bsec, endT = tod + bsec;
        const double c    = f.C(k);
        const bool   sync = R.dOpen > 0.0 && R.barsSeen == E.barsSeen;   // both markets have every bar of the session so far
        if (!E.dayOK || !sync) return;
        // RSL: at 10:00, NQ's move from its open against beta x the reference market's move
        if (endT == P.RslTime() && E.s[S_RSL].vt.state == VS_FLAT && E.s[S_RSL].tradesToday == 0 && E.sig[b] > 0.0f)
        {
            const double z = ((c / E.dOpen - 1.0) - P.RslBeta * (R.dClose / R.dOpen - 1.0)) / E.sig[b];
            const int    d = z >= P.RslK ? 1 : (z <= -P.RslK ? -1 : 0);
            if (d != 0 && DirOn(P, d))
            {
                char what[140];
                std::snprintf(what, sizeof what, "relative strength %+.2f (NQ %+.3f%%, reference %+.3f%% x %.1f)", z,
                              100.0 * (c / E.dOpen - 1.0), 100.0 * (R.dClose / R.dOpen - 1.0), P.RslBeta);
                Emit(f, E, P, S_RSL, k, d, KIND_MARKET, 0.0, P.RslStop * E.adr, 0.0, 0, what);
            }
        }
        // ESL: the reference market has closed beyond ITS noise band, NQ is still inside its own but on the right side of VWAP
        if (R.ok && endT >= P.FirstMark() && endT <= P.EslLastEntry() && E.s[S_ESL].vt.state == VS_FLAT && E.s[S_ESL].tradesToday == 0)
        {
            const double eu = DMax(R.dOpen, R.prevClose) * (1.0 + R.sig[b]), el = DMin(R.dOpen, R.prevClose) * (1.0 - R.sig[b]);
            const double ub = DMax(E.dOpen, E.prevClose) * (1.0 + P.NbmBand * E.sig[b]);
            const double lb = DMin(E.dOpen, E.prevClose) * (1.0 - P.NbmBand * E.sig[b]);
            const int d = (R.dClose > eu && c <= ub && c > E.vwap) ? 1 : ((R.dClose < el && c >= lb && c < E.vwap) ? -1 : 0);
            if (d != 0 && DirOn(P, d))
                Emit(f, E, P, S_ESL, k, d, KIND_MARKET, 0.0, P.EslStop * E.adr, 0.0, 0, "reference market beyond its band, NQ not yet");
        }
    }

    // ------------------------------------------------------------ bar driver
    template <class F> static void ProcessBar(F& f, Engine& E, const Params& P, int k)
    {
        const int date = f.Date(k), tod = f.Tod(k);
        const int bsec = f.BarSec() > 0 ? f.BarSec() : 300;
        for (int g = 0; g < SG_ENGINE_END; ++g) f.Out(g, k) = 0.0f;
        if (tod < P.SessOpen || tod >= P.SessClose) return;            // only the day session is used

        // (1) a new day session: bank yesterday, average the last Lookback full sessions
        if (date != E.curDate)
        {
            for (int si = 0; si < NS; ++si)
            {
                if (E.s[si].vt.state == VS_OPEN) CloseVirtual(f, E, P, si, EX_EOD, E.dClose, k);
                E.s[si].vt.state = VS_FLAT;
                E.s[si].trend = 0; E.s[si].tradesToday = 0; E.s[si].dayDead = 0; E.s[si].orh = 0.0; E.s[si].orl = 0.0;
                E.s[si].exitLong = E.s[si].exitShort = 0;
            }
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
            E.curDate = date; ++E.sessions; E.pyrFor = 0;
            E.nb = IMin((P.SessClose - P.SessOpen) / bsec, MAXB);
            E.halt = ExchangeHaltTod(date);
            E.barsSeen = 0; E.lastB = -1;
            E.dOpen = f.O(k); E.dHigh = f.H(k); E.dLow = f.L(k); E.dClose = f.C(k); E.pv = 0.0; E.vol = 0.0; E.vwap = 0.0;
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
            E.dayOK = (E.adr > 0.0 && E.prevClose > 0.0 && !E.halt && E.dOpen > 0.0) ? 1 : 0;
        }
        const int b = (tod - P.SessOpen) / bsec;
        if (b < 0 || b >= E.nb) return;
        const bool flatBar = tod >= P.EOD;

        // (2) the engines' own trades on this bar
        for (int si = 0; si < NS; ++si) VirtualStep(f, E, P, si, k, tod, flatBar);

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
        if (E.dayOK)
        {
            f.Out(SG_UB, k) = (float)(DMax(E.dOpen, E.prevClose) * (1.0 + P.NbmBand * E.sig[b]));
            f.Out(SG_LB, k) = (float)(DMin(E.dOpen, E.prevClose) * (1.0 - P.NbmBand * E.sig[b]));
        }

        // (4) decisions at this close (a switched-off strategy still runs its tally)
        if (E.dayOK && !flatBar)
        {
            DecideNBM(f, E, P, k, tod, bsec);
            DecideGOD(f, E, P, k, tod, bsec);
            DecideTVP(f, E, P, k, tod, bsec);
            DecidePYR(f, E, P, k, tod, bsec);
            if (P.RefChart > 0 && (tod + bsec - P.SessOpen) % 1800 == 0 && tod + bsec >= P.FirstMark())
            {
                E.refPendBar = k; E.refPendKey = (int64_t)date * 86400 + tod;   // RSL / ESL: decided by RefDecide()
            }
        }
        if (P.RefChart > 0) FeedRef(f, E, P, (int64_t)date * 86400 + tod - 1);   // keep the reference session current
    }
}   // namespace msu

#ifndef MSU_CORE_ONLY
namespace msu
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
        // the reference chart (MES / ES), fetched once per study call
        SCGraphData     ref;
        SCDateTimeArray refDT;
        int             refN = 0;
        void   LoadRef(int chart)
        {
            refN = 0;
            if (chart <= 0 || chart == sc.ChartNumber) return;
            sc.GetChartBaseData(chart, ref);
            sc.GetChartDateTimeArray(chart, refDT);
            refN = IMin(ref[SC_LAST].GetArraySize(), refDT.GetArraySize());
        }
        int    RefSize() const     { return refN; }
        int    RefDate(int j)      { return refDT.DateAt(j); }
        int    RefTod(int j)       { return refDT.TimeAt(j); }
        double RefO(int j)         { return ref[SC_OPEN][j]; }
        double RefH(int j)         { return ref[SC_HIGH][j]; }
        double RefL(int j)         { return ref[SC_LOW][j]; }
        double RefC(int j)         { return ref[SC_LAST][j]; }
    };

    struct OParams
    {
        int    OrderMode  = 0;        // 0 = send orders, 1 = signals only
        int    AllowLive  = 0;
        double MaxRiskUSD = 0.0;      // per leg, 0 = off
        double DailyLoss  = 0.0;      // $, account, 0 = off
        double ProfitLock = 0.0;      // $, account, 0 = off
        int    MaxConsec  = 0;        // losing legs in a row today, 0 = off
        int    Safety     = 0;        // 0 = block, 1 = warn only
        int    Inherited  = 0;        // 0 = flatten, 1 = leave alone
        int    EntryDelay = 120;      // s; input 0 = 120, -1 = off
        int    EarlyClose = 0;        // 0 = honour CME short sessions, 1 = ignore
        int    Fallbacks  = 0;
    };

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
        P.On[S_NBM]  = sc.Input[IN_NBM_ON].GetYesNo() ? 1 : 0;
        P.On[S_GOD]  = sc.Input[IN_GOD_ON].GetYesNo() ? 1 : 0;
        P.On[S_TVP]  = sc.Input[IN_TVP_ON].GetYesNo() ? 1 : 0;
        P.Qty[S_NBM] = R.GI(IN_NBM_QTY, 1, 100, D.Qty[S_NBM]);
        P.Qty[S_GOD] = R.GI(IN_GOD_QTY, 1, 100, D.Qty[S_GOD]);
        P.Qty[S_TVP] = R.GI(IN_TVP_QTY, 1, 100, D.Qty[S_TVP]);
        P.NbmBand    = R.GF(IN_NBM_BAND, 0.1, 10.0, D.NbmBand);
        P.NbmCat     = R.GF(IN_NBM_CAT, 0.05, 5.0, D.NbmCat);
        P.NbmMaxTr   = R.GI(IN_NBM_MAXTR, 0, 50, D.NbmMaxTr);
        P.GodMinGap  = R.GF(IN_GOD_MINGAP, 0.0, 5.0, D.GodMinGap);
        P.GodStop    = R.GF(IN_GOD_STOP, 0.01, 5.0, D.GodStop);
        P.GodValid   = R.GI(IN_GOD_VALID, 0, 390, 0);
        if (P.GodValid == 0) P.GodValid = D.GodValid;              // 0 = the default (lessons #64)
        if (P.GodValid <= 5) { ++P.Fallbacks; P.GodValid = D.GodValid; }
        P.GodChase   = R.GI(IN_GOD_CHASE, 0, 100, D.GodChase);
        P.TvpBand    = R.GF(IN_TVP_BAND, 0.1, 10.0, D.TvpBand);
        P.TvpStop    = R.GF(IN_TVP_STOP, 0.01, 5.0, D.TvpStop);
        P.TvpConfirm = R.GI(IN_TVP_CONFIRM, 1, 12, D.TvpConfirm);
        P.SessOpen   = R.GT(IN_SESS_OPEN, D.SessOpen);
        P.SessClose  = R.GT(IN_SESS_CLOSE, D.SessClose);
        P.EOD        = R.GT(IN_EOD, D.EOD);
        if (!(P.SessOpen < P.EOD && P.EOD < P.SessClose)) { ++P.Fallbacks; P.SessOpen = D.SessOpen; P.SessClose = D.SessClose; P.EOD = D.EOD; }
        P.Directions = R.GX(IN_DIRECTIONS, 3, D.Directions);
        P.Lookback   = R.GI(IN_LOOKBACK, 3, MAXLOOK, D.Lookback);
        P.TallyCost  = R.GF(IN_TALLY_COST, 0.0, 50.0, D.TallyCost);
        P.Verbose    = sc.Input[IN_VERBOSE].GetYesNo() ? 1 : 0;
        // v2 inputs: on a study placed before v2 they read 0 -> the three new legs stay off and the numbers take their defaults
        P.RefChart   = R.GI(IN_REF_CHART, 0, 5000, 0);
        P.On[S_RSL]  = sc.Input[IN_RSL_ON].GetYesNo() ? 1 : 0;
        P.On[S_PYR]  = sc.Input[IN_PYR_ON].GetYesNo() ? 1 : 0;
        P.On[S_ESL]  = sc.Input[IN_ESL_ON].GetYesNo() ? 1 : 0;
        auto Q0 = [&](int idx, int def) { const int v = sc.Input[idx].GetInt(); return v == 0 ? def : R.GI(idx, 1, 100, def); };
        auto F0 = [&](int idx, double lo, double hi, double def) { const double v = (double)sc.Input[idx].GetFloat(); return v == 0.0 ? def : R.GF(idx, lo, hi, def); };
        P.Qty[S_RSL] = Q0(IN_RSL_QTY, D.Qty[S_RSL]);
        P.Qty[S_PYR] = Q0(IN_PYR_QTY, D.Qty[S_PYR]);
        P.Qty[S_ESL] = Q0(IN_ESL_QTY, D.Qty[S_ESL]);
        P.RslK       = F0(IN_RSL_K, 0.05, 10.0, D.RslK);
        P.RslBeta    = F0(IN_RSL_BETA, 0.2, 5.0, D.RslBeta);
        P.RslStop    = F0(IN_RSL_STOP, 0.01, 5.0, D.RslStop);
        P.PyrProfit  = F0(IN_PYR_PROFIT, 0.01, 5.0, D.PyrProfit);
        P.PyrStop    = F0(IN_PYR_STOP, 0.01, 5.0, D.PyrStop);
        P.EslStop    = F0(IN_ESL_STOP, 0.01, 5.0, D.EslStop);
        return P;
    }

    static OParams ReadOParams(SCStudyInterfaceRef sc)
    {
        OParams O;
        const OParams D;
        InputReader R{ sc, O.Fallbacks };
        O.OrderMode  = R.GX(IN_ORDER_MODE, 2, D.OrderMode);
        O.AllowLive  = sc.Input[IN_ALLOW_LIVE].GetYesNo() ? 1 : 0;
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

    enum : int { LG_FLAT = 0, LG_PENDING = 1, LG_OPEN = 2 };
    enum : int
    {
        FW_NONE = 0, FW_EOD, FW_ENGINE, FW_UNPROTECTED, FW_WRONG_SIDE, FW_MISMATCH, FW_REISSUE, FW_INHERITED, FW_DAILY_LOSS,
        FW_STOP_MOVE, FW_COUNT
    };
    static const char* const kFlatWhy[FW_COUNT] =
    {
        "-", "flat time", "engine exit, no other leg staying", "a leg's stop is not working (#14)", "attached stop on the wrong side of the fill",
        "account position differs from the open legs", "flatten re-issued", "untracked position outside trading hours",
        "daily loss limit reached", "a leg's stop could not be tightened for its engine exit"
    };
    static const int kTraderMagic = 0x4D535531;

    struct Leg                                              // one strategy's order pair; time keys in trade-time seconds
    {
        int     state, dir, sigSeq, kind, qty, canceling, exiting, expireTod, fillVerified, covShort;
        double  entryRef, dist, cancelPx, entryFill;
        int64_t parentID, stopID, submitAbs, submitBarKey, fillAbs, cancelAbs, covShortAbs;
        char    cancelWhy[80];
    };

    // Order-layer state. Heap, PersistentPointer 2. Survives full recalculations (#38, #43);
    // it holds order IDs and time keys only, never bar indices (#37).
    struct Trader
    {
        int     magic;
        Leg     leg[NS];
        int     sigSeqDone[NS];
        int64_t sigKeyDone[NS], exitKeyDone[NS];
        int     flattenPending, flattenWhy, flattenCount, mismatch;
        int64_t flattenAbs, mismatchAbs, staleCancelAbs;
        double  flattenPx;
        int     postRecalc, flatSeen, inheritedWarned, liveBlockWarned, bannerDone, recalcLogged;
        int     cfgDone, cfgFail, tzState, feedWarnDay, clockWarned, refWarned;
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
                sc.AddMessageToLog("MSU ORDERS: full recalculation complete -- orders are sent only on real-time, Chart Replay or "
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
            const int lt = sc.LatestDateTimeForLastBar.GetTime();
            if (lt > nowTod) nowTod = lt;
        }
        const int     nowTD    = TradeDate(barDate, nowTod);
        const int64_t nowAbs   = (int64_t)barDate * 86400 + nowTod;
        const int64_t barKey   = (int64_t)barDate * 86400 + barTod;
        const double  lastPx   = sc.Close[nowBar];
        const double  usdPerPt = sc.CurrencyValuePerTick > 0.0f ? (double)sc.CurrencyValuePerTick / tick : 0.0;
        const bool    engOK    = E.lastCommitted >= nowBar - 1;
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

        // ---- data feed, real time only
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
                if (rawLag > 90 && rawLag <= 3600) delayed = true;             // a delayed feed (#65)
                else if ((rawLag > 3600 || rawLag < -3600) && !T.clockWarned)
                {
                    T.clockWarned = 1;
                    std::snprintf(msg, sizeof msg, "MSU FEED: system time and last-trade time differ by %d s -- not comparable; the "
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
            if (feedDown) std::snprintf(msg, sizeof msg, "MSU FEED: data feed not connected (state %d) -- %s.", sc.ServerConnectionState,
                                        blockSafety ? "no new entries; open legs keep their attached stops" : "warn only");
            else std::snprintf(msg, sizeof msg, "MSU FEED: the last trade is %d s behind the system clock -- the data looks DELAYED. %s", rawLag,
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

        // A leg has ended: log its P&L from the order fill prices (an estimate; the account P&L is Sierra's) and reset it.
        auto CloseLeg = [&](int s, double exitPx, const char* how)
        {
            Leg& g = T.leg[s];
            const double pl = g.dir * (exitPx - g.entryFill) * usdPerPt * g.qty;
            ++T.runLegs; ++T.legsToday;
            if (pl > 0.0) { ++T.runWins; ++T.winsToday; T.consecLosses = 0; } else { ++T.lossesToday; ++T.consecLosses; }
            T.runPL += pl; T.legPLToday += pl;
            std::snprintf(msg, sizeof msg, "MSU %s LEG CLOSED (%s) %s signal #%d | %d contract(s) %.2f -> %.2f | P&L ~%.2f before costs "
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
            std::snprintf(msg, sizeof msg, "MSU FLATTEN (%s) -- FlattenAndCancelAllOrders()=%d; legs are closed once the broker reports flat.",
                          kFlatWhy[why > FW_NONE && why < FW_COUNT ? why : FW_NONE], r);
            sc.AddMessageToLog(msg, (why == FW_EOD || why == FW_ENGINE) ? 0 : 1);
        };

        auto CancelEntry = [&](int s, const char* why)               // one leg's resting / unfilled parent only
        {
            Leg& g = T.leg[s];
            const int r = sc.CancelOrder(static_cast<uint64_t>(g.parentID));
            g.canceling = 1; g.cancelAbs = nowAbs;
            std::snprintf(g.cancelWhy, sizeof g.cancelWhy, "%s", why);
            std::snprintf(msg, sizeof msg, "MSU %s ENTRY CANCEL signal #%d (%s) -- CancelOrder()=%d; a fill that wins the race is kept "
                          "with its attached stop.", kStrat[s], g.sigSeq, why, r);
            sc.AddMessageToLog(msg, 0);
        };

        // A resting stop entry is void once price has traded through the other side: bar extremes since
        // the order went in are read, because the study is not called on every trade (lessons #76).
        auto OppositeBroke = [&](const Leg& g) -> bool
        {
            if (g.dir * (lastPx - g.cancelPx) <= 0.0) return true;
            for (int k = nowBar; k >= nowBar - 1 && k >= 0; --k)
            {
                if ((int64_t)sc.BaseDateTimeIn.DateAt(k) * 86400 + sc.BaseDateTimeIn.TimeAt(k) < g.submitBarKey) break;
                if (g.dir * ((g.dir > 0 ? (double)sc.Low[k] : (double)sc.High[k]) - g.cancelPx) <= 0.0) return true;
            }
            return false;
        };

        // Verify-then-modify, improve-only. 1 moved / pending, 0 nothing to do, -1 failed.
        auto TightenStop = [&](int s) -> int
        {
            Leg& g = T.leg[s];
            s_SCTradeOrder o;
            if (!ReadOrder(g.stopID, o) || !IsWorkingOrderStatusIgnorePendingChildren(o.OrderStatusCode)) return 0;
            const double want = sc.RoundToTickSize(lastPx - g.dir * 2.0 * tick);
            if (g.dir * (want - o.Price1) < 0.5 * tick) return 0;                                    // not an improvement
            if (o.LastModifyPrice1 != 0.0 && DAbs(want - o.LastModifyPrice1) < 0.5 * tick) return 1; // already pending
            s_SCNewOrder mod;
            mod.InternalOrderID = o.InternalOrderID;
            mod.Price1          = want;
            mod.OrderQuantity   = 0;                                                                 // price only
            const int res = sc.ModifyOrder(mod);
            if (P.Verbose || res <= 0 || g.exiting == 1)
            {
                std::snprintf(msg, sizeof msg, "MSU %s ENGINE EXIT signal #%d -- stop %.2f -> %.2f, 2 ticks behind the market "
                              "(ModifyOrder=%d); it follows the price until it fills.", kStrat[s], g.sigSeq, o.Price1, want, res);
                sc.AddMessageToLog(msg, res > 0 ? 0 : 1);
            }
            g.exiting = 2;
            return res > 0 ? 1 : -1;
        };

        char skip[320]; skip[0] = 0;
        auto Submit = [&](int s) -> bool
        {
            const Strat& S = E.s[s];
            const int    d    = S.sigDir;
            const double dist = DMax(sc.RoundToTickSize(S.sigStopDist), 2.0 * tick);
            if (usdPerPt <= 0.0) { std::snprintf(skip, sizeof skip, "$ per point unknown (check the symbol settings)"); return false; }
            int qty = P.Qty[s];
            if (O.MaxRiskUSD > 0.0)
            {
                while (qty > 1 && dist * usdPerPt * qty > O.MaxRiskUSD + 1e-9) --qty;
                if (dist * usdPerPt * qty > O.MaxRiskUSD + 1e-9)
                { std::snprintf(skip, sizeof skip, "risk $%.2f for 1 contract is over the $%.2f cap", dist * usdPerPt, O.MaxRiskUSD); return false; }
            }
            int    kind = S.sigKind;
            double trig = sc.RoundToTickSize(S.sigTrig);
            if (kind == KIND_STOP)
            {
                if (d * (lastPx - S.sigCancelPx) <= 0.0)
                { std::snprintf(skip, sizeof skip, "price %.2f is already through the opposite side %.2f", lastPx, S.sigCancelPx); return false; }
                const double through = d * (lastPx - trig);                 // price may open beyond the trigger (#42)
                if (through >= 0.0)
                {
                    if (through > P.GodChase * tick + 1e-9)
                    { std::snprintf(skip, sizeof skip, "price %.2f already %.2f pts beyond the trigger %.2f (chase allowance %d ticks)",
                                    lastPx, through, trig, P.GodChase); return false; }
                    kind = KIND_MARKET;
                }
            }
            s_SCNewOrder o;                                                 // a fresh struct per leg
            o.OrderType              = kind == KIND_STOP ? SCT_ORDERTYPE_STOP : SCT_ORDERTYPE_MARKET;
            if (kind == KIND_STOP) o.Price1 = trig;
            o.OrderQuantity          = qty;
            o.TimeInForce            = SCT_TIF_DAY;
            o.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP;
            o.Stop1Offset            = dist;                                // from the actual fill (#20)
            SCString tag;
            tag.Format("MSU %s #%d", kStrat[s], S.sigSeq);
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
            g.state = LG_PENDING; g.dir = d; g.sigSeq = S.sigSeq; g.kind = kind; g.qty = qty; g.dist = dist;
            g.entryRef = kind == KIND_STOP ? trig : lastPx; g.cancelPx = S.sigCancelPx; g.expireTod = S.sigExpireTod;
            g.parentID = o.InternalOrderID; g.stopID = o.Stop1InternalOrderID;
            g.submitAbs = nowAbs; g.submitBarKey = barKey;
            if (g.stopID == 0)
                sc.AddMessageToLog("MSU: entry accepted but NO attached stop ID returned -- flattened if no stop works after the fill (#14).", 1);
            std::snprintf(msg, sizeof msg, "MSU %s ORDER %s signal #%d | %s x%d at %s%.2f | attached stop offset %.2f pts (stop ~%.2f) | "
                          "$risk %.2f", kStrat[s], DirName(d), S.sigSeq, kind == KIND_STOP ? "STOP entry" : "MARKET", qty,
                          kind == KIND_STOP ? "" : "~", g.entryRef, dist, g.entryRef - d * dist, dist * usdPerPt * qty);
            sc.AddMessageToLog(msg, 0);
            return true;
        };

        // ---------------------------------------------------------- banner
        if (!T.bannerDone)
        {
            T.bannerDone = 1;
            const bool sim = sc.GlobalTradeSimulationIsOn != 0;
            const bool off = O.OrderMode != 0;
            std::snprintf(msg, sizeof msg, "MSU ORDERS %s | Trade Simulation Mode %s -> SendOrdersToTradeService=%d%s",
                          off ? "OFF -- signals only, NO ORDERS ARE SENT" : "SEND", sim ? "ON" : "OFF", (int)sc.SendOrdersToTradeService,
                          off ? "" : (liveBlocked ? " | LIVE ENTRIES BLOCKED: Sim is OFF and 'Allow LIVE orders' = No"
                                                  : (!sim ? " | *** LIVE ORDERS ENABLED ***" : "")));
            sc.AddMessageToLog(msg, (off || liveBlocked || !sim) ? 1 : 0);
            HHMM(hm, sizeof hm, P.EOD);
            char legs[160]; legs[0] = 0;
            for (int s = 0; s < NS; ++s)
            {
                const size_t n = std::strlen(legs);
                std::snprintf(legs + n, sizeof legs - n, "%s%s %s x%d", n ? " | " : "", kStrat[s], P.On[s] ? "ON" : "off", P.Qty[s]);
            }
            std::snprintf(msg, sizeof msg, "MSU plan: %s | reference chart #%d | MaximumPositionAllowed %d | one account, one leg "
                          "per strategy, each with its own attached stop | max $ risk per leg %s | daily loss halt %s | daily profit lock "
                          "%s | max losing legs in a row %s | flat time %s | %s $/pt %.2f%s",
                          legs, P.RefChart, P.TotalQty(),
                          O.MaxRiskUSD > 0.0 ? "ON" : "OFF", O.DailyLoss > 0.0 ? "ON" : "OFF", O.ProfitLock > 0.0 ? "ON" : "OFF",
                          O.MaxConsec > 0 ? "ON" : "OFF", hm, sc.Symbol.GetChars(), usdPerPt,
                          usdPerPt > 0.0 ? "" : " *** $/pt UNKNOWN: no entries ***");
            sc.AddMessageToLog(msg, (usdPerPt > 0.0 && P.TotalQty() > 0) ? 0 : 1);
            if (O.Fallbacks + P.Fallbacks > 0)
            {
                std::snprintf(msg, sizeof msg, "MSU: %d input(s) out of range -> defaults used. Re-enter them in Study Settings (#15).",
                              O.Fallbacks + P.Fallbacks);
                sc.AddMessageToLog(msg, 1);
            }
        }
        if (liveBlocked && O.OrderMode == 0 && !T.liveBlockWarned)
        {
            T.liveBlockWarned = 1;
            sc.AddMessageToLog("MSU: Trade Simulation Mode is OFF and 'Allow LIVE orders' = No -> no new entries. Open legs are still managed.", 1);
        }

        // ---- chart checks, once after every recalculation
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
            if (!(pre("MNQ") || pre("NQ"))) { T.cfgFail |= 4; AddWhy("symbol is not MNQ / NQ; "); }
            if (E.sessions < P.Lookback + 1)
            {
                char w[120]; std::snprintf(w, sizeof w, "only %d day sessions loaded, %d needed (Chart Settings >> Days to Load); ", E.sessions, P.Lookback + 1);
                T.cfgFail |= 8; AddWhy(w);
            }
            std::snprintf(T.cfgWhy, sizeof T.cfgWhy, "%s", why);
            std::snprintf(msg, sizeof msg, "MSU CHART CHECK %s | time zone %s (%s; chart setting '%s') | bars %d s | symbol %s | day sessions "
                          "loaded %d | %s%s", T.cfgFail ? "FAILED" : "ok", T.tzState == 1 ? "New York" : (T.tzState == 2 ? "NOT New York" : "unknown"),
                          tzInfo, tzName.GetChars(), sc.SecondsPerBar, sym != nullptr ? sym : "?", E.sessions,
                          T.cfgFail ? (O.Safety == 0 ? "NO NEW ENTRIES: " : "warn only: ") : "", T.cfgWhy);
            sc.AddMessageToLog(msg, (T.cfgFail || T.tzState == 0) ? 1 : 0);
            std::snprintf(msg, sizeof msg, "MSU ENVIRONMENT | trade account '%s' | Auto Trading Enabled %d | trading locked %d | %s | data feed "
                          "state %d (2 = connected) | system time - last trade %d s | safety checks %s | untracked positions %s | max entry "
                          "delay %d s | CME early closes %s", sc.SelectedTradeAccount.GetChars(), (int)sc.IsAutoTradingEnabled,
                          (int)sc.TradingIsLocked, realTime ? "REAL TIME" : "Chart Replay / back test", (int)sc.ServerConnectionState, rawLag,
                          O.Safety == 0 ? "BLOCK" : "WARN ONLY", O.Inherited == 0 ? "flattened" : "left alone", O.EntryDelay,
                          O.EarlyClose == 0 ? "honoured" : "IGNORED");
            sc.AddMessageToLog(msg, (sc.IsAutoTradingEnabled == 0 || sc.TradingIsLocked != 0 || O.Safety != 0 || O.Inherited != 0 ||
                                     O.EntryDelay < 0 || O.EarlyClose != 0) ? 1 : 0);
        }
        const bool cfgBlock = O.Safety == 0 && T.cfgFail != 0;
        if ((P.On[S_RSL] || P.On[S_ESL]) && !T.refWarned && (P.RefChart <= 0 || E.es.cursor == 0))
        {
            T.refWarned = 1;
            std::snprintf(msg, sizeof msg, "MSU REFERENCE CHART: RSL / ESL need an MES or ES 5-minute chart in this chartbook (same time zone, "
                          "same days loaded) and its chart number in the input 'Reference chart'. Input = %d, reference bars read = %d -> "
                          "those two legs make no signals until this is fixed.", P.RefChart, E.es.cursor);
            sc.AddMessageToLog(msg, 1);
        }

        // ---- first live call after a recalculation: the engine's signal numbers restarted; legs keep their order IDs
        if (T.postRecalc)
        {
            T.postRecalc = 0;
            for (int s = 0; s < NS; ++s) { T.sigSeqDone[s] = E.s[s].sigSeq; T.sigKeyDone[s] = E.s[s].sigKey; }
            if ((AnyLeg(LG_OPEN) || AnyLeg(LG_PENDING)))
                sc.AddMessageToLog("MSU: open or working legs carried across a recalculation -- still managed by their order IDs.", 0);
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
                        sc.AddMessageToLog("MSU *** MANUAL ACTION NEEDED *** the position is still open after 10 flatten attempts -- "
                                           "close it in the Trade Window. Retrying every 5 minutes.", 1);
                    Flatten(FW_REISSUE);
                }
                acted = true;                                         // nothing else while a flatten is in flight
            }
        }

        // ---- legs, by their own orders
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
                        std::snprintf(msg, sizeof msg, "MSU %s FILL %s signal #%d | %d contract(s) at %.2f (planned %.2f, slippage %+.2f pts)",
                                      kStrat[s], DirName(g.dir), g.sigSeq, g.qty, g.entryFill, g.entryRef, g.dir * (g.entryFill - g.entryRef));
                        sc.AddMessageToLog(msg, 0);
                    }
                    else if (!have || !IsWorkingOrderStatus(po.OrderStatusCode))
                    {
                        std::snprintf(msg, sizeof msg, "MSU %s ENTRY signal #%d not filled -- %s.", kStrat[s], g.sigSeq,
                                      g.canceling ? g.cancelWhy : "the order is no longer working (rejected or cancelled outside the study)");
                        sc.AddMessageToLog(msg, g.canceling ? 0 : 1);
                        std::memset(&g, 0, sizeof g);
                    }
                    else if (g.canceling) { if (nowAbs - g.cancelAbs >= 5) CancelEntry(s, g.cancelWhy); }
                    else if (g.kind == KIND_MARKET) { if (nowAbs - g.submitAbs >= 60) CancelEntry(s, "market entry not filled after 60 s"); }
                    else if (!inHours)              CancelEntry(s, "outside trading hours");
                    else if (nowTod >= g.expireTod) CancelEntry(s, "entry window ended");
                    else if (OppositeBroke(g))      CancelEntry(s, "the opposite side of the opening range broke first");
                }
                if (g.state == LG_OPEN)
                {
                    s_SCTradeOrder so;
                    const bool have = ReadOrder(g.stopID, so);
                    if (have && so.OrderStatusCode == SCT_OSC_FILLED)
                    {
                        CloseLeg(s, so.AvgFillPrice > 0.0 ? (double)so.AvgFillPrice : (double)so.Price1,
                                 g.exiting ? "engine exit: tightened stop" : "attached stop");
                        continue;
                    }
                    const bool stopWorks = have && IsWorkingOrderStatusIgnorePendingChildren(so.OrderStatusCode);
                    if (stopWorks && !g.fillVerified)                         // once per fill (#60)
                    {
                        g.fillVerified = 1;
                        const double want  = g.entryFill - g.dir * g.dist;
                        const bool   wrong = g.dir * (g.entryFill - so.Price1) <= 0.0;
                        std::snprintf(msg, sizeof msg, "MSU %s FILL VERIFY %s | fill %.2f | stop %.2f (planned %.2f, difference %+.2f) | stop "
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
                        std::snprintf(msg, sizeof msg, "MSU %s: the leg's stop has not been working for 5 s -- flattening everything.", kStrat[s]);
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
                    std::snprintf(msg, sizeof msg, "MSU: the account already holds %+d contract(s) this study did not open -- no new "
                                  "entries; %s.", posNet, O.Inherited == 0 ? "flattened at the flat time or at once outside trading hours"
                                                                           : "left alone (input)");
                    sc.AddMessageToLog(msg, 1);
                }
                if (O.Inherited == 0 && !inHours) Flatten(FW_INHERITED);
            }
            else if (!T.mismatch) { T.mismatch = 1; T.mismatchAbs = nowAbs; }
            else if (nowAbs - T.mismatchAbs >= 5)
            {
                std::snprintf(msg, sizeof msg, "MSU: account position %+d but the open legs add up to %+d for 5 s -- flattening everything "
                              "(another study or a manual trade on this account? lessons #72).", posNet, expNet);
                sc.AddMessageToLog(msg, 1);
                Flatten(FW_MISMATCH);
            }
            if (posNet == 0 && expNet == 0 && !AnyLeg(LG_PENDING) && working && !acted && nowAbs - T.staleCancelAbs >= 5)
            {
                T.staleCancelAbs = nowAbs;
                sc.CancelAllOrders(); acted = true;                           // working orders can outlive the state (#41)
                sc.AddMessageToLog("MSU: working orders found with no leg open or pending -- cancelled.", 1);
            }
        }

        // ---- trading-day rollover: the account P&L baseline, counters and halts (#43)
        if (nowTD != T.lastDay)
        {
            if (T.lastDay != 0 && (T.sigToday > 0 || T.legsToday > 0))
            {
                std::snprintf(msg, sizeof msg, "MSU DAY SUMMARY | signals %d, traded %d | legs closed %d (%d wins) | leg P&L ~%.2f | run ~%.2f",
                              T.sigToday, T.sigTradedToday, T.legsToday, T.winsToday, T.legPLToday, T.runPL);
                sc.AddMessageToLog(msg, 0);
            }
            T.lastDay = nowTD; T.legsToday = T.winsToday = T.lossesToday = T.consecLosses = T.sigToday = T.sigTradedToday = 0;
            T.legPLToday = 0.0; T.haltLogged = 0;
            T.baseCumPL = pos.CumulativeProfitLoss; T.baseTrades = pos.TotalTrades;
            HHMM(hm, sizeof hm, effEOD);
            std::snprintf(msg, sizeof msg, "MSU STATUS | session %s | flat time %s | averages %s (avg range %.2f, prior close %.2f) | account "
                          "position %+d | account realised P&L baseline %.2f", haltTod > 0 ? "SHORT (CME halt) -- no entries" : "normal", hm,
                          E.ringN >= P.Lookback ? "ready" : "NOT ready", E.adr, E.prevClose, posNet, T.baseCumPL);
            sc.AddMessageToLog(msg, 0);
        }
        if (pos.TotalTrades < T.baseTrades)                                   // Sierra's statistics were reset during the day
        {
            T.baseCumPL = pos.CumulativeProfitLoss - T.legPLToday; T.baseTrades = pos.TotalTrades;
            sc.AddMessageToLog("MSU: trade statistics were reset during the day -- the day's P&L baseline was rebuilt from the leg log.", 1);
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
            std::snprintf(msg, sizeof msg, "MSU RISK HALT for the rest of the day: %s | account day P&L %.2f (realised %.2f, open ~%.2f), "
                          "losing legs in a row %d. Resets on the next trading day.",
                          haltLoss ? "daily loss limit" : (haltProfit ? "daily profit lock" : "losing streak"), dayPLNow, realised, OpenPL(),
                          T.consecLosses);
            sc.AddMessageToLog(msg, 1);
        }

        // ---- exits the study decides
        if (!T.flattenPending && !acted && AnyLeg(LG_OPEN))
        {
            if (!inHours) Flatten(FW_EOD);
            else if (haltLoss) Flatten(FW_DAILY_LOSS);
        }
        // ---- new engine signals: trade them or say why not
        for (int s = 0; s < NS; ++s)
        {
            const Strat& S = E.s[s];
            if (!engOK || S.sigSeq <= 0 || (S.sigSeq == T.sigSeqDone[s] && S.sigKey == T.sigKeyDone[s])) continue;
            T.sigSeqDone[s] = S.sigSeq; T.sigKeyDone[s] = S.sigKey;
            if (!P.On[s]) continue;                                           // switched off: tally only, no log line
            ++T.sigToday;
            const int64_t due = S.sigKey + bsec;
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
            std::snprintf(msg, sizeof msg, "MSU %s engine SIGNAL #%d %s -> %s%s", kStrat[s], S.sigSeq, DirName(S.sigDir),
                          why == nullptr ? "TRADED" : "NOT traded -- ", why == nullptr ? "" : why);
            sc.AddMessageToLog(msg, (why != nullptr && O.OrderMode == 0) ? 1 : 0);
        }

        // ---- engine exits, after the entries: a leg opened in this call keeps another leg's exit from flattening it
        if (!T.flattenPending && !acted)
        {
            int staying = 0, leaving = 0;
            for (int s = 0; s < NS; ++s)                                  // pass 1: which legs does the engine want out?
            {
                Leg& g = T.leg[s];
                const Strat& S = E.s[s];
                if (g.state == LG_OPEN && !g.exiting && engOK && S.exitKey != T.exitKeyDone[s] && S.exitKey + bsec > g.fillAbs)
                {
                    T.exitKeyDone[s] = S.exitKey;                         // decided on a bar that closed after the fill, handled once
                    if ((g.dir > 0 && S.exitLong) || (g.dir < 0 && S.exitShort)) g.exiting = 1;
                }
                if (g.state == LG_OPEN && g.exiting) ++leaving; else if (g.state != LG_FLAT) ++staying;
            }
            if (leaving > 0 && staying == 0) Flatten(FW_ENGINE);          // nothing else to protect: one flatten
            else if (leaving > 0)
                for (int s = 0; s < NS && !acted; ++s)                    // pass 2: tighten the leaving legs' own stops
                    if (T.leg[s].state == LG_OPEN && T.leg[s].exiting && TightenStop(s) < 0) Flatten(FW_STOP_MOVE);
        }
    }
}   // namespace msu

SCSFExport scsf_MomentumSuite_NQ_MNQ(SCStudyInterfaceRef sc)
{
    using namespace msu;

    if (sc.SetDefaults)
    {
        sc.GraphName        = "Momentum Suite NQ MNQ " STUDY_VERSION;
        sc.StudyDescription = "Six MNQ day-session strategies in one study on one trade account: noise-band momentum (NBM), relative strength vs MES (RSL), "
                              "gap opening drive (GOD), VWAP pullback (TVP), pyramid add-on (PYR), reference market leads (ESL). One leg each, with its own "
                              "attached stop; all flat at 15:55 New York. MNQ 5-minute chart, New York time. Replay-testable tick by tick.";
        sc.AutoLoop         = 1;
        sc.GraphRegion      = 0;
        sc.ValueFormat      = VALUEFORMAT_INHERITED;
        sc.DrawZeros        = 0;

        auto Style = [&](int idx, const char* name, int style, COLORREF col, int width)
        {
            SCSubgraphRef s = sc.Subgraph[idx];
            s.Name = name; s.DrawStyle = style; s.PrimaryColor = col; s.LineWidth = width; s.DrawZeros = 0;
        };
        Style(SG_VWAP,   "Session VWAP (from the day-session open)", DRAWSTYLE_LINE,       RGB(200, 140, 40),  2);
        Style(SG_UB,     "Noise band, upper (NBM width)",            DRAWSTYLE_LINE,       RGB(63, 185, 80),   1);
        Style(SG_LB,     "Noise band, lower (NBM width)",            DRAWSTYLE_LINE,       RGB(248, 81, 73),   1);
        Style(SG_TRIG,   "GOD entry trigger",                        DRAWSTYLE_DASH,       RGB(88, 166, 255),  2);
        Style(SG_CANCEL, "GOD cancel level",                         DRAWSTYLE_DASH,       RGB(150, 150, 150), 2);
        Style(SG_TREND,  "TVP day trend +1/-1 (stored)",             DRAWSTYLE_IGNORE,     RGB(200, 200, 200), 1);
        Style(SG_LONG0,  "NBM long signal",                          DRAWSTYLE_ARROW_UP,   RGB(63, 185, 80),   3);
        Style(SG_LONG1,  "GOD long signal",                          DRAWSTYLE_ARROW_UP,   RGB(88, 166, 255),  3);
        Style(SG_LONG2,  "TVP long signal",                          DRAWSTYLE_ARROW_UP,   RGB(200, 140, 40),  3);
        Style(SG_SHORT0, "NBM short signal",                         DRAWSTYLE_ARROW_DOWN, RGB(248, 81, 73),   3);
        Style(SG_SHORT1, "GOD short signal",                         DRAWSTYLE_ARROW_DOWN, RGB(150, 70, 180),  3);
        Style(SG_SHORT2, "TVP short signal",                         DRAWSTYLE_ARROW_DOWN, RGB(255, 150, 0),   3);
        Style(SG_STOP0,  "Order: NBM leg stop",                      DRAWSTYLE_DASH,       RGB(248, 81, 73),   2);
        Style(SG_STOP1,  "Order: GOD leg stop",                      DRAWSTYLE_DASH,       RGB(150, 70, 180),  2);
        Style(SG_STOP2,  "Order: TVP leg stop",                      DRAWSTYLE_DASH,       RGB(255, 150, 0),   2);
        Style(SG_LONG3,  "RSL long signal",                          DRAWSTYLE_ARROW_UP,   RGB(0, 200, 200),   3);
        Style(SG_LONG4,  "PYR long add",                             DRAWSTYLE_ARROW_UP,   RGB(120, 220, 120), 2);
        Style(SG_LONG5,  "ESL long signal",                          DRAWSTYLE_ARROW_UP,   RGB(180, 180, 255), 3);
        Style(SG_SHORT3, "RSL short signal",                         DRAWSTYLE_ARROW_DOWN, RGB(200, 0, 200),   3);
        Style(SG_SHORT4, "PYR short add",                            DRAWSTYLE_ARROW_DOWN, RGB(255, 140, 140), 2);
        Style(SG_SHORT5, "ESL short signal",                         DRAWSTYLE_ARROW_DOWN, RGB(255, 200, 120), 3);
        Style(SG_STOP3,  "Order: RSL leg stop",                      DRAWSTYLE_DASH,       RGB(200, 0, 200),   2);
        Style(SG_STOP4,  "Order: PYR leg stop",                      DRAWSTYLE_DASH,       RGB(255, 140, 140), 2);
        Style(SG_STOP5,  "Order: ESL leg stop",                      DRAWSTYLE_DASH,       RGB(255, 200, 120), 2);
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
        DefYN(IN_NBM_ON,      "NBM noise-band momentum: trade it", D.On[S_NBM]);
        DefIN(IN_NBM_QTY,     "NBM contracts", D.Qty[S_NBM], 1, 100);
        DefFL(IN_NBM_BAND,    "NBM noise band width, x the average move for the time of day (measured 1.0)", D.NbmBand, 0.1, 10.0);
        DefFL(IN_NBM_CAT,     "NBM attached disaster stop, x average day range (measured 0.5)", D.NbmCat, 0.05, 5.0);
        DefIN(IN_NBM_MAXTR,   "NBM max trades per day (measured 3; 0 = off)", D.NbmMaxTr, 0, 50);
        DefYN(IN_GOD_ON,      "GOD gap opening drive: trade it", D.On[S_GOD]);
        DefIN(IN_GOD_QTY,     "GOD contracts", D.Qty[S_GOD], 1, 100);
        DefFL(IN_GOD_MINGAP,  "GOD min gap: |open - prior close|, x average day range (measured 0.05)", D.GodMinGap, 0.0, 5.0);
        DefFL(IN_GOD_STOP,    "GOD attached stop, x average day range (measured 0.10)", D.GodStop, 0.01, 5.0);
        DefIN(IN_GOD_VALID,   "GOD entry order rests until, minutes after the open (0 = 60 default)", 0, 0, 390);
        DefIN(IN_GOD_CHASE,   "GOD price already through the trigger: market if within, ticks", D.GodChase, 0, 100);
        DefYN(IN_TVP_ON,      "TVP trend-day VWAP pullback: trade it", D.On[S_TVP]);
        DefIN(IN_TVP_QTY,     "TVP contracts", D.Qty[S_TVP], 1, 100);
        DefFL(IN_TVP_BAND,    "TVP noise band width (measured 1.0)", D.TvpBand, 0.1, 10.0);
        DefFL(IN_TVP_STOP,    "TVP attached stop, x average day range (measured 0.15)", D.TvpStop, 0.01, 5.0);
        DefIN(IN_TVP_CONFIRM, "TVP confirmation block, chart bars (measured 3 = 15 minutes)", D.TvpConfirm, 1, 12);
        DefTM(IN_SESS_OPEN,   "Day session opens (New York)", D.SessOpen);
        DefTM(IN_SESS_CLOSE,  "Day session closes (New York)", D.SessClose);
        DefTM(IN_EOD,         "Flat time: cancel entries, close everything (New York; measured 15:55)", D.EOD);
        DefCX(IN_DIRECTIONS,  "Directions", "Long and short (measured);Long only;Short only", D.Directions);
        DefIN(IN_LOOKBACK,    "Sessions in the average range / noise profile (measured 14)", D.Lookback, 3, MAXLOOK);
        DefFL(IN_TALLY_COST,  "Tally cost per contract per round trip, points (diagnostic only)", D.TallyCost, 0.0, 50.0);
        DefYN(IN_VERBOSE,     "Verbose log", D.Verbose);
        const OParams DO;
        DefCX(IN_ORDER_MODE,  "Orders", "Send orders (Trade Simulation; live needs the next input);Signals only -- no orders", DO.OrderMode);
        DefYN(IN_ALLOW_LIVE,  "Allow LIVE orders (when Trade Simulation Mode is OFF)", DO.AllowLive);
        DefFL(IN_MAX_RISK_USD, "Max $ risk per leg (0 = off; reduces contracts, skips if 1 is over)", DO.MaxRiskUSD, 0.0, 1.0e7);
        DefFL(IN_DAILY_LOSS,  "Daily loss halt, $ on the account incl. open legs (0 = off)", DO.DailyLoss, 0.0, 1.0e7);
        DefFL(IN_PROFIT_LOCK, "Daily profit lock, $ realised: no new entries after (0 = off)", DO.ProfitLock, 0.0, 1.0e7);
        DefIN(IN_MAX_CONSEC,  "Max losing legs in a row per day (0 = off)", DO.MaxConsec, 0, 50);
        DefCX(IN_SAFETY,      "Safety checks (chart: New York time, 5-min bars, MNQ; feed: connected, real time)",
              "Block new entries when a check fails (recommended);Warn only", DO.Safety);
        DefCX(IN_INHERITED,   "Position already on the account when the study starts",
              "Flatten at the flat time, or at once outside trading hours (recommended);Leave alone", DO.Inherited);
        DefIN(IN_ENTRY_DELAY, "Max entry delay after the signal bar closes, seconds (0 = 120 default, -1 = off)", 0, -1, 3600);
        DefCX(IN_EARLY_CLOSE, "CME holiday halts / early closes (13:00 / 13:15 New York)",
              "No entries; flatten before the halt (recommended);Ignore", DO.EarlyClose);
        DefIN(IN_REF_CHART,   "Reference chart number: MES or ES, 5-minute, same time zone (0 = none; needed by RSL and ESL)", D.RefChart, 0, 5000);
        DefYN(IN_RSL_ON,      "RSL relative-strength leader (NQ vs reference at 10:00): trade it", D.On[S_RSL]);
        DefIN(IN_RSL_QTY,     "RSL contracts", D.Qty[S_RSL], 0, 100);
        DefFL(IN_RSL_K,       "RSL min relative strength, x the average NQ move at 10:00 (measured 0.45)", D.RslK, 0.0, 10.0);
        DefFL(IN_RSL_BETA,    "RSL reference-market multiplier (measured 1.2)", D.RslBeta, 0.0, 5.0);
        DefFL(IN_RSL_STOP,    "RSL attached stop, x average day range (measured 0.15)", D.RslStop, 0.0, 5.0);
        DefYN(IN_PYR_ON,      "PYR add-on to a winning NBM trade: trade it", D.On[S_PYR]);
        DefIN(IN_PYR_QTY,     "PYR contracts", D.Qty[S_PYR], 0, 100);
        DefFL(IN_PYR_PROFIT,  "PYR needs the NBM trade ahead by, x average day range (measured 0.15)", D.PyrProfit, 0.0, 5.0);
        DefFL(IN_PYR_STOP,    "PYR attached stop, x average day range (measured 0.25)", D.PyrStop, 0.0, 5.0);
        DefYN(IN_ESL_ON,      "ESL reference market leads (its band broken, NQ's not yet): trade it", D.On[S_ESL]);
        DefIN(IN_ESL_QTY,     "ESL contracts", D.Qty[S_ESL], 0, 100);
        DefFL(IN_ESL_STOP,    "ESL attached stop, x average day range (measured 0.20)", D.EslStop, 0.0, 5.0);
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

    // Trade-behaviour settings on EVERY call (lessons #15/#16). Several legs share the position.
    sc.SendOrdersToTradeService                       = !sc.GlobalTradeSimulationIsOn;  // read, never assigned
    sc.MaximumPositionAllowed                         = IMax(P.TotalQty(), 1);           // lessons #8
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
        std::snprintf(msg, sizeof msg, "MSU Momentum Suite NQ MNQ " STUDY_VERSION " | built " STUDY_BUILD_STAMP " | chart #%d %s | %d s "
                      "bars | inputs out of range: %d", sc.ChartNumber, sc.Symbol.GetChars(), sc.SecondsPerBar, P.Fallbacks + O.Fallbacks);
        sc.AddMessageToLog(msg, 0);
    }
    if (sc.TickSize <= 0.0f)
    {
        if (!E.tickWarned) { E.tickWarned = 1; sc.AddMessageToLog("MSU: sc.TickSize is 0 -- engine idle. Check the symbol settings.", 1); }
        return;
    }
    E.usdPerPt   = sc.CurrencyValuePerTick > 0.0f ? (double)sc.CurrencyValuePerTick / sc.TickSize : 0.0;
    E.virtualLog = O.OrderMode != 0 ? 1 : 0;

    ScFeed f(sc);
    f.LoadRef(P.RefChart);
    // Commit every closed bar exactly once, in order (lessons #26/#27).
    const int lastClosed = sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_CLOSED ? i : i - 1;
    if (lastClosed > E.lastCommitted)
        for (int k = IMax(E.lastCommitted + 1, 0); k <= lastClosed; ++k)
        {
            E.quiet = (sc.IsFullRecalculation || k < sc.ArraySize - 2) ? 1 : 0;   // lessons #63: per bar, not per call
            ProcessBar(f, E, P, k);
            RefDecide(f, E, P, k);                              // RSL / ESL for bar k, if its reference bar is already final
            E.lastCommitted = k;
        }
    if (lastClosed < i)                                     // the forming bar: carry the lines, no decisions
        for (int g = 0; g <= SG_CANCEL; ++g) sc.Subgraph[g][i] = i > 0 ? sc.Subgraph[g][i - 1] : 0.0f;

    E.quiet = (sc.IsFullRecalculation || E.refPendBar < sc.ArraySize - 2) ? 1 : 0;
    RefDecide(f, E, P, lastClosed);                         // RSL / ESL, once the reference bar for the mark is final

    if (!E.tallyLogged && E.lastCommitted >= sc.ArraySize - 2) { E.tallyLogged = 1; LogTally(f, E, P); }
    if (!sc.IsFullRecalculation || i == sc.ArraySize - 1) E.recalcOpen = 0;

    OrderLayer(sc, E, P, O, T);

    for (int s = 0; s < NS; ++s)
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
