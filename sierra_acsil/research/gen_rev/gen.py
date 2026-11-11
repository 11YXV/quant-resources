import sys, os, re
G=os.path.dirname(os.path.abspath(__file__))
eng=open(G+'/tmpl_engine.inc').read(); trd=open(G+'/tmpl_trader.inc').read()
COMMON_FIELDS='''        int    RsiLen     = 2;                         // Wilder RSI length, day-session closes
        double VaFrac     = 0.70;                      // value area = this share of the session's volume
        double VaBin      = 1.0;                       // price bin of the volume profile, points
        int    ChaseTicks = 0;'''
S={}
S['DWR']=dict(FILE='DailyWashoutRebound_NQ_MNQ',NS='dwr',TAG='DWR',GRAPH='Daily Washout Rebound NQ MNQ',DEF_MAXTR='1',
 DESC='Cross-day mean reversion on MNQ: when the 2-period RSI of the last day-session closes is below 30 (a short-term washout), buy the day-session open; attached stop 0.75 x the 14-session average range; no target; flat at 15:55 New York. Long only as measured. MNQ 5-minute chart with the overnight session loaded, New York time.',
 BAND_STYLE='DRAWSTYLE_IGNORE',L_STYLE='DRAWSTYLE_IGNORE',UB_NAME='(unused)',LB_NAME='(unused)',L1_NAME='RSI of day-session closes (stored)',L2_NAME='(unused)',
 INPUT_ENUM='        IN_RSI_LEN = 0, IN_RSI_BUY, IN_RSI_SELL, IN_STOP_ADR,',
 PARAM_FIELDS='''        double RsiBuy     = 30.0;                      // long when RSI is below this
        double RsiSell    = 0.0;                       // short when RSI is above this; 0 = off (measured: off)
        double StopAdr    = 0.75;                      // attached stop, x average day range
'''+COMMON_FIELDS,
 READ_PARAMS='''        P.RsiLen     = R.GI(IN_RSI_LEN, 2, 14, D.RsiLen);
        P.RsiBuy     = R.GF(IN_RSI_BUY, 1.0, 99.0, D.RsiBuy);
        P.RsiSell    = R.GF(IN_RSI_SELL, 0.0, 99.0, D.RsiSell);
        P.StopAdr    = R.GF(IN_STOP_ADR, 0.05, 5.0, D.StopAdr);
        P.VaBin      = sc.TickSize > 0.0f ? 4.0 * sc.TickSize : 1.0;''',
 DEF_INPUTS='''        DefIN(IN_RSI_LEN,     "RSI length, day-session closes (measured 2)", D.RsiLen, 2, 14);
        DefFL(IN_RSI_BUY,     "Buy the open when RSI is below (measured 30; positive 20-40)", D.RsiBuy, 1.0, 99.0);
        DefFL(IN_RSI_SELL,    "Sell the open when RSI is above (0 = off; measured OFF, the short side did not hold up)", D.RsiSell, 0.0, 99.0);
        DefFL(IN_STOP_ADR,    "Attached stop, x average day range (measured 0.75)", D.StopAdr, 0.05, 5.0);''')
