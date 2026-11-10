import sys, os
G=os.path.dirname(os.path.abspath(__file__))
eng=open(G+'/tmpl_engine.inc').read(); trd=open(G+'/tmpl_trader.inc').read()
S={}
S['NBM']=dict(FILE='NoiseBandMomentum_NQ_MNQ',NS='nbm',TAG='NBM',GRAPH='Noise Band Momentum NQ MNQ',DEF_MAXTR='3',
 DESC='Intraday momentum on MNQ: at each 30-minute mark, long when the close is above the upper noise band (open / prior close +- the 14-session average move for that time of day) and above session VWAP, short mirrored; exit when a 30-minute close is back through max(band, VWAP); attached disaster stop; flat at 15:55 New York. MNQ 5-minute chart, New York time.',
 BAND_STYLE='DRAWSTYLE_LINE',L_STYLE='DRAWSTYLE_IGNORE',L1_NAME='(unused)',L2_NAME='(unused)',
 INPUT_ENUM='        IN_BAND_MULT = 0, IN_USE_VWAP, IN_FIRST_MARK, IN_LAST_ENTRY, IN_CAT_STOP,',
 PARAM_FIELDS='''        double BandMult   = 1.0;                       // x the average |move from the open| at that time of day
        int    UseVwap    = 1;
        int    FirstMark  = 10 * 3600;                 // first 30-minute decision (bar close time)
        int    LastEntry  = 15 * 3600 + 30 * 60;       // last decision that may open a trade
        double CatStop    = 0.5;                       // attached disaster stop, x average day range
        int    ChaseTicks = 0;''',
 READ_PARAMS='''        P.BandMult   = R.GF(IN_BAND_MULT, 0.1, 10.0, D.BandMult);
        P.UseVwap    = sc.Input[IN_USE_VWAP].GetYesNo() ? 1 : 0;
        P.FirstMark  = R.GT(IN_FIRST_MARK, D.FirstMark);
        P.LastEntry  = R.GT(IN_LAST_ENTRY, D.LastEntry);
        P.CatStop    = R.GF(IN_CAT_STOP, 0.05, 5.0, D.CatStop);''',
 DEF_INPUTS='''        DefFL(IN_BAND_MULT,   "Noise band width, x the average move for the time of day (measured 1.0)", D.BandMult, 0.1, 10.0);
        DefYN(IN_USE_VWAP,    "Use session VWAP in the entry and the trailing exit (measured Yes)", D.UseVwap);
        DefTM(IN_FIRST_MARK,  "First 30-minute decision at (bar close, New York; measured 10:00)", D.FirstMark);
        DefTM(IN_LAST_ENTRY,  "Last decision that may open a trade (measured 15:30)", D.LastEntry);
        DefFL(IN_CAT_STOP,    "Attached disaster stop, x average day range (measured 0.5)", D.CatStop, 0.05, 5.0);''')
S['GOD']=dict(FILE='GapOpeningDrive_NQ_MNQ',NS='god',TAG='GOD',GRAPH='Gap Opening Drive NQ MNQ',DEF_MAXTR='1',
 DESC='Opening drive on MNQ: when the day session opens away from the prior close, a stop entry one tick beyond the first 5-minute range on the side of the gap only; a break of the other side first cancels the day; attached stop 0.10 x the 14-session average range; no target; flat at 15:55 New York. MNQ 5-minute chart, New York time.',
 BAND_STYLE='DRAWSTYLE_IGNORE',L_STYLE='DRAWSTYLE_DASH',L1_NAME='Entry trigger (opening range, gap side)',L2_NAME='Cancel level (other side)',
 INPUT_ENUM='        IN_OR_MIN = 0, IN_MIN_GAP, IN_MAX_GAP, IN_STOP_ADR, IN_VALID_MIN, IN_CHASE,',
 PARAM_FIELDS='''        int    OrMinutes  = 5;                         // opening range length
        double MinGap     = 0.05;                      // |open - prior close|, x average day range
        double MaxGap     = 0.0;                       // 0 = off
        double StopAdr    = 0.10;                      // attached stop, x average day range
        int    ValidMin   = 60;                        // the entry rests until this many minutes after the open
        int    ChaseTicks = 4;                         // price already through the trigger: market if within this
        double BandMult   = 1.0;                       // display only''',
 READ_PARAMS='''        P.OrMinutes  = R.GI(IN_OR_MIN, 1, 120, D.OrMinutes);
        P.MinGap     = R.GF(IN_MIN_GAP, 0.0, 5.0, D.MinGap);
        P.MaxGap     = R.GF(IN_MAX_GAP, 0.0, 20.0, D.MaxGap);
        P.StopAdr    = R.GF(IN_STOP_ADR, 0.01, 5.0, D.StopAdr);
        P.ValidMin   = R.GI(IN_VALID_MIN, 0, 390, 0);
        if (P.ValidMin == 0) P.ValidMin = D.ValidMin;              // 0 = the default (lessons #64)
        if (P.ValidMin <= P.OrMinutes) { ++P.Fallbacks; P.ValidMin = P.OrMinutes + 55; }
        P.ChaseTicks = R.GI(IN_CHASE, 0, 100, D.ChaseTicks);''',
 DEF_INPUTS='''        DefIN(IN_OR_MIN,      "Opening range, minutes (measured 5; a multiple of the bar size)", D.OrMinutes, 1, 120);
        DefFL(IN_MIN_GAP,     "Min gap: |open - prior close|, x average day range (measured 0.05)", D.MinGap, 0.0, 5.0);
        DefFL(IN_MAX_GAP,     "Max gap, x average day range (0 = off)", D.MaxGap, 0.0, 20.0);
        DefFL(IN_STOP_ADR,    "Attached stop, x average day range (measured 0.10)", D.StopAdr, 0.01, 5.0);
        DefIN(IN_VALID_MIN,   "Entry order rests until, minutes after the open (0 = 60 default)", 0, 0, 390);
        DefIN(IN_CHASE,       "Price already through the trigger: enter at market if within, ticks", D.ChaseTicks, 0, 100);''')
