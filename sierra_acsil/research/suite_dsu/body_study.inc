
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
