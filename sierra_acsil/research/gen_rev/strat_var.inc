    // ---------------------------------------------------------------- decision
    // Value-area re-entry (the "80% rule" of auction-market theory). The day session opens OUTSIDE the
    // prior session's value area; at a 30-minute mark the close is back INSIDE it -> the open was
    // rejected: trade towards the far side of value (short if the open was above, long if below).
    enum : int { PREOPEN = 0 };
    template <class F> static void PreDecide(F& f, Engine& E, const Params& P, int k) { (void)f; (void)E; (void)P; (void)k; }
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec)
    {
        if (E.vaOK) { f.Out(SG_UB, k) = (float)E.vaH; f.Out(SG_LB, k) = (float)E.vaL; f.Out(SG_L1, k) = (float)E.vaPOC; }
        if (E.dayDead) return;
        if (E.barsSeen == 1)                                            // the session's first bar: where did it open?
        {
            if (b != 0 || !E.vaOK) { E.dayDead = 1; return; }           // first bar missing, or the prior session was not a full one
            E.side = E.dOpen > E.vaH ? -1 : (E.dOpen < E.vaL ? 1 : 0);
            if (E.side == 0) { E.dayDead = 1; return; }                 // opened inside value: no trade today
        }
        const int endT = tod + bsec;
        if (endT > P.LastEntry) { E.dayDead = 1; return; }
        if (endT < P.FirstMark || (endT - P.SessOpen) % (P.MarkMin * 60) != 0) return;
        const double c = f.C(k);
        if (!(c > E.vaL && c < E.vaH)) return;
        E.dayDead = 1;                                                  // one decision per day
        if (!DirOn(P, E.side) || (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades) || E.vt.state != VS_FLAT) return;
        char what[200];
        std::snprintf(what, sizeof what, "opened %.2f %s the prior value area %.2f-%.2f (POC %.2f), %d-minute close back inside",
                      E.dOpen, E.side < 0 ? "above" : "below", E.vaL, E.vaH, E.vaPOC, P.MarkMin);
        Emit(f, E, P, k, E.side, KIND_MARKET, 0.0, P.StopAdr * E.adr, 0.0, 0, what);
    }
