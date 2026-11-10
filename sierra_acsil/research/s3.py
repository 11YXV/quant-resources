from lib import *
from s2 import sim_bracket
def mk(tr): 
    df=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp']); df['pts']=df.dir*(df.xp-df.ep); return df
def gapfade(ming=0.15,maxg=0.6,stopf=0.25,eodx=120,follow=False):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(ADR[i]) or np.isnan(PC[i]): continue
        g=(DO[i]-PC[i])/ADR[i]
        if not(ming<=abs(g)<=maxg): continue
        d=-np.sign(g)*( -1 if follow else 1); ep=O[i,1]
        stop=ep-d*stopf*ADR[i]; tgt=None if follow else PC[i]
        xt,xp=sim_bracket(i,1,d,ep,stop,tgt,eodx); tr.append((i,1,xt,d,ep,xp))
    return mk(tr)
def lasthalf(mode='day',thr=0.0,t0=360,eod=389):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(PC[i]): continue
        if mode=='day': r=C[i,t0-1]/PC[i]-1
        elif mode=='first': r=C[i,29]/PC[i]-1
        elif mode=='intra': r=C[i,t0-1]/DO[i]-1
        if abs(r)<thr*SIG[i,389] if not np.isnan(SIG[i,389]) else True: continue
        d=np.sign(r); 
        if d==0: continue
        tr.append((i,t0,eod,d,O[i,t0],O[i,eod]))
    return mk(tr)
def vwapfade(k=2.0,t0=90,t1=330,stopk=1.0,eod=389,maxnoise=1.0,tgt_vwap=True,maxtr=2):
    """fade deviation from VWAP of k*running std (of tp-vwap) when price is inside the noise band (non-trend day)"""
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(SIG[i,0]) or np.isnan(PC[i]): continue
        dev=C[i]-VW[i]
        sd=np.sqrt(np.cumsum((TP[i]-VW[i])**2)/np.arange(1,391))
        UB=max(DO[i],PC[i])*(1+maxnoise*SIG[i]); LB=min(DO[i],PC[i])*(1-maxnoise*SIG[i])
        t=t0; n=0
        while t<t1 and n<maxtr:
            if sd[t]>0 and abs(dev[t])>k*sd[t] and LB[t]<C[i,t]<UB[t]:
                d=-np.sign(dev[t]); ep=O[i,t+1]; stop=ep-d*stopk*sd[t]
                # exit at vwap touch (dynamic): walk
                xt=None
                for u in range(t+1,eod):
                    if (d>0 and L[i,u]<=stop) or (d<0 and H[i,u]>=stop): xt,xp=u,stop; break
                    if tgt_vwap and ((d>0 and H[i,u]>=VW[i,u]) or (d<0 and L[i,u]<=VW[i,u])): xt,xp=u,VW[i,u]; break
                if xt is None: xt,xp=eod,O[i,eod]
                tr.append((i,t+1,xt,d,ep,xp)); n+=1; t=xt+1
            else: t+=1
    return mk(tr)
def sweep(level='pd',t1=240,stopbuf=0.02,tgtR=2.0,minpen=0.0,eod=389,confirm=5):
    """failed breakout: price trades beyond prior-day (or overnight) high/low, then a 5-min close back inside -> fade; stop beyond the extreme"""
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(ADR[i]) or np.isnan(PH[i]): continue
        hi,lo=(PH[i],PL[i]) if level=='pd' else (ONH[i],ONL[i])
        if np.isnan(hi): continue
        done=False
        for t in range(confirm,t1,confirm):
            seg_h=H[i,:t].max(); seg_l=L[i,:t].min(); c=C[i,t-1]
            for d,cond,ext in ((-1, seg_h>hi+minpen*ADR[i] and c<hi and H[i,t-confirm:t].max()>hi, seg_h),(1, seg_l<lo-minpen*ADR[i] and c>lo and L[i,t-confirm:t].min()<lo, seg_l)):
                if cond:
                    ep=O[i,t]; stop=ext-d*stopbuf*ADR[i]; risk=abs(ep-stop)
                    if risk>0.3*ADR[i] or risk<=0: continue
                    xt,xp=sim_bracket(i,t,d,ep,stop,ep+d*risk*tgtR if tgtR else None,eod)
                    tr.append((i,t,xt,d,ep,xp)); done=True; break
            if done: break
    return mk(tr)
def midday(q=0.25,eod=389,bar=30):
    """Kaufman TSM Midday S/R: after 12:00, close in lower 25% of morning range -> long; upper -> short; break of morning range -> reverse"""
    tr=[]
    for i in range(20,N):
        if SHORT[i]: continue
        mh=H[i,:150].max(); ml=L[i,:150].min(); r=mh-ml; pos=0
        for t in range(150+bar-1,eod,bar):
            c=C[i,t]; want=pos
            if c<ml: want=-1
            elif c>mh: want=1
            elif c<=ml+q*r and pos<=0: want=1
            elif c>=mh-q*r and pos>=0: want=-1
            if want!=pos:
                if pos!=0: tr.append((i,et,t+1,pos,ep,O[i,t+1]))
                pos=want; ep=O[i,t+1]; et=t+1
        if pos!=0: tr.append((i,et,eod,pos,ep,O[i,eod]))
    return mk(tr)
if __name__=='__main__':
    for a in ((0.15,0.6,0.25),(0.3,1.0,0.3),(0.05,0.3,0.2)): stats(gapfade(*a),label=f'gapfade{a}')
    stats(gapfade(0.3,2,0.3,389,True),label='gapfollow')
    for m in ('day','first','intra'):
        for th in (0,1.0): stats(lasthalf(m,th),label=f'lasthalf {m} thr{th}')
    stats(lasthalf('day',1.0,330),label='last hour day thr1')
    for k in (1.5,2.0,2.5): 
        stats(vwapfade(k),label=f'vwapfade k{k}')
    stats(vwapfade(2.0,maxnoise=9),label='vwapfade k2 nofilter')
    stats(vwapfade(2.0,stopk=2.0),label='vwapfade k2 stop2')
    for lv in ('pd','on'):
        for tg in (1.0,2.0,None): stats(sweep(lv,tgtR=tg),label=f'sweep {lv} tg{tg}')
    stats(midday(),label='kaufman midday')
