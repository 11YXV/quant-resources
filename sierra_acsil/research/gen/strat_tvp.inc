    // ---------------------------------------------------------------- decision
    // Trend-day VWAP pullback. The day's trend is set at 30-minute marks (close beyond a noise band).
    // At every 'Confirm bars' boundary: with an up trend, the last Confirm bars traded down to VWAP
    // and the close is back above it -> long at the next open; short mirrored.
    template <class F> static void Decide(F& f, Engine& E, const Params& P, int k, int b, int tod, int bsec, double ub, double lb)
    {
        const int endT = tod + bsec;
        if (endT < P.FirstMark) return;
        const double c = f.C(k), vw = E.vwap;
        if ((endT - P.SessOpen) % 1800 == 0)
        {
            if (c > ub) E.trend = 1; else if (c < lb) E.trend = -1;
        }
        f.Out(SG_L1, k) = (float)E.trend;
        if (E.trend == 0 || (endT - P.SessOpen) % (P.ConfirmBars * bsec) != 0 || endT > P.LastEntry) return;
        if (E.vt.state != VS_FLAT || (P.MaxTrades > 0 && E.tradesToday >= P.MaxTrades)) return;
        if (b < P.ConfirmBars - 1) return;
        double lo = f.L(k), hi = f.H(k);
        for (int q = 1; q < P.ConfirmBars && k - q >= 0; ++q)
        {
            if (f.Date(k - q) != f.Date(k) || f.Tod(k - q) < P.SessOpen) break;
            lo = DMin(lo, f.L(k - q)); hi = DMax(hi, f.H(k - q));
        }
        const int d = (E.trend > 0 && lo <= vw && c > vw) ? 1 : ((E.trend < 0 && hi >= vw && c < vw) ? -1 : 0);
        if (d == 0 || !DirOn(P, d)) return;
        Emit(f, E, P, k, d, KIND_MARKET, 0.0, P.StopAdr * E.adr, 0.0, 0, "trend day, pullback to VWAP rejected");
    }
