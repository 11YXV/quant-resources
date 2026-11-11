from L import *
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)
D=np.nonzero(ok)[0]
R=slice(930,1320)
c=CF[:,R]; h=HF[:,R]; l=LF[:,R]; o=OF[:,R]
def P(lbl,pts,dy,**kw):
    r=rep(pts,dy,lbl,show=False,**kw)
    if r is None or r['IS'] is None or r['OOS'] is None: return
    f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
    flag='**' if r['IS']['pf']>1.15 and r['OOS']['pf']>1.15 else '  '
    print(f"{flag}{lbl:40s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
print('== F short-horizon reversal: fade m-min move > z*sd, hold hmin (market at close of bar) ==')
ret1=np.diff(np.log(c),axis=1)
for m in (1,3,5,15):
    rm=np.log(c[:,m:]/c[:,:-m])              # return ending at col j+m
    # per-day sd of m-min returns from prior 14 days
    sd=pd.Series(np.nanstd(rm,axis=1)).rolling(14).mean().shift(1).values
    for z in (2.5,3.5):
        for hold in (5,15,30):
            pts=[];dy=[]
            for d in D:
                if not np.isfinite(sd[d]): continue
                x=rm[d]/sd[d]; j=0
                idx=np.nonzero(np.abs(x)>z)[0]
                last=-1
                for i in idx:
                    t=i+m       # column of signal close
                    if t<=last or t<15 or t+hold>384: continue
                    s=-np.sign(x[i]); pts.append(s*(c[d,t+hold]-c[d,t])); dy.append(d); last=t+hold
            P(f'fade m{m} z{z} hold{hold}',pts,dy)
print('== L calendar ==')
dom=pd.Series(TD); 
mo=TD.to_period('M'); first=np.r_[True,mo[1:]!=mo[:-1]]; last=np.r_[mo[1:]!=mo[:-1],False]
tom=np.zeros(N,bool)
for i in np.nonzero(first)[0]:
    tom[max(0,i-1):i+3]=True
for nm,mm in (('turn of month (-1..+3)',tom),('not TOM',~tom),('all days',np.ones(N,bool)),('Mon',TD.weekday==0),('Fri',TD.weekday==4)):
    m_=D[mm[D]]; P(nm+' long o->c',CF[m_,1314]-RO[m_],m_)
print('== O small gap fill (fade gap toward prior close, target=pc, stop=k*gap) ==')
gap=(RO-PC)
for lo,hi in ((0.05,0.15),(0.15,0.3),(0.3,0.6),(0.6,9)):
    for sm in (1.0,2.0):
        for xk in (k(10,30),k(12,0),1314):
            pts=[];dy=[]
            for d in D:
                g=gap[d]/ADR[d]
                if not(lo<=abs(g)<hi): continue
                s=-np.sign(g); p,_=sim(d,930,s,stop=sm*abs(gap[d]),tgt=abs(gap[d]),xk=xk,entry=RO[d])
                pts.append(p);dy.append(d)
            P(f'gapfill {lo}-{hi}adr stop{sm}g x{xk}',pts,dy)
