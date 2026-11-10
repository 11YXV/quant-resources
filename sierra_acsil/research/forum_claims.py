import warnings; warnings.filterwarnings('ignore')
from ref import *
def mk(tr): return pd.DataFrame(tr,columns=['day','em','xm','dir','ep','xp']).assign(pts=lambda d:d.dir*(d.xp-d.ep))
def S(d,l):
    r=stats(d,show=False)
    if r is None or 'IS' not in r or 'OOS' not in r: print(f"{l:44s} too few trades"); return
    a,i,o=r['ALL'],r['IS'],r['OOS']
    print(f"{l:44s} n={a['n']:5d} PF IS {i['pf']:.2f} OOS {o['pf']:.2f} | $/mo IS {i['mo']:5.0f} OOS {o['mo']:5.0f} | wr {a['wr']:.2f} dd {a['dd']:7.0f} sh {a['sh']:.2f}")
ok=np.array([valid(i) for i in range(N)])
gap=(DO-PC)/ADR5
dc=pd.Series(C[:,389]); pret=(dc/dc.shift(1)-1).shift(1).values; ma50=dc.rolling(50).mean().shift(1).values; up=dc.shift(1).values>ma50
ppos=np.r_[np.nan,((C[:,389]-L.min(1))/np.maximum(RNG,1e-9))[:-1]]
print('--- 1 gap rebound: enter 09:30 open, exit after hold minutes (no stop / stop)')
def gaptr(cond,d=1,hold=60,sf=None):
    tr=[]
    for i in np.where(ok&cond)[0]:
        ep=O[i,0]; stop=None if sf is None else ep-d*sf*ADR5[i]; xm,xp=walk(i,0,d,stop,hold); tr.append((i,0,xm,d,ep,xp))
    return mk(tr)
for g in (0.1,0.25,0.5):
    S(gaptr(gap<=-g),f'gap down >= {g} ADR, long 60m')
    S(gaptr((gap<=-g)&(pret<0)),f'  + prior day down')
    S(gaptr((gap<=-g)&(ppos<0.25)),f'  + prior close in low quarter')
    S(gaptr((gap<=-g)&up),f'  + above 50-day average')
    S(gaptr(gap>=g,-1),f'gap up >= {g} ADR, short 60m')
S(gaptr((gap<=-0.25)&(pret<0),hold=30),'gap dn .25 + prior down, 30m'); S(gaptr((gap<=-0.25)&(pret<0),hold=120),'gap dn .25 + prior down, 120m'); S(gaptr((gap<=-0.25)&(pret<0),hold=385),'gap dn .25 + prior down, to close')
print('--- 2 ORB 15m, 5-min close confirmation, stop at midpoint, target R, min range')
def orb15(n=3,tgtR=2.0,minr=0.0,last_b=30,stopmode='mid',eod=385):
    tr=[]
    for i in np.where(ok)[0]:
        rh=H5[i,:n].max(); rl=L5[i,:n].min(); r=rh-rl
        if r<minr*ADR5[i]: continue
        for b in range(n,last_b):
            c=C5[i,b]; d=1 if c>rh else -1 if c<rl else 0
            if not d: continue
            m=b*5+5; ep=O[i,m]; stop=(rh+rl)/2 if stopmode=='mid' else (rl if d>0 else rh)
            risk=d*(ep-stop)
            if risk<=0: break
            tgt=None if tgtR is None else ep+d*tgtR*risk; xm=eod; xp=O[i,eod]
            for mm in range(m,eod):
                if (d>0 and L[i,mm]<=stop) or (d<0 and H[i,mm]>=stop): xm,xp=mm,stop; break
                if tgt is not None and ((d>0 and H[i,mm]>=tgt) or (d<0 and L[i,mm]<=tgt)): xm,xp=mm,tgt; break
            tr.append((i,m,xm,d,ep,xp)); break
    return mk(tr)
for tg in (1.0,2.0,3.0,None):
    for mr in (0.0,0.15,0.25): S(orb15(tgtR=tg,minr=mr),f'ORB15 mid stop tgt {tg}R minrange {mr}')