S['VAR']=dict(FILE='ValueAreaReentry_NQ_MNQ',NS='var',TAG='VAR',GRAPH='Value Area Reentry NQ MNQ',DEF_MAXTR='1',
 DESC='Auction-market mean reversion on MNQ (the 80 percent rule): the day session opens outside the prior session value area (70 percent of its volume); the first 30-minute close from 10:00 to 12:30 that is back inside the value area is traded towards the far side (short if the open was above, long if below); attached stop 0.20 x the 14-session average range; no target; flat at 15:55 New York. MNQ 5-minute chart, New York time.',
 BAND_STYLE='DRAWSTYLE_DASH',L_STYLE='DRAWSTYLE_DASH',UB_NAME='Prior value area high',LB_NAME='Prior value area low',L1_NAME='Prior point of control',L2_NAME='(unused)',
 INPUT_ENUM='        IN_VA_FRAC = 0, IN_VA_BIN, IN_MARK_MIN, IN_FIRST_MARK, IN_LAST_ENTRY, IN_STOP_ADR,',
 PARAM_FIELDS='''        int    VaBinTicks = 4;                         // profile bin, ticks (4 = 1 point)
        int    MarkMin    = 30;                        // decision marks, minutes from the open
        int    FirstMark  = 10 * 3600;                 // first decision (bar close time)
        int    LastEntry  = 12 * 3600 + 30 * 60;       // last decision that may open a trade
        double StopAdr    = 0.20;                      // attached stop, x average day range
'''+COMMON_FIELDS,
 READ_PARAMS='''        P.VaFrac     = R.GF(IN_VA_FRAC, 0.30, 0.95, D.VaFrac);
        P.VaBinTicks = R.GI(IN_VA_BIN, 1, 400, D.VaBinTicks);
        P.VaBin      = sc.TickSize > 0.0f ? P.VaBinTicks * (double)sc.TickSize : 1.0;
        P.MarkMin    = R.GI(IN_MARK_MIN, 5, 120, D.MarkMin);
        P.FirstMark  = R.GT(IN_FIRST_MARK, D.FirstMark);
        P.LastEntry  = R.GT(IN_LAST_ENTRY, D.LastEntry);
        P.StopAdr    = R.GF(IN_STOP_ADR, 0.02, 5.0, D.StopAdr);''',
 DEF_INPUTS='''        DefFL(IN_VA_FRAC,     "Value area, share of the prior session volume (measured 0.70)", D.VaFrac, 0.30, 0.95);
        DefIN(IN_VA_BIN,      "Volume profile bin, ticks (measured 4 = 1 point)", D.VaBinTicks, 1, 400);
        DefIN(IN_MARK_MIN,    "Decision marks every, minutes from the open (measured 30; a multiple of the bar size)", D.MarkMin, 5, 120);
        DefTM(IN_FIRST_MARK,  "First decision at (bar close, New York; measured 10:00)", D.FirstMark);
        DefTM(IN_LAST_ENTRY,  "Last decision that may open a trade (measured 12:30)", D.LastEntry);
        DefFL(IN_STOP_ADR,    "Attached stop, x average day range (measured 0.20)", D.StopAdr, 0.02, 5.0);''')
S['CEB']=dict(FILE='CompressionExpansionBreakout_NQ_MNQ',NS='ceb',TAG='CEB',GRAPH='Compression Expansion Breakout NQ MNQ',DEF_MAXTR='1',
 DESC='Volatility-cycle breakout on MNQ: only on the day after a narrow-range day (the prior day-session range is the smallest of the last 4 full sessions, NR4), the first 5-minute close outside the 30-minute opening range before 13:00 is followed in its direction; attached stop 0.25 x the 14-session average range; no target; flat at 15:55 New York. MNQ 5-minute chart, New York time.',
 BAND_STYLE='DRAWSTYLE_DASH',L_STYLE='DRAWSTYLE_IGNORE',UB_NAME='Opening range high (narrow-range days)',LB_NAME='Opening range low (narrow-range days)',L1_NAME='Narrow-range count (stored)',L2_NAME='(unused)',
 INPUT_ENUM='        IN_NR_DAYS = 0, IN_OR_MIN, IN_LAST_ENTRY, IN_STOP_ADR,',
 PARAM_FIELDS='''        int    NrDays     = 4;                         // yesterday's range is the smallest of this many full sessions
        int    OrMinutes  = 30;                        // opening range length
        int    LastEntry  = 13 * 3600;                 // last bar close that may open a trade
        double StopAdr    = 0.25;                      // attached stop, x average day range
'''+COMMON_FIELDS,
 READ_PARAMS='''        P.NrDays     = R.GI(IN_NR_DAYS, 1, 20, D.NrDays);
        P.OrMinutes  = R.GI(IN_OR_MIN, 5, 180, D.OrMinutes);
        P.LastEntry  = R.GT(IN_LAST_ENTRY, D.LastEntry);
        P.StopAdr    = R.GF(IN_STOP_ADR, 0.02, 5.0, D.StopAdr);
        P.VaBin      = sc.TickSize > 0.0f ? 4.0 * sc.TickSize : 1.0;''',
 DEF_INPUTS='''        DefIN(IN_NR_DAYS,     "Narrow-range test: prior range is the smallest of, sessions (measured 4; 1 = every day)", D.NrDays, 1, 20);
        DefIN(IN_OR_MIN,      "Opening range, minutes (measured 30; a multiple of the bar size)", D.OrMinutes, 5, 180);
        DefTM(IN_LAST_ENTRY,  "Last bar close that may open a trade (New York; measured 13:00)", D.LastEntry);
        DefFL(IN_STOP_ADR,    "Attached stop, x average day range (measured 0.25)", D.StopAdr, 0.02, 5.0);''')
