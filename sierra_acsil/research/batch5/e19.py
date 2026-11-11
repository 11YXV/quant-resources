import os, warnings; warnings.filterwarnings('ignore')
from S5 import *
adrp=pd.Series({D.td:D.adr/D.dOpen if D.adr>0 else np.nan for D in r.days})
pct=adrp.rolling(250,min_periods=120).apply(lambda x:(x[:-1]<x[-1]).mean(),raw=True)
def EOD(t=329,pctmove=None,z=None,regime=None,stop=0.25,xm=385):
    tr=[]
    for D in r.days:
        if not ok(D) or not r.has[D.i,t]: continue
        if regime is not None and not (pct.get(D.td,np.nan)>=regime): continue
        mv=r.c1[D.i,t]-D.prevClose
        if pctmove is not None and abs(mv/D.prevClose)<pctmove: continue
        if z is not None and abs(mv/D.adr)<z: continue
        tr.append(trade_m(D,t+1,int(np.sign(mv)),stop*D.adr if stop else None,xm=xm))
    return [x for x in tr if x]
print(os.environ.get('MKT','NQ'),'-- absolute move from prior close to 15:00 (percent)')
for p in (0.005,0.01,0.015,0.02):
    t=L(EOD(pctmove=p),f'|move| > {p*100:.1f}%')
print('-- volatility regime (avg range / price, percentile of last 250 sessions), |move|>0.5 avg range')
for g in (0.0,0.5,0.67,0.8):
    t=L(EOD(z=0.5,regime=g),f'regime >= {g}')
print('-- quarterly net, base rule (|move|>0.5 avg range, 15:00 -> 15:55)')
t=r.stats(EOD(z=0.5)); q=t.groupby(t.date.dt.to_period('Q')).net.sum().round(0).astype(int)
print(' '.join(f'{k}:{v}' for k,v in q.items()))