S['TVP']=dict(FILE='TrendDayVwapPullback_NQ_MNQ',NS='tvp',TAG='TVP',GRAPH='Trend Day VWAP Pullback NQ MNQ',DEF_MAXTR='1',
 DESC='Trend-day pullback on MNQ: once a 30-minute close is beyond a noise band (open / prior close +- the 14-session average move for that time of day) the day has a trend; the first 15-minute block that trades back to session VWAP and closes on the trend side is bought (sold) at the next open; attached stop 0.15 x the average day range; no target; flat at 15:55 New York. MNQ 5-minute chart, New York time.',
 BAND_STYLE='DRAWSTYLE_LINE',L_STYLE='DRAWSTYLE_IGNORE',L1_NAME='Day trend +1/-1 (stored)',L2_NAME='(unused)',
 INPUT_ENUM='        IN_BAND_MULT = 0, IN_CONFIRM, IN_STOP_ADR, IN_FIRST_MARK, IN_LAST_ENTRY,',
 PARAM_FIELDS='''        double BandMult   = 1.0;                       // x the average |move from the open| at that time of day
        int    ConfirmBars = 3;                        // chart bars per confirmation block (3 x 5 min)
        double StopAdr    = 0.15;                      // attached stop, x average day range
        int    FirstMark  = 10 * 3600;                 // first decision (bar close time)
        int    LastEntry  = 15 * 3600;                 // last decision that may open a trade
        int    ChaseTicks = 0;''',
 READ_PARAMS='''        P.BandMult   = R.GF(IN_BAND_MULT, 0.1, 10.0, D.BandMult);
        P.ConfirmBars = R.GI(IN_CONFIRM, 1, 12, D.ConfirmBars);
        P.StopAdr    = R.GF(IN_STOP_ADR, 0.01, 5.0, D.StopAdr);
        P.FirstMark  = R.GT(IN_FIRST_MARK, D.FirstMark);
        P.LastEntry  = R.GT(IN_LAST_ENTRY, D.LastEntry);''',
 DEF_INPUTS='''        DefFL(IN_BAND_MULT,   "Noise band width, x the average move for the time of day (measured 1.0)", D.BandMult, 0.1, 10.0);
        DefIN(IN_CONFIRM,     "Confirmation block, chart bars (measured 3 = 15 minutes)", D.ConfirmBars, 1, 12);
        DefFL(IN_STOP_ADR,    "Attached stop, x average day range (measured 0.15)", D.StopAdr, 0.01, 5.0);
        DefTM(IN_FIRST_MARK,  "First decision at (bar close, New York; measured 10:00)", D.FirstMark);
        DefTM(IN_LAST_ENTRY,  "Last decision that may open a trade (measured 15:00)", D.LastEntry);''')
out=sys.argv[1]
for k,s in S.items():
    s=dict(s); s['NSU']=s['NS'].upper(); s['STRATEGY']=open(f"{G}/strat_{s['NS']}.inc").read().rstrip('\n')
    hdr=open(f"{G}/doc_{s['NS']}.txt").read().rstrip('\n') if os.path.exists(f"{G}/doc_{s['NS']}.txt") else '/* documentation pending */'
    body=eng+trd
    for a,b in s.items(): body=body.replace('@'+a+'@',b)
    assert '@' not in body.replace('@',' ') or True
    import re
    left=re.findall(r'@[A-Z_0-9]+@',body); assert not left,left
    src=f'#include "sierrachart.h"\n\nSCDLLName("{s["FILE"]}")\n\n{hdr}\n\n{body}'
    open(f"{out}/{s['FILE']}.cpp",'w').write(src); print(s['FILE'],len(src.splitlines()),'lines')
