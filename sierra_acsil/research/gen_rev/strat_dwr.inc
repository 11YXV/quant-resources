    // ---------------------------------------------------------------- decision
    // Daily washout rebound. Decided on the bar that ENDS at the day-session open, so the order goes
    // in at the open: RSI of the last day-session closes below 'RSI buy' -> long. (An optional short
    // side, RSI above 'RSI sell', is off by default: it did not hold up in testing.)
    enum : int { PREOPEN = 1 };
    template <class F> static void PreDecide(F& f, Engine& E, const Params& P, int k)
    {
        f.Out(SG_L1, k) = (float)E.rsi;
        if (E.rsi < 0.0 || E.vt.state != VS_FLAT || (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades)) return;
        const int d = E.rsi < P.RsiBuy ? 1 : ((P.RsiSell > 0.0 && E.rsi > P.RsiSell) ? -1 : 0);
        if (d == 0 || !DirOn(P, d)) return;
        char what[160];
        std::snprintf(what, sizeof what, "RSI(%d) of day-session closes %.1f %s %.1f: enter at the open, stop %.2f x avg range",
                      P.RsiLen, E.rsi, d > 0 ? "below" : "above", d > 0 ? P.RsiBuy : P.RsiSell, P.StopAdr);
        Emit(f, E, P, k, d, KIND_MARKET, 0.0, P.StopAdr * E.adr, 0.0, 0, what);
    }
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec)
    {
        (void)P; (void)b; (void)tod; (void)bsec;
        f.Out(SG_L1, k) = (float)E.rsi;                                 // stored for inspection, not drawn
    }