S(orb15(tgtR=2.0,stopmode='opp'),'ORB15 opposite-side stop tgt 2R'); S(orb15(n=6,tgtR=2.0),'ORB30 mid stop tgt 2R')
print('--- 3 long only, first two hours')
def am(cond,x=120,d=1): return mk([(i,0,x,d,O[i,0],O[i,x]) for i in np.where(ok&cond)[0]])
S(am(np.ones(N,bool)),'long 09:30-11:30 every day'); S(am(up),'  above 50-day average'); S(am(pret>0),'  prior day up'); S(am(gap>0),'  gap up'); S(am(up&(gap>0)),'  above avg + gap up'); S(am(~up,d=-1),'short 09:30-11:30 below 50-day avg')
print('--- 4 ATR trend band (supertrend) on 10-min day-session bars')
def st(n=10,mult=3.0,bar=10,eod=385,first=6):
    k=bar//5; tr=[]
    for i in np.where(ok)[0]:
        nb=B//k; h=H5[i,:nb*k].reshape(nb,k).max(1); l=L5[i,:nb*k].reshape(nb,k).min(1); c=C5[i,:nb*k].reshape(nb,k)[:,-1]
        trn=np.maximum(h-l,np.r_[0,np.maximum(abs(h[1:]-c[:-1]),abs(l[1:]-c[:-1]))]); atr=pd.Series(trn).rolling(n,min_periods=3).mean().values
        mid=(h+l)/2; dirn=0; band=0; pos=0
        for b in range(2,nb):
            ub=mid[b]+mult*atr[b]; lb=mid[b]-mult*atr[b]
            if dirn>=0: band=max(band,lb) if dirn>0 else lb
            if dirn<0: band=min(band,ub)
            nd=dirn
            if dirn>=0 and c[b]<band and dirn>0: nd=-1; band=ub
            elif dirn<0 and c[b]>band: nd=1; band=lb
            elif dirn==0: nd=1 if c[b]>mid[b-1] else -1; band=lb if nd>0 else ub
            m=(b+1)*bar
            if m>=eod: break
            if nd!=pos and b>=first:
                if pos!=0: tr.append((i,em,m,pos,ep,O[i,m]))
                pos=nd; ep=O[i,m]; em=m
            dirn=nd
        if pos!=0: tr.append((i,em,eod,pos,ep,O[i,eod]))
    return mk(tr)
for mult in (2.0,3.0,4.0):
    for n in (10,20): S(st(n,mult),f'supertrend ATR{n} x{mult} 10m')
S(st(10,3.0,bar=15),'supertrend ATR10 x3 15m'); S(st(10,3.0,bar=30),'supertrend ATR10 x3 30m')
print('--- 5 VWAP reversion only in 09:45-11:30 and 13:00-15:00')
def vrev(k=2.0,stopk=1.0,wins=((15,120),(210,330)),hardx=None):
    tr=[]
    for i in np.where(ok)[0]:
        dev=C[i]-VW[i]; sd=np.sqrt(np.cumsum((TP[i]-VW[i])**2)/np.arange(1,391))
        for a,bn in wins:
            t=a
            while t<bn:
                if sd[t]>0 and abs(dev[t])>k*sd[t]:
                    d=-int(np.sign(dev[t])); ep=O[i,t+1]; stop=ep-d*stopk*sd[t]; end=bn if hardx is None else min(385,t+1+hardx); xm,xp=end,O[i,end]
                    for u in range(t+1,end):
                        if (d>0 and L[i,u]<=stop) or (d<0 and H[i,u]>=stop): xm,xp=u,stop; break
                        if (d>0 and H[i,u]>=VW[i,u]) or (d<0 and L[i,u]<=VW[i,u]): xm,xp=u,VW[i,u]; break
                    tr.append((i,t+1,xm,d,ep,xp)); break
                t+=1
    return mk(tr)
for k in (1.5,2.0,2.5,3.0):
    for sk in (1.0,2.0): S(vrev(k,sk),f'VWAP fade {k} sd, stop {sk} sd, both windows')
S(vrev(2.0,1.0,wins=((15,120),)),'VWAP fade 2 sd morning only'); S(vrev(2.0,1.0,wins=((210,330),)),'VWAP fade 2 sd afternoon only')
