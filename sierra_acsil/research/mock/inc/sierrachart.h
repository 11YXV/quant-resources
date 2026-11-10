// Behavioural mock of the ACSIL surface these studies use + a simulated broker. Test tool only.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#define SCDLLName(x)
#define SCSFExport void
#define RGB(r,g,b) ((unsigned)(((r)&0xff)|(((g)&0xff)<<8)|(((b)&0xff)<<16)))
typedef unsigned COLORREF;
enum { DRAWSTYLE_LINE, DRAWSTYLE_IGNORE, DRAWSTYLE_DASH, DRAWSTYLE_ARROW_UP, DRAWSTYLE_ARROW_DOWN, DRAWSTYLE_POINT };
enum { VALUEFORMAT_INHERITED = 40 };
enum { BHCS_BAR_HAS_CLOSED = 2, BHCS_BAR_HAS_NOT_CLOSED = 3 };
enum { SCT_ORDERTYPE_MARKET = 0, SCT_ORDERTYPE_LIMIT = 1, SCT_ORDERTYPE_STOP = 2 };
enum SCTimeInForceEnum { SCT_TIF_UNSET = 0, SCT_TIF_DAY = 1 };
enum { SCT_OSC_PENDING_CHILD = 3, SCT_OSC_OPEN = 5, SCT_OSC_FILLED = 8, SCT_OSC_CANCELED = 9 };
enum { SCTRADING_ORDER_ERROR = -1, SCS_CONNECTED = 2 };
inline bool IsWorkingOrderStatus(int c) { return c == SCT_OSC_OPEN || c == SCT_OSC_PENDING_CHILD; }
inline bool IsWorkingOrderStatusIgnorePendingChildren(int c) { return c == SCT_OSC_OPEN; }
struct SCString { std::string s; SCString() {} SCString(const char* c) : s(c) {} const char* GetChars() const { return s.c_str(); }
  void Format(const char* f, ...) { char b[512]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); s = b; }
  SCString& operator=(const char* c) { s = c; return *this; } };
struct SCDateTime { int d = 0, t = 0; SCDateTime() {} SCDateTime(int D, int T) : d(D), t(T) {} int GetDate() const { return d; } int GetTime() const { return t; } bool IsDateSet() const { return d != 0; } };
struct s_SCNewOrder { int OrderType = 0; double Price1 = 1e300; double OrderQuantity = 0; SCTimeInForceEnum TimeInForce = SCT_TIF_UNSET;
  char AttachedOrderStop1Type = SCT_ORDERTYPE_STOP; double Stop1Offset = 0.0; SCString TextTag; uint32_t InternalOrderID = 0, Stop1InternalOrderID = 0; };
struct s_SCTradeOrder { uint32_t InternalOrderID = 0; double Price1 = 0; int OrderStatusCode = 0; double OrderQuantity = 0; };
struct s_SCPositionData { double PositionQuantity = 0, AveragePrice = 0; int WorkingOrdersExist = 0, TotalTrades = 0; double CumulativeProfitLoss = 0, LastTradeProfitLoss = 0; };
struct MockInput { std::string Name; int kind = 0; int iv = 0; double fv = 0; 
  void SetYesNo(int v) { iv = v; } void SetTime(int v) { iv = v; } void SetInt(int v) { iv = v; } void SetIntLimits(int, int) {}
  void SetFloat(float v) { fv = v; } void SetFloatLimits(float, float) {} void SetCustomInputStrings(const char*) {} void SetCustomInputIndex(int v) { iv = v; }
  int GetInt() const { return iv; } float GetFloat() const { return (float)fv; } int GetTime() const { return iv; } unsigned GetIndex() const { return (unsigned)iv; } int GetYesNo() const { return iv; } };
struct MockSubgraph { std::string Name; int DrawStyle = 0; unsigned PrimaryColor = 0; int LineWidth = 0, DrawZeros = 0; std::vector<float> Data;
  float& operator[](int i) { if ((int)Data.size() <= i) Data.resize(i + 4096, 0.0f); return Data[i]; } };
