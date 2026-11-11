import numpy as np, pandas as pd
def load(s):
    z=np.load(f'/home/user/work/{s}.npz',allow_pickle=True)
    d={k:z[k] for k in z.files}
    d['td']=pd.to_datetime(d['td'])
    return d
NQ=load('NQ')
TD=NQ['td']; N=len(TD)
C=NQ['c']; O=NQ['o']; H=NQ['h']; Lo=NQ['l']; V=np.nan_to_num(NQ['v'])
nb=np.isfinite(C)
RTHN=nb[:,930:1320].sum(1); ONN=nb[:,:930].sum(1)
# ffilled close (within day); px[k] = last known price at END of minute k
CF=pd.DataFrame(C).ffill(axis=1).values
HF=np.where(nb,H,CF); LF=np.where(nb,Lo,CF); OF=np.where(nb,O,CF)
FULL=(RTHN>=380)             # full RTH day
GOODON=(ONN>=700)
# previous trading-day refs (same contract only, else nan)
SYM=NQ['sym']
def prevref(arr):
    out=np.full(N,np.nan); out[1:]=arr[:-1]
    ok=np.zeros(N,bool); ok[1:]=(SYM[1:]==SYM[:-1])&((TD[1:]-TD[:-1]).days<=4)
    out[~ok]=np.nan; return out
RC=CF[:,1319]   # RTH close 16:00
RO=OF[:,930]
RH=np.nanmax(HF[:,930:1320],1); RL=np.nanmin(LF[:,930:1320],1)
RNG=np.where(FULL,RH-RL,np.nan)
ADR=pd.Series(RNG).rolling(14,min_periods=10).mean().shift(1).values
PC=prevref(RC); PH=prevref(RH); PL=prevref(RL); PO=prevref(RO)
IS=TD<'2022-01-01'; OOS=~IS
YR=TD.year.values
COST=1.25  # points per round trip per contract ($2.50)
def k(hh,mm): return ((hh*60+mm)-1080)%1440
def rep(pts,days,label='',cost=COST,mult=2.0,show=True):
    """pts: gross points per trade; days: day index per trade"""
    pts=np.asarray(pts,float); days=np.asarray(days,int)
    if len(pts)==0:
        if show: print(f'{label:34s} no trades'); 
        return None
    net=(pts-cost)*mult; dt=TD[days]; out={}
    for nm,m in (('ALL',np.ones(len(pts),bool)),('IS',IS[days]),('OOS',OOS[days])):
        x=net[m]
        if len(x)==0: out[nm]=None; continue
        s=pd.Series(x,index=dt[m]); mo=s.groupby(s.index.to_period('M')).sum()
        mo=mo.reindex(pd.period_range(TD[IS if nm=='IS' else (OOS if nm=='OOS' else slice(None))][0],TD[IS if nm=='IS' else (OOS if nm=='OOS' else slice(None))][-1],freq='M'),fill_value=0)
        eq=np.cumsum(x); dd=(eq-np.maximum.accumulate(eq)).min()
        w=x[x>0].sum(); l=-x[x<=0].sum()
        t=x.mean()/x.std()*np.sqrt(len(x)) if len(x)>2 and x.std()>0 else 0
        out[nm]=dict(n=len(x),net=x.sum(),mo=mo.mean(),wr=(x>0).mean(),pf=w/max(l,1e-9),exp=x.mean(),dd=dd,t=t,posm=(mo>0).mean())
    if show:
        a=out
        f=lambda v: 'none' if v is None else f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
        print(f"{label:34s} IS {f(a['IS'])} | OOS {f(a['OOS'])}")
    return out
def sim(d,ek,side,stop=None,tgt=None,xk=1314,entry=None):
    """enter at close of minute ek (or given price), manage on minute bars ek+1..xk; stop-first. returns pts, exit_k"""
    e=CF[d,ek] if entry is None else entry
    h=HF[d,ek+1:xk+1]; l=LF[d,ek+1:xk+1]
    if side>0:
        sh=np.nonzero(l<=e-stop)[0] if stop else []
        th=np.nonzero(h>=e+tgt)[0] if tgt else []
    else:
        sh=np.nonzero(h>=e+stop)[0] if stop else []
        th=np.nonzero(l<=e-tgt)[0] if tgt else []
    si=sh[0] if len(sh) else 10**9; ti=th[0] if len(th) else 10**9
    if si==10**9 and ti==10**9: return side*(CF[d,xk]-e), xk
    if si<=ti: return -stop, ek+1+si
    return tgt, ek+1+ti
