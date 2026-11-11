# batch-5 screening helpers: back-adjusted data, engine-like fills (ref2), daily tables for NQ/ES/YM
import numpy as np, pandas as pd, os
import ref2 as r
r.prep()
def daily(mkt):
    z=np.load(f'/home/user/work/{mkt}adj.npz',allow_pickle=True)
    td=pd.to_datetime(z['td']); c=z['c'][:,930:1320]; o=z['o'][:,930:1320]; h=z['h'][:,930:1320]; l=z['l'][:,930:1320]
    hs=np.isfinite(c); cf=pd.DataFrame(c).ffill(axis=1).values
    first=np.argmax(hs,1)
    df=pd.DataFrame(dict(o=o[np.arange(len(td)),first],c=cf[:,-1],h=np.nanmax(h,1),l=np.nanmin(l,1),n=hs.sum(1),
                         c1000=cf[:,29],c1100=cf[:,89],c1200=cf[:,149],c1400=cf[:,269],c1500=cf[:,329]),index=td)
    df=df[df.n>=133]          # >=35% of the session
    df['full']=df.n>=380
    return df
NQd=daily('NQ'); ESd=daily('ES'); YMd=daily('YM')
DAYS={D.td:D for D in r.days}
def ok(D): return D.ok and D.bok[0]
def trade_m(D,m,side,dist,xm=385,tgt=None):
    """market entry at the open of RTH minute m (0 = 09:30), stop-first on 1-min bars, exit at the open of minute xm (385 = 15:55)"""
    d=D.i; mins=np.nonzero(r.has[d])[0]; mins=mins[mins>=m]
    if len(mins)==0 or mins[0]>=xm: return None
    e=r.o1[d,mins[0]]; dist=r.rnd(dist) if dist else None; stop=e-side*dist if dist else None
    for q in mins:
        if q>=xm: return (D.i,side,q,q,e,side*(r.o1[d,q]-e),'EOD')
        if dist and ((side>0 and r.l1[d,q]<=stop) or (side<0 and r.h1[d,q]>=stop)):
            x=stop if q==mins[0] else (min(stop,r.o1[d,q]) if side>0 else max(stop,r.o1[d,q])); return (D.i,side,mins[0],q,e,side*(x-e),'STOP')
        if tgt and ((side>0 and r.h1[d,q]>=e+tgt) or (side<0 and r.l1[d,q]<=e-tgt)): return (D.i,side,mins[0],q,e,tgt,'TGT')
    return (D.i,side,mins[0],mins[-1],e,side*(r.c1[d,mins[-1]]-e),'EOD')
def L(tr,lbl):
    tr=[t for t in tr if t]
    if len(tr)<10: print(f'{lbl:44s} few ({len(tr)})'); return None
    t=r.stats(tr); r.line(t,lbl); return t
