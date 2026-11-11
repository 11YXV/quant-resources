# reference models for the three candidates, on 1-min data; decisions as a 5-min chart would make them
import numpy as np, pandas as pd, sys, os
MKT=os.environ.get('MKT','NQ')
import L
if MKT=='ES':
    E=L.load('ES')
    # rebind L's arrays to ES
    L.NQ=E; L.TD=E['td']; L.N=len(L.TD)
    L.C=E['c']; L.O=E['o']; L.H=E['h']; L.Lo=E['l']; L.V=np.nan_to_num(E['v'])
    L.nb=np.isfinite(L.C); L.RTHN=L.nb[:,930:1320].sum(1); L.ONN=L.nb[:,:930].sum(1)
    L.CF=pd.DataFrame(L.C).ffill(axis=1).values
    L.HF=np.where(L.nb,L.H,L.CF); L.LF=np.where(L.nb,L.Lo,L.CF); L.OF=np.where(L.nb,L.O,L.CF)
    L.FULL=L.RTHN>=380; L.SYM=E['sym']
    L.RC=L.CF[:,1319]; L.RO=L.OF[:,930]
    L.RH=np.nanmax(np.where(L.FULL[:,None],L.HF[:,930:1320],0),1); L.RL=np.nanmin(np.where(L.FULL[:,None],L.LF[:,930:1320],9e9),1)
    L.RNG=np.where(L.FULL,L.RH-L.RL,np.nan)
    L.ADR=pd.Series(L.RNG).rolling(14,min_periods=10).mean().shift(1).values
    L.PC=L.prevref(L.RC); L.PH=L.prevref(L.RH); L.PL=L.prevref(L.RL)
    L.IS=L.TD<'2022-01-01'; L.OOS=~L.IS
from L import k
g=L
def daily_rsi(n=2):
    r1=pd.Series(np.where(np.isfinite(g.PC)&g.FULL,g.RC/g.PC-1,0.0))
    idx=(1+r1).cumprod(); d=idx.diff()
    up=d.clip(lower=0).ewm(alpha=1/n,adjust=False).mean(); dn=(-d.clip(upper=0)).ewm(alpha=1/n,adjust=False).mean()
    return (100-100/(1+up/dn)).shift(1).values
def simx(d,ek,side,stop,xk,entry=None,tgt=None):
    e=g.CF[d,ek] if entry is None else entry
    h=g.HF[d,ek+1:xk+1]; l=g.LF[d,ek+1:xk+1]
    if side>0:
        sh=np.nonzero(l<=e-stop)[0] if stop else []; th=np.nonzero(h>=e+tgt)[0] if tgt else []
    else:
        sh=np.nonzero(h>=e+stop)[0] if stop else []; th=np.nonzero(l<=e-tgt)[0] if tgt else []
    si=sh[0] if len(sh) else 10**9; ti=th[0] if len(th) else 10**9
    if si==10**9 and ti==10**9: return side*(g.CF[d,xk]-e), xk
    if si<=ti: return -stop, ek+1+si
    return tgt, ek+1+ti
def base(): return np.nonzero(g.FULL&np.isfinite(g.ADR)&np.isfinite(g.PC))[0]
def DWR(th=20,stop=0.75,ek=930,xk=1314,n=2):
    R=daily_rsi(n); out=[]
    for d in base():
        if not R[d]<th: continue
        e=g.OF[d,ek] if ek==930 else g.CF[d,ek]
        p,x=simx(d,ek if ek>930 else 930,1,stop*g.ADR[d] if stop else None,xk,entry=e)
        out.append((d,1,ek,x,e,p))
    return out
def profile(d,frac=0.70,bar=1):
    h=g.HF[d,930:1320]; l=g.LF[d,930:1320]; v=g.V[d,930:1320]
    if bar>1:
        h=h.reshape(-1,bar).max(1); l=l.reshape(-1,bar).min(1); v=v.reshape(-1,bar).sum(1)
    lo=np.floor(l.min()); hi=np.ceil(h.max()); bs=1.0 if MKT=='NQ' else 0.25
    bs=max(bs,round((hi-lo)/200/bs)*bs)
    nbin=int((hi-lo)/bs)+1; vol=np.zeros(nbin)
    a=((l-lo)/bs).astype(int); b=((h-lo)/bs).astype(int)
    for i in range(len(v)): vol[a[i]:b[i]+1]+=v[i]/(b[i]-a[i]+1)
    p=vol.argmax(); tot=vol.sum(); acc=vol[p]; i=j=p
    while acc<frac*tot:
        up=vol[j+1] if j+1<nbin else -1; dn=vol[i-1] if i>0 else -1
        if up<0 and dn<0: break
        if up>=dn: j+=1; acc+=up
        else: i-=1; acc+=dn
    return lo+(p+0.5)*bs, lo+(j+1)*bs, lo+i*bs
