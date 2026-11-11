    // ---------------------------------------------------------------- decision
    // Compression -> expansion. Yesterday's day-session range was the smallest of the last 'NR days'
    // full sessions (Crabel's NR4). Today: the first bar that CLOSES outside the opening range is
    // followed, either way, with a volatility stop, to the close.
    enum : int { PREOPEN = 0 };
    template <class F> static void PreDecide(F& f, Engine& E, const Params& P, int k) { (void)f; (void)E; (void)P; (void)k; }
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec)
    {
        const int endT = tod + bsec, orEnd = P.SessOpen + P.OrMinutes * 60;
        f.Out(SG_L1, k) = (float)E.nrCount;                             // stored for inspection, not drawn
        if (tod >= orEnd && E.orh > 0.0) { f.Out(SG_UB, k) = (float)E.orh; f.Out(SG_LB, k) = (float)E.orl; }
        if (E.dayDead) return;
        if (E.nrCount < P.NrDays) { E.dayDead = 1; return; }            // yesterday was not a narrow-range day
        if (tod < orEnd)
        {
            if (E.barsSeen != b + 1) { E.dayDead = 1; return; }         // an opening-range bar is missing
            if (b == 0) { E.orh = f.H(k); E.orl = f.L(k); }
            else { E.orh = DMax(E.orh, f.H(k)); E.orl = DMin(E.orl, f.L(k)); }
            return;
        }
        if (E.orh <= 0.0 || endT > P.LastEntry) { E.dayDead = 1; return; }
        const double c = f.C(k);
        const int d = c > E.orh ? 1 : (c < E.orl ? -1 : 0);
        if (d == 0) return;
        E.dayDead = 1;                                                  // one decision per day
        if (!DirOn(P, d) || (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades) || E.vt.state != VS_FLAT) return;
        char what[200];
        std::snprintf(what, sizeof what, "prior range the smallest of %d sessions; close %.2f %s the %d-minute opening range %.2f-%.2f",
                      E.nrCount, c, d > 0 ? "above" : "below", P.OrMinutes, E.orl, E.orh);
        Emit(f, E, P, k, d, KIND_MARKET, 0.0, P.StopAdr * E.adr, 0.0, 0, what);
    }
