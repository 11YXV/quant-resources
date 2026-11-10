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
