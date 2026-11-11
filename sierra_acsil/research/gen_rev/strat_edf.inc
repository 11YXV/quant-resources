    // ---------------------------------------------------------------- decision
    // End-of-day flow. At the 'Decision at' bar close (15:00), measure the move from the prior
    // day-session close. When it is at least 'Min move' percent, the rebalancing of leveraged ETFs
    // and the hedging of short-gamma option positions buy (sell) into the close in that direction:
    // trade with the move, attached volatility stop, out at the flat time. One decision a day.
    enum : int { PREOPEN = 0 };
    template <class F> static void PreDecide(F& f, Engine& E, const Params& P, int k) { (void)f; (void)E; (void)P; (void)k; }
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec)
    {
        (void)b;
        const double band = P.MinMovePct / 100.0;
        f.Out(SG_UB, k) = (float)(E.prevClose * (1.0 + band));         // the move needed, drawn all day
        f.Out(SG_LB, k) = (float)(E.prevClose * (1.0 - band));
        if (E.dayDead) return;
        const int endT = tod + bsec;
        if (endT < P.EntryMark) return;
        E.dayDead = 1;                                                  // one decision per day
        if (endT != P.EntryMark) return;                                // the decision bar is missing from the data
        const double mv = f.C(k) / E.prevClose - 1.0;
        f.Out(SG_L1, k) = (float)(100.0 * mv);
        if (DAbs(mv) * 100.0 < P.MinMovePct) return;
        const int d = mv > 0.0 ? 1 : -1;
        if (!DirOn(P, d) || (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades) || E.vt.state != VS_FLAT) return;
        char what[200];
        std::snprintf(what, sizeof what, "%+.2f%% from the prior close %.2f at the decision bar (min %.2f%%): trade with the move into the close",
                      100.0 * mv, E.prevClose, P.MinMovePct);
        Emit(f, E, P, k, d, KIND_MARKET, 0.0, P.StopAdr * E.adr, 0.0, 0, what);
    }
