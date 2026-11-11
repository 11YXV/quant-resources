// Runs the real study code on 1-minute bars replayed as 4 ticks each, 5-minute chart bars, simulated broker.
// usage: harness <bars.csv> <trades_out.csv> <log_out> <histSessions> <recalcEvery (0=never)> <mode> [inputIdx=value ...]
#include STUDY_FILE
#include <cstdlib>
#include <cstring>
#include <map>
#include <array>
int main(int argc, char** argv) {
  s_sc sc; FILE* in = fopen(argv[1], "r"); sc.br.out = fopen(argv[2], "w"); sc.log = fopen(argv[3], "w");
  const int hist = atoi(argv[4]), recalcEvery = atoi(argv[5]), mode = atoi(argv[6]);
  sc.SetDefaults = 1; STUDY_FN(sc); sc.SetDefaults = 0;
  for (int a = 7; a < argc; ++a) { int idx; double v; if (sscanf(argv[a], "%d=%lf", &idx, &v) == 2) { sc.Input[idx].iv = (int)v; sc.Input[idx].fv = v; } }
  const char* ev; long failAt = (ev = getenv("FAILFLAT_AT")) ? atol(ev) : -1; long injAt = (ev = getenv("INJECT_AT")) ? atol(ev) : -1;
  if (getenv("TZ_UTC")) sc.ConvertDateTimeUTCToChartTimeZone = &s_sc::ConvUTC;
  auto recalc = [&]() { sc.IsFullRecalculation = 1; for (int i = 0; i < sc.ArraySize; ++i) { sc.Index = i; STUDY_FN(sc); } sc.IsFullRecalculation = 0; };
  std::map<long long, std::array<double, 4>> ref;
  if (const char* rp = getenv("REF")) { FILE* rf = fopen(rp, "r"); char ln[256]; int rd, rt; double a, b, e, g, vv;
    while (fgets(ln, sizeof ln, rf)) if (sscanf(ln, "%d,%d,%lf,%lf,%lf,%lf,%lf", &rd, &rt, &a, &b, &e, &g, &vv) == 7) ref[(long long)rd * 86400 + rt] = { a, b, e, g }; fclose(rf); }
  int d, t; double o, h, l, c, v; long calls = 0, ticks = 0, viol = 0; int sessions = 0, lastD = 0; bool live = false; char line[256];
  auto call = [&](int i) { sc.Index = i; STUDY_FN(sc); ++calls; };
  while (fgets(line, sizeof line, in)) {
    if (sscanf(line, "%d,%d,%lf,%lf,%lf,%lf,%lf", &d, &t, &o, &h, &l, &c, &v) != 7) continue;
    if (d != lastD) { lastD = d; ++sessions; if (!live && sessions > hist) { live = true; recalc(); } }
    const int bt = t - t % 300; const int n = sc.ArraySize;
    { auto it = ref.find((long long)d * 86400 + t);
      if (it != ref.end()) { const auto& r = it->second; const int rn = (int)sc.rC.size();
        if (rn == 0 || sc.rDT.d[rn - 1] != d || sc.rDT.t[rn - 1] != bt) { sc.rO.push_back((float)r[0]); sc.rH.push_back((float)r[1]); sc.rL.push_back((float)r[2]); sc.rC.push_back((float)r[3]); sc.rDT.d.push_back(d); sc.rDT.t.push_back(bt); }
        else { if (r[1] > sc.rH[rn - 1]) sc.rH[rn - 1] = (float)r[1]; if (r[2] < sc.rL[rn - 1]) sc.rL[rn - 1] = (float)r[2]; sc.rC[rn - 1] = (float)r[3]; } } }
    const bool nb = n == 0 || sc.BaseDateTimeIn.d[n - 1] != d || sc.BaseDateTimeIn.t[n - 1] != bt;
    const double px[4] = { o, c >= o ? l : h, c >= o ? h : l, c };
    for (int k = 0; k < 4; ++k) {
      const int tt = t + 15 * k; const bool first = nb && k == 0; const float p = (float)px[k];
      if (first) { sc.Open.v.push_back(p); sc.High.v.push_back(p); sc.Low.v.push_back(p); sc.Close.v.push_back(p); sc.Volume.v.push_back(0);
                   sc.BaseDateTimeIn.d.push_back(d); sc.BaseDateTimeIn.t.push_back(bt); ++sc.ArraySize; }
      const int m = sc.ArraySize - 1;
      if (p > sc.High.v[m]) sc.High.v[m] = p; if (p < sc.Low.v[m]) sc.Low.v[m] = p; sc.Close.v[m] = p; sc.Volume.v[m] += (float)(v / 4);
      sc.LatestDateTimeForLastBar = SCDateTime(d, tt); sc.CurrentSystemDateTime = SCDateTime(d, tt);
      if (!live) continue;
      sc.br.tick(d, tt, p); ++ticks;
      if (ticks == failAt) failAt = -2;
      if (failAt == -2 && sc.br.pos != 0) { sc.br.failFlatten = 12; failAt = -1; }
      if (ticks == injAt) sc.br.exec(1, 1, p);
      if (recalcEvery > 0 && ticks % recalcEvery == 0) recalc();
      if (mode == 1 && !first && k != 3) continue;                 // sparse calls: once per minute
      if (first && m > 0) call(m - 1);
      call(m);
      if (mode == 2) call(m);                                       // dense: twice
      // invariants
      if (std::fabs(sc.br.pos) > sc.MaximumPositionAllowed) ++viol;
      if (sc.br.pos != 0 && (tt >= 15 * 3600 + 56 * 60 || tt < 9 * 3600 + 30 * 60)) ++viol;
    } }
  sc.LastCallToFunction = 1; STUDY_FN(sc);
  int working = 0; for (auto& x : sc.br.ord) if (IsWorkingOrderStatus(x.status)) ++working;
  printf("sessions %d bars %d ticks %ld calls %ld | fills %ld trades %d cumPL %.2f | end pos %.0f working %d | invariant violations %ld | log lines %ld / severity-1 %ld\n",
         sessions, sc.ArraySize, ticks, calls, sc.br.fills, sc.br.trades, sc.br.cum, sc.br.pos, working, viol, sc.nLog[0], sc.nLog[1]);
  return 0; }
