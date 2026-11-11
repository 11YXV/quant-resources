import warnings, pickle; warnings.filterwarnings('ignore')
from ref import *
def mk(tr): return pd.DataFrame(tr,columns=['day','em','xm','dir','ep','xp']).assign(pts=lambda d:d.dir*(d.xp-d.ep))
BASE=pickle.load(open('trades.pkl','rb')); N3=pickle.load(open('new3.pkl','rb'))
def dly(d): return ((d.pts-COST)*2).groupby(d.day).sum().reindex(range(N),fill_value=0)
BP=sum(dly(v) for v in list(BASE.values())+list(N3.values()))
def S(d,l):
    r=stats(d,show=False)
    if r is None or 'IS' not in r or 'OOS' not in r: print(f"{l:42s} too few"); return
    a,i,o=r['ALL'],r['IS'],r['OOS']; x=dly(d)
    print(f"{l:42s} n={a['n']:5d} PF IS {i['pf']:.2f} OOS {o['pf']:.2f} | $/mo IS {i['mo']:4.0f} OOS {o['mo']:4.0f} | wr {a['wr']:.2f} dd {a['dd']:6.0f} sh {a['sh']:.2f} | corr six {np.corrcoef(x,BP)[0,1]:.2f}")
ok=np.array([valid(i) for i in range(N)])
def bands(i,vm=1.0): return max(DO[i],PC[i])*(1+vm*SIG5[i]), min(DO[i],PC[i])*(1-vm*SIG5[i])
volp=pd.Series(ADR5/DO); reg=(volp.rolling(250,min_periods=120).rank(pct=True)).values     # percentile of today's ADR% vs last 250 days
print('--- a) NBM by volatility regime (ADR%/price percentile over 250 days)')
nb=BASE['NBM'].copy(); nb['r']=reg[nb.day.values]
for lo,hi in ((0,0.33),(0.33,0.66),(0.66,1.01)): S(nb[(nb.r>=lo)&(nb.r<hi)],f'NBM regime pct {lo}-{hi}')
print('--- a2) fade band breaks in LOW vol regime: short above UB at a mark, target VWAP, stop x ADR')
def fade(rmax=0.33,sf=0.15,last_b=59,vm=1.0,tgt=True):
    tr=[]
    for i in np.where(ok)[0]:
        if not(reg[i]<rmax): continue
        UB,LB=bands(i,vm)
        for b in MARK:
            if b>last_b: break
            c=C5[i,b]; d=-1 if c>UB[b] else 1 if c<LB[b] else 0
            if not d: continue
            m=b*5+5; ep=O[i,m]; stop=ep-d*sf*ADR5[i]; xm,xp=385,O[i,385]
            for u in range(m,385):
                if (d>0 and L[i,u]<=stop) or (d<0 and H[i,u]>=stop): xm,xp=u,stop; break
                if tgt and ((d>0 and H[i,u]>=VW[i,u]) or (d<0 and L[i,u]<=VW[i,u])): xm,xp=u,VW[i,u]; break
            tr.append((i,m,xm,d,ep,xp)); break
    return mk(tr)
for rm in (0.25,0.4):
    for sf in (0.1,0.2): S(fade(rm,sf),f'fade low-vol<{rm} stop {sf}')
S(fade(1.01,0.15),'fade all regimes (control)')
print('--- b) early breakout at 09:45 (z of first 15 min) -> hold to close')
def early(b0=2,k=1.0,sf=0.15,vw=True):
    tr=[]
    for i in np.where(ok)[0]:
        c=C5[i,b0]; z=(c/DO[i]-1)/SIG5[i,b0]
        if abs(z)<k: continue
        d=int(np.sign(z))
        if vw and np.sign(c-VW5[i,b0])!=d: continue
        m=b0*5+5; ep=O[i,m]; xm,xp=walk(i,m,d,ep-d*sf*ADR5[i],385); tr.append((i,m,xm,d,ep,xp))
    return mk(tr)
for b0 in (1,2,3):
    for k in (1.0,1.5): S(early(b0,k),f'early t={b0*5+5}m z>{k}')
S(early(2,1.0,0.1),'early 15m stop .1'); S(early(2,1.0,0.25),'early 15m stop .25')
print('--- e) opening volume surge: first 30m volume vs 14-day avg > x, follow first 30m direction')
v30=V[:,:30].sum(1); rv=v30/pd.Series(v30).rolling(14).mean().shift(1).values
def volsurge(x=1.3,sf=0.2,lo=None):
    tr=[]
    for i in np.where(ok)[0]:
        if not(rv[i]>=x) or (lo and rv[i]>=lo): continue
        d=int(np.sign(C[i,29]-DO[i]))
        if d==0: continue
        ep=O[i,30]; xm,xp=walk(i,30,d,ep-d*sf*ADR5[i],385); tr.append((i,30,xm,d,ep,xp))
    return mk(tr)
for x in (1.15,1.3,1.6): S(volsurge(x),f'vol surge >{x}')
S(volsurge(0.0,lo=0.85),'low opening volume <0.85 (control)')
print('--- i) trend day: resting LIMIT at VWAP (no confirmation), stop x ADR')
def vlimit(sf=0.15,last_b=65,vm=1.0,wait=1):
    tr=[]
    for i in np.where(ok)[0]:
        UB,LB=bands(i,vm); td=0; tset=0
        for b in range(5,last_b+1):
            c=C5[i,b]
            if (b+1)%6==0:
                nd=1 if c>UB[b] else -1 if c<LB[b] else td
                if nd!=td: td=nd; tset=b
            if td==0 or b-tset<wait-1: continue
            lim=VW5[i,b]; m0=b*5+5; done=False
            for m in range(m0,min(m0+5,385)):
                if (td>0 and L[i,m]<=lim) or (td<0 and H[i,m]>=lim):
                    ep=min(lim,O[i,m]) if td>0 else max(lim,O[i,m]); stop=ep-td*sf*ADR5[i]
                    if (td>0 and L[i,m]<=stop) or (td<0 and H[i,m]>=stop): tr.append((i,m,m,td,ep,stop))
                    else:
                        xm,xp=walk(i,m+1,td,stop,385); tr.append((i,m,xm,td,ep,xp))
                    done=True; break
            if done: break
    return mk(tr)
for sf in (0.1,0.15,0.25): S(vlimit(sf),f'VWAP limit stop {sf}')
print('--- c) Monday long open->close with stop; by year')
def wd(day=0,sf=0.3,d=1):
    return mk([(i,1)+walk(i,1,d,O[i,1]-d*sf*ADR5[i],385)[:1]+(d,O[i,1],walk(i,1,d,O[i,1]-d*sf*ADR5[i],385)[1]) for i in np.where(ok&(TD.weekday==day))[0]])
for dd in range(5): S(wd(dd),f'weekday {dd} long, stop .3')
x=wd(0); print('Monday by year $:',((x.pts-COST)*2).groupby(TD[x.day.values].year).sum().round(0).astype(int).to_dict())
