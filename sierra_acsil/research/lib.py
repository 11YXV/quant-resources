import numpy as np, pandas as pd, pickle
import os
days=pickle.load(open(os.environ.get('DAYS','days.pkl'),'rb'))
N=len(days)
TD=pd.to_datetime([d['td'] for d in days])
O=np.array([d['o'] for d in days]);H=np.array([d['h'] for d in days]);L=np.array([d['l'] for d in days]);C=np.array([d['c'] for d in days]);V=np.array([d['v'] for d in days])
SHORT=np.array([d['short'] for d in days])
PC=np.array([d['pc'] for d in days]);PH=np.array([d['ph'] for d in days]);PL=np.array([d['pl'] for d in days])
ONH=np.array([d['onh'] for d in days]);ONL=np.array([d['onl'] for d in days])
DO=O[:,0]
# daily RTH range, 14d ATR-like (prior days only)
RNG=H.max(1)-L.min(1)
ADR=pd.Series(RNG).rolling(14).mean().shift(1).values
# cumulative VWAP using typical price
TP=(H+L+C)/3
VW=np.cumsum(TP*np.maximum(V,1),1)/np.cumsum(np.maximum(V,1),1)
# noise profile: avg |c_t/open-1| over prior 14 days
MOVE=np.abs(C/DO[:,None]-1)
SIG=pd.DataFrame(MOVE).rolling(14).mean().shift(1).values
COST=float(os.environ.get('COST','1.25'))
def stats(tr,cost=COST,mult=2.0,label='',show=True,split='2022-01-01'):
    """tr: DataFrame with day(int), pts (gross points per contract), optional qty"""
    if len(tr)==0: print(label,'no trades'); return None
    t=tr.copy(); q=t.qty if 'qty' in t else 1
    t['net']=(t.pts-cost)*mult*q
    t['date']=TD[t.day.values]
    out={}
    for nm,s in (('ALL',t),('IS',t[t.date<split]),('OOS',t[t.date>=split])):
        if len(s)==0: continue
        m=s.groupby(s.date.dt.to_period('M')).net.sum()
        allm=pd.period_range(s.date.min(),s.date.max(),freq='M'); m=m.reindex(allm,fill_value=0)
        eq=s.net.cumsum(); dd=(eq-eq.cummax()).min()
        w=s.net[s.net>0]; l=s.net[s.net<=0]
        dly=s.groupby('date').net.sum()
        sh=dly.mean()/dly.std()*np.sqrt(252*len(dly)/max(1,(s.date.max()-s.date.min()).days*252/365)) if len(dly)>2 else 0
        out[nm]=dict(n=len(s),net=s.net.sum(),mo=m.mean(),med=m.median(),posm=(m>0).mean(),wr=(s.net>0).mean(),pf=w.sum()/max(1e-9,-l.sum()),exp=s.net.mean(),dd=dd,sh=sh,worst=m.min())
    if show:
        for k,v in out.items():
            print(f"{label:28s} {k:3s} n={v['n']:5d} net={v['net']:9.0f} mo={v['mo']:7.0f} med={v['med']:7.0f} +m={v['posm']:.2f} wr={v['wr']:.2f} pf={v['pf']:.2f} exp={v['exp']:6.1f} dd={v['dd']:8.0f} sh={v['sh']:.2f} worstM={v['worst']:.0f}")
    return out