_vp={}
def VA(frac=0.70,bar=1):
    key=(frac,bar,MKT)
    if key not in _vp:
        poc=np.full(g.N,np.nan); vah=poc.copy(); val=poc.copy()
        for d in range(g.N):
            if g.FULL[d]: poc[d],vah[d],val[d]=profile(d,frac,bar)
        _vp[key]=(g.prevref(poc),g.prevref(vah),g.prevref(val))
    return _vp[key]
def VAR(step=30,nclose=1,stop=0.2,last=k(13,0),xk=1314,frac=0.70,bar=5,mingap=0.0,first=930):
    POC,VAH,VAL=VA(frac,bar); out=[]
    for d in base():
        if not np.isfinite(VAH[d]): continue
        o=g.RO[d]
        if VAL[d]-mingap*g.ADR[d]<=o<=VAH[d]+mingap*g.ADR[d]: continue
        side=-1 if o>VAH[d] else 1; cnt=0
        for kk in range(930+step-1,last,step):
            p=g.CF[d,kk]
            if VAL[d]<p<VAH[d]: cnt+=1
            else: cnt=0
            if cnt>=nclose and kk>=first:
                st=stop*g.ADR[d]
                r,x=simx(d,kk,side,st,xk); out.append((d,side,kk,x,p,r)); break
    return out
def PIF(th=0.2,s0=k(8,30)-1,xk=k(10,0)-1,stop=0.3,ek=930):
    out=[]
    for d in base():
        if g.nb[d,k(8,0):930].sum()<60: continue
        pm=g.CF[d,929]-g.CF[d,s0]
        if abs(pm)<=th*g.ADR[d]: continue
        side=-int(np.sign(pm)); e=g.OF[d,930]
        r,x=simx(d,930,side,stop*g.ADR[d] if stop else None,xk,entry=e); out.append((d,side,930,x,e,r))
    return out
def R(tr,lbl,cost=None,show=True,mult=None):
    cost=(1.25 if MKT=='NQ' else 0.8) if cost is None else cost
    mult=(2.0 if MKT=='NQ' else 5.0) if mult is None else mult
    r=g.rep([t[5] for t in tr],[t[0] for t in tr],lbl,cost=cost,mult=mult,show=False)
    if show and r and r['IS'] and r['OOS']:
        f=lambda v: f"n={v['n']:4d} pf={v['pf']:.2f} mo={v['mo']:5.0f} exp={v['exp']:5.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:6.0f}"
        print(f"{lbl:38s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
    return r
def CMP(kind='nr7',orm=30,stopm='side',xk=1314,last=k(13,0),stopf=1.0,dirf=None):
    r=pd.Series(g.RNG); pr=g.prevref(g.RNG)
    rs=r.shift(1)
    PH,PL=g.PH,g.PL; PPH=g.prevref(g.RH); PPL=g.prevref(g.RL)
    nr7=(rs==rs.rolling(7,min_periods=7).min()).values&np.isfinite(pr)
    nr4=(rs==rs.rolling(4,min_periods=4).min()).values&np.isfinite(pr)
    # prevref chain for inside day needs same contract two days back; approximate with PH/PL vs raw shift
    ins=(g.PH<np.r_[np.nan,np.r_[np.nan,g.RH[:-1]][:-1]])&(g.PL>np.r_[np.nan,np.r_[np.nan,g.RL[:-1]][:-1]])&(g.SYM==np.r_[g.SYM[:2],g.SYM[:-2]])
    low=pr<0.65*g.ADR
    m={'nr7':nr7,'nr4':nr4,'inside':ins,'low':low,'nr7|inside':nr7|ins,'nr4|inside':nr4|ins,'all':np.ones(g.N,bool)}[kind]
    out=[]
    for d in base():
        if not m[d]: continue
        oh=g.HF[d,930:930+orm].max(); ol=g.LF[d,930:930+orm].min(); w=oh-ol
        h=g.HF[d,930+orm:last]; l=g.LF[d,930+orm:last]
        a=np.nonzero(h>=oh+0.25)[0]; b=np.nonzero(l<=ol-0.25)[0]
        ia=a[0] if len(a) else 9999; ib=b[0] if len(b) else 9999
        if ia==ib: continue
        st=(w+0.25)*stopf if stopm=='side' else float(stopm)*g.ADR[d]
        if ia<ib: p,x=simx(d,930+orm+ia,1,st,xk,entry=oh+0.25); out.append((d,1,930+orm+ia,x,oh+0.25,p))
        else: p,x=simx(d,930+orm+ib,-1,st,xk,entry=ol-0.25); out.append((d,-1,930+orm+ib,x,ol-0.25,p))
    return out