S['EDF']=dict(FILE='EndOfDayFlow_NQ_MNQ',NS='edf',TAG='EDF',GRAPH='End Of Day Flow NQ MNQ',DEF_MAXTR='1',
 DESC='End-of-day rebalancing flow on MNQ: at 15:00 New York, if the price is at least 1.0 percent away from the prior day-session close, trade in the direction of the move (leveraged ETF rebalancing and short-gamma option hedging buy strength and sell weakness into the close); attached stop 0.40 x the 14-session average range; no target; flat at 15:55 New York. MNQ 5-minute chart, New York time.',
 BAND_STYLE='DRAWSTYLE_DASH',L_STYLE='DRAWSTYLE_IGNORE',UB_NAME='Prior close + min move',LB_NAME='Prior close - min move',L1_NAME='Move from prior close at the decision, % (stored)',L2_NAME='(unused)',
 INPUT_ENUM='        IN_MIN_MOVE = 0, IN_ENTRY_MARK, IN_STOP_ADR,',
 PARAM_FIELDS='''        double MinMovePct = 1.0;                       // |decision close / prior close - 1|, percent
        int    EntryMark  = 15 * 3600;                 // decision bar close time
        double StopAdr    = 0.40;                      // attached stop, x average day range
'''+COMMON_FIELDS,
 READ_PARAMS='''        P.MinMovePct = R.GF(IN_MIN_MOVE, 0.05, 20.0, D.MinMovePct);
        P.EntryMark  = R.GT(IN_ENTRY_MARK, D.EntryMark);
        P.StopAdr    = R.GF(IN_STOP_ADR, 0.02, 5.0, D.StopAdr);
        P.VaBin      = sc.TickSize > 0.0f ? 4.0 * sc.TickSize : 1.0;''',
 DEF_INPUTS='''        DefFL(IN_MIN_MOVE,    "Min move from the prior day-session close at the decision, % (measured 1.0; positive 0.75-1.5)", D.MinMovePct, 0.05, 20.0);
        DefTM(IN_ENTRY_MARK,  "Decision at (bar close, New York; measured 15:00; a multiple of the bar size)", D.EntryMark);
        DefFL(IN_STOP_ADR,    "Attached stop, x average day range (measured 0.40; similar 0.40-0.60)", D.StopAdr, 0.02, 5.0);''')
out=sys.argv[1]
only=sys.argv[2:] 
for k,s in S.items():
    if only and k not in only: continue
    s=dict(s); s['NSU']=s['NS'].upper(); s['STRATEGY']=open(f"{G}/strat_{s['NS']}.inc").read().rstrip('\n')
    hdr=open(f"{G}/doc_{s['NS']}.txt").read().rstrip('\n') if os.path.exists(f"{G}/doc_{s['NS']}.txt") else '/* header */'
    body=eng+trd
    for a,b in s.items(): body=body.replace('@'+a+'@',b)
    left=re.findall(r'@[A-Z_0-9]+@',body); assert not left,left
    src=f'#include "sierrachart.h"\n\nSCDLLName("{s["FILE"]}")\n\n{hdr}\n\n{body}'
    open(f"{out}/{s['FILE']}.cpp",'w').write(src); print(s['FILE'],len(src.splitlines()),'lines')
