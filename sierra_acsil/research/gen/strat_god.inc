    // ---------------------------------------------------------------- decision
    // Gap-aligned opening drive. When the opening range is complete: the session opened at least
    // 'Min gap' x average range away from the prior close -> a resting stop entry one tick beyond the
    // range on the GAP's side only; a break of the other side first cancels the day.
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec, double ub, double lb)
    {
        (void)ub; (void)lb;
        const double tick = f.Tick();
        VTrade& v = E.vt;
        if (v.state == VS_PEND_STOP) { f.Out(SG_L1, k) = (float)v.trig; f.Out(SG_L2, k) = (float)v.cancelPx; }
        if (E.dayDead) return;
        const int endT = tod + bsec, orEnd = P.SessOpen + P.OrMinutes * 60;
        if (b == 0) { E.orh = f.H(k); E.orl = f.L(k); }
        else if (tod < orEnd) { E.orh = DMax(E.orh, f.H(k)); E.orl = DMin(E.orl, f.L(k)); }
        if (endT < orEnd) return;
        E.dayDead = 1;                                                  // one decision per day
        if (endT != orEnd || E.barsSeen * bsec != P.OrMinutes * 60) return;   // an opening-range bar is missing
        const double gap = (E.dOpen - E.prevClose) / E.adr;
        if (DAbs(gap) < P.MinGap || (P.MaxGap > 0.0 && DAbs(gap) > P.MaxGap)) return;
        const int d = gap > 0.0 ? 1 : -1;
        if (!DirOn(P, d) || (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades) || v.state != VS_FLAT) return;
        const double trig = d > 0 ? E.orh + tick : E.orl - tick;
        const double cancel = d > 0 ? E.orl - tick : E.orh + tick;
        char what[160];
        std::snprintf(what, sizeof what, "gap %+.2f x avg range, opening range %.2f-%.2f: stop entry %.2f, cancelled through %.2f",
                      gap, E.orl, E.orh, trig, cancel);
        Emit(f, E, P, k, d, KIND_STOP, trig, P.StopAdr * E.adr, cancel, P.SessOpen + P.ValidMin * 60, what);
        f.Out(SG_L1, k) = (float)trig; f.Out(SG_L2, k) = (float)cancel;
    }