typedef MockSubgraph& SCSubgraphRef;
struct MockArr { std::vector<float> v; float operator[](int i) const { return v[i]; } };
struct MockDT { std::vector<int> d, t; int DateAt(int i) const { return d[i]; } int TimeAt(int i) const { return t[i]; } };
struct MOrder { uint32_t id; int type, side, status; double price, qty, offset; uint32_t parent, child; };
struct s_sc;
struct Broker {
  std::vector<MOrder> ord; double pos = 0, avg = 0, cum = 0, last = 0, px = 0, prev = 0, usdPerPt = 2.0; int trades = 0; uint32_t nextId = 100;
  int date = 0, tod = 0; FILE* out = nullptr; int eDate = 0, eTod = 0; int failFlatten = 0; long fills = 0;
  MOrder* get(uint32_t id) { for (auto& o : ord) if (o.id == id) return &o; return nullptr; }
  void exec(int side, double qty, double p) {
    ++fills;
    if (pos == 0) { pos = side * qty; avg = p; eDate = date; eTod = tod; return; }
    if ((pos > 0) == (side > 0)) { avg = (avg * std::fabs(pos) + p * qty) / (std::fabs(pos) + qty); pos += side * qty; return; }
    const double q = std::fmin(qty, std::fabs(pos)); const int d = pos > 0 ? 1 : -1; const double pl = d * (p - avg) * q * usdPerPt;
    cum += pl; last = pl; pos += side * q;
    if (out) fprintf(out, "%d,%d,%d,%d,%d,%.2f,%.2f,%.0f,%.2f\n", eDate, eTod, date, tod, d, avg, p, q, pl);
    if (pos == 0) ++trades;
    if (qty > q) { pos = side * (qty - q); avg = p; eDate = date; eTod = tod; } }
  void fillParent(MOrder& o, double p) { o.status = SCT_OSC_FILLED; exec(o.side, o.qty, p);
    if (MOrder* c = get(o.child)) { c->price = p - o.side * c->offset; c->status = SCT_OSC_OPEN; } }
  void tick(int D, int T, double p) { date = D; tod = T; prev = px == 0 ? p : px; px = p;
    for (size_t n = 0; n < ord.size(); ++n) { MOrder o = ord[n]; if (o.status != SCT_OSC_OPEN || o.type != SCT_ORDERTYPE_STOP) continue;
      const bool hit = o.side > 0 ? p >= o.price : p <= o.price; if (!hit) continue;
      const bool crossed = o.side > 0 ? prev < o.price : prev > o.price; const double fp = crossed ? o.price : p;
      if (o.parent == 0) fillParent(ord[n], fp);
      else { ord[n].status = SCT_OSC_FILLED; exec(o.side, std::fmin(o.qty, std::fabs(pos)), fp); } } }
  int submit(s_SCNewOrder& n, int side, int maxPos) {
    if (std::fabs(pos) + n.OrderQuantity > maxPos + 1e-9) return -8998;
    for (auto& o : ord) if (IsWorkingOrderStatus(o.status)) return -8997;         // AllowEntryWithWorkingOrders = 0
    MOrder p{ nextId++, n.OrderType, side, SCT_OSC_OPEN, n.Price1, n.OrderQuantity, 0, 0, 0 };
    MOrder c{ nextId++, SCT_ORDERTYPE_STOP, -side, SCT_OSC_PENDING_CHILD, 0, n.OrderQuantity, n.Stop1Offset, p.id, 0 };
    const bool hasStop = n.Stop1Offset > 0; if (hasStop) p.child = c.id;
    ord.push_back(p); if (hasStop) ord.push_back(c);
    n.InternalOrderID = p.id; n.Stop1InternalOrderID = hasStop ? c.id : 0;
    if (p.type == SCT_ORDERTYPE_MARKET) fillParent(*get(p.id), px);
    return (int)n.OrderQuantity; }
  int cancelAll() { for (auto& o : ord) if (IsWorkingOrderStatus(o.status)) o.status = SCT_OSC_CANCELED; return 1; }
  int flatten() { if (failFlatten > 0) { --failFlatten; return -1; } cancelAll(); if (pos != 0) exec(pos > 0 ? -1 : 1, std::fabs(pos), px); return 1; }
};
struct s_sc {
  int SetDefaults = 0, Index = 0, ArraySize = 0, IsFullRecalculation = 0, LastCallToFunction = 0, AutoLoop = 0, GraphRegion = 0, ValueFormat = 0, DrawZeros = 0;
  int SecondsPerBar = 300, ChartNumber = 1, ServerConnectionState = 2, IsAutoTradingEnabled = 1, GlobalTradeSimulationIsOn = 1;
  uint16_t TradingIsLocked = 0; float TickSize = 0.25f, CurrencyValuePerTick = 0.5f;
  int SendOrdersToTradeService = 0, MaximumPositionAllowed = 0, SupportReversals = 0, AllowOppositeEntryWithOpposingPositionOrOrders = 0,
      SupportAttachedOrdersForTrading = 0, UseGUIAttachedOrderSetting = 0, CancelAllOrdersOnEntriesAndReversals = 0, CancelAllOrdersOnEntries = 0,
      CancelAllOrdersOnReversals = 0, AllowEntryWithWorkingOrders = 0, AllowOnlyOneTradePerBar = 0, AllowMultipleEntriesInSameDirection = 0,
      CancelAllWorkingOrdersOnExit = 0, SupportTradingScaleIn = 0, SupportTradingScaleOut = 0, MaintainTradeStatisticsAndTradesData = 0;
  SCString GraphName, StudyDescription, Symbol{ "MNQZ26" }, SelectedTradeAccount{ "Sim1" };
  MockArr Open, High, Low, Close, Volume; MockDT BaseDateTimeIn; MockSubgraph Subgraph[60]; MockInput Input[128];
  SCDateTime LatestDateTimeForLastBar, CurrentSystemDateTime; void* pp[8] = {}; Broker br; FILE* log = nullptr; long nLog[2] = {0, 0}; int tz = 0; int replay = 1;
  void*& GetPersistentPointer(int i) { return pp[i]; }
  void AddMessageToLog(const char* m, int lvl) { ++nLog[lvl ? 1 : 0]; if (log) fprintf(log, "%d %02d:%02d:%02d [%d] %s\n", br.date, br.tod / 3600, br.tod / 60 % 60, br.tod % 60, lvl, m); }
  void AddMessageToLog(const SCString& m, int lvl) { AddMessageToLog(m.GetChars(), lvl); }
  int GetBarHasClosedStatus(int i) const { return i < ArraySize - 1 ? BHCS_BAR_HAS_CLOSED : BHCS_BAR_HAS_NOT_CLOSED; }
  int ChartIsDownloadingHistoricalData(int) const { return 0; }
  int IsReplayRunning() const { return replay; }
  void GetTradePosition(s_SCPositionData& p) { p.PositionQuantity = br.pos; p.AveragePrice = br.avg; p.TotalTrades = br.trades; p.CumulativeProfitLoss = br.cum;
    p.LastTradeProfitLoss = br.last; p.WorkingOrdersExist = 0; for (auto& o : br.ord) if (IsWorkingOrderStatus(o.status)) p.WorkingOrdersExist = 1; }
  int GetOrderByOrderID(uint64_t id, s_SCTradeOrder& o) { MOrder* m = br.get((uint32_t)id); if (!m) return SCTRADING_ORDER_ERROR;
    o.InternalOrderID = m->id; o.Price1 = m->price; o.OrderStatusCode = m->status; o.OrderQuantity = m->qty; return 1; }
  int FlattenAndCancelAllOrders() { return IsFullRecalculation ? -8999 : br.flatten(); }
  int CancelAllOrders() { return IsFullRecalculation ? -8999 : br.cancelAll(); }
  double RoundToTickSize(double x) const { return std::floor(x / TickSize + 0.5) * TickSize; }
  double BuyEntry(s_SCNewOrder& o) { return IsFullRecalculation ? -8999 : br.submit(o, 1, MaximumPositionAllowed); }
  double SellEntry(s_SCNewOrder& o) { return IsFullRecalculation ? -8999 : br.submit(o, -1, MaximumPositionAllowed); }
  const char* GetTradingErrorTextMessage(int) const { return "mock rejection"; }
  static SCDateTime ConvNY(const SCDateTime& u) { return SCDateTime(u.d, u.t - (u.d == 46037 ? 5 : 4) * 3600); }
  static SCDateTime ConvUTC(const SCDateTime& u) { return u; }
  SCDateTime (*ConvertDateTimeUTCToChartTimeZone)(const SCDateTime&) = &ConvNY;
  static void TzName(SCString& s, int) { s = "mock New York"; }
  void (*GetChartTimeZoneStringRef)(SCString&, int) = &TzName;
};
typedef s_sc& SCStudyInterfaceRef;
