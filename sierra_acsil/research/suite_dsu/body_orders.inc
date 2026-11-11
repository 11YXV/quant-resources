
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
