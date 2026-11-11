import warnings, os, pickle; warnings.filterwarnings('ignore')
from ref import *
def mk(tr): return pd.DataFrame(tr,columns=['day','em','xm','dir','ep','xp']).assign(pts=lambda d:d.dir*(d.xp-d.ep))
BASE=pickle.load(open('trades.pkl','rb'))
def dly(d): return ((d.pts-COST)*2).groupby(d.day).sum().reindex(range(N),fill_value=0)
BD={k:dly(v) for k,v in BASE.items()}; BP=sum(BD.values())
def S(d,l):
    r=stats(d,show=False)
    if r is None or 'IS' not in r or 'OOS' not in r: print(f"{l:40s} too few"); return
    a,i,o=r['ALL'],r['IS'],r['OOS']; x=dly(d)
    print(f"{l:40s} n={a['n']:5d} PF IS {i['pf']:.2f} OOS {o['pf']:.2f} | $/mo IS {i['mo']:4.0f} OOS {o['mo']:4.0f} | wr {a['wr']:.2f} dd {a['dd']:6.0f} sh {a['sh']:.2f} | corr port {np.corrcoef(x,BP)[0,1]:.2f}")
ok=np.array([valid(i) for i in range(N)])
def bands(i,vm=1.0): return max(DO[i],PC[i])*(1+vm*SIG5[i]), min(DO[i],PC[i])*(1-vm*SIG5[i])
print('--- E band retest: after a 30m close beyond the band, a later block trades back to the band and closes beyond it')
def retest(vm=1.0,sf=0.15,cb=3,last_b=65,flat=385,tol=0.0):
    tr=[]
    for i in np.where(ok)[0]:
        UB,LB=bands(i,vm); td=0
        for b in range(5,last_b+1):
            c=C5[i,b]
            if (b+1)%6==0:
                if c>UB[b]: 
                    if td!=1: td=1; t0=b
                elif c<LB[b]:
                    if td!=-1: td=-1; t0=b
            if td==0 or (b+1)%cb or b<=t0: continue
            lo=L5[i,b-cb+1:b+1].min(); hi=H5[i,b-cb+1:b+1].max()
            d=1 if (td>0 and lo<=UB[b]*(1+tol) and c>UB[b]) else -1 if (td<0 and hi>=LB[b]*(1-tol) and c<LB[b]) else 0
            if not d: continue
            m=b*5+5; ep=O[i,m]; xm,xp=walk(i,m,d,ep-d*sf*ADR5[i],flat); tr.append((i,m,xm,d,ep,xp)); break
    return mk(tr)
for sf in (0.1,0.15,0.25):
    for cb in (1,3): S(retest(sf=sf,cb=cb),f'retest stop {sf} block {cb*5}m')
S(retest(vm=0.8),'retest band 0.8'); S(retest(vm=1.3),'retest band 1.3')
print('--- A lunch range (11:30-13:30) breakout, with / without morning-trend filter')
def lunch(a=120,bq=240,sf=0.15,last=360,flat=385,filt='none'):
    tr=[]
    for i in np.where(ok)[0]:
        rh=H[i,a:bq].max(); rl=L[i,a:bq].min()
        UB,LB=bands(i)
        for m in range(bq,last):
            up=H[i,m]>=rh+TICK; dn=L[i,m]<=rl-TICK
            if up and dn: break
            if not(up or dn): continue
            d=1 if up else -1
            if filt=='open' and np.sign(C[i,bq-1]-DO[i])!=d: break
            if filt=='vwap' and np.sign(C[i,m-1]-VW[i,m-1])!=d: break
            if filt=='band' and not((d>0 and C[i,bq-1]>UB[bq//5-1]) or (d<0 and C[i,bq-1]<LB[bq//5-1])): break
            ep=max(rh+TICK,O[i,m]) if d>0 else min(rl-TICK,O[i,m]); stop=ep-d*sf*ADR5[i]
            if (d>0 and L[i,m]<=stop) or (d<0 and H[i,m]>=stop): tr.append((i,m,m,d,ep,stop)); break
            xm,xp=walk(i,m+1,d,stop,flat); tr.append((i,m,xm,d,ep,xp)); break
    return mk(tr)
for f in ('none','open','vwap','band'):
    for sf in (0.1,0.2): S(lunch(sf=sf,filt=f),f'lunch break filt {f} stop {sf}')
S(lunch(a=150,bq=270,filt='open',sf=0.2),'lunch 12:00-14:00 open stop .2')
print('--- S pyramid: NBM open and in profit at a later mark -> add one contract (own stop), exits with NBM')
def pyr(prof=0.25,sf=0.25,maxadd=1):
    tr=[]; nb=BASE['NBM']
    for r in nb.itertuples():
        i=r.day; UB,LB=bands(i); n=0
        for b in MARK:
            m=b*5+5
            if m<=r.em or m>=r.xm or m>360: continue
            c=C5[i,b]
            if r.dir*(c-r.ep)>=prof*ADR5[i] and ((r.dir>0 and c>UB[b]) or (r.dir<0 and c<LB[b])) and n<maxadd:
                ep=O[i,m]; stop=ep-r.dir*sf*ADR5[i]; xm,xp=walk(i,m,r.dir,stop,int(r.xm)); 
                if xm>=r.xm: xp=r.xp if r.xm<385 else O[i,385]
                tr.append((i,m,xm,r.dir,ep,xp)); n+=1
    return mk(tr)
for pf_ in (0.15,0.25,0.4):
    for sf in (0.15,0.25): S(pyr(pf_,sf),f'pyramid profit {pf_} stop {sf}')
print('--- C NQ vs ES relative strength at 10:30 / 11:00 -> hold to close')
ES=pickle.load(open('days_ES.pkl','rb')); em={d['td']:d for d in ES}
def rs(t=60,k=0.5,sf=0.2,mode='lead'):
    tr=[]
    for i in np.where(ok)[0]:
        e=em.get(days[i]['td'])
        if e is None or e['n']<380: continue
        rn=C[i,t-1]/DO[i]-1; re=e['c'][t-1]/e['o'][0]-1; z=(rn-1.2*re)/SIG5[i,t//5-1]
        if abs(z)<k: continue
        d=int(np.sign(z)) if mode=='lead' else -int(np.sign(z))
        if mode=='agree' and np.sign(rn)!=np.sign(z): continue
        ep=O[i,t]; xm,xp=walk(i,t,d,ep-d*sf*ADR5[i],385); tr.append((i,t,xm,d,ep,xp))
    return mk(tr)
for t in (30,60,90):
    for k in (0.3,0.6): S(rs(t,k),f'RS lead t{t} k{k}')
S(rs(60,0.3,mode='agree'),'RS lead + same sign as NQ move')
