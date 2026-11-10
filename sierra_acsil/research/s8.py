from lib import *
from s1 import noise
from s2 import sim_bracket
from s6 import god
import pickle
def mk(tr):
    df=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp']); df['pts']=df.dir*(df.xp-df.ep); return df
def aft_break(tm=150,sf=0.1,last=360,eod=389,buf=0.25,align=None):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(ADR[i]): continue
        mh=H[i,:tm].max(); ml=L[i,:tm].min()
        for t in range(tm,last):
            up=H[i,t]>=mh+buf; dn=L[i,t]<=ml-buf
            if up and dn: break
            if not(up or dn): continue
            d=1 if up else -1
            if align=='open' and np.sign(C[i,tm-1]-DO[i])!=d: break
            if align=='vwap' and np.sign(C[i,t-1]-VW[i,t-1])!=d: break
            ep=max(mh+buf,O[i,t]) if d>0 else min(ml-buf,O[i,t]); stop=ep-d*sf*ADR[i]
            if (d>0 and L[i,t]<=stop) or (d<0 and H[i,t]>=stop): xt,xp=t,stop
            else: xt,xp=sim_bracket(i,t,d,ep,stop,None,eod)
            tr.append((i,t,xt,d,ep,xp)); break
    return mk(tr)
def reversal(vm=1.0,t1=300,sf=0.15,eod=389,need=30):
    """failed breakdown: price closed (30m mark) beyond a noise band, later a 30m close back across VWAP and the open -> trade the reversal to close"""
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(SIG[i,0]) or np.isnan(PC[i]) or np.isnan(ADR[i]): continue
        UB=max(DO[i],PC[i])*(1+vm*SIG[i]); LB=min(DO[i],PC[i])*(1-vm*SIG[i]); was=0
        for t in range(29,t1,30):
            c=C[i,t]
            if was==0:
                if c>UB[t]: was=1
                elif c<LB[t]: was=-1
            else:
                if was<0 and c>VW[i,t] and c>DO[i]: d=1
                elif was>0 and c<VW[i,t] and c<DO[i]: d=-1
                else: continue
                ep=O[i,t+1]; stop=ep-d*sf*ADR[i]; xt,xp=sim_bracket(i,t+1,d,ep,stop,None,eod); tr.append((i,t+1,xt,d,ep,xp)); break
    return mk(tr)
def pullback(vm=1.0,t1=330,sf=0.1,eod=389):
    """trend established (30m close beyond band); later a 5m bar touches VWAP and closes back on the trend side -> enter with trend"""
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(SIG[i,0]) or np.isnan(PC[i]) or np.isnan(ADR[i]): continue
        UB=max(DO[i],PC[i])*(1+vm*SIG[i]); LB=min(DO[i],PC[i])*(1-vm*SIG[i]); tr_d=0
        for t in range(29,t1,5):
            c=C[i,t]
            if (t+1)%30==0:
                if c>UB[t]: tr_d=1
                elif c<LB[t]: tr_d=-1
            if tr_d==0: continue
            lo=L[i,t-4:t+1].min(); hi=H[i,t-4:t+1].max(); vw=VW[i,t]
            if tr_d>0 and lo<=vw and c>vw: d=1
            elif tr_d<0 and hi>=vw and c<vw: d=-1
            else: continue
            ep=O[i,t+1]; stop=ep-d*sf*ADR[i]; xt,xp=sim_bracket(i,t+1,d,ep,stop,None,eod); tr.append((i,t+1,xt,d,ep,xp)); break
    return mk(tr)
def dayfollow(sf=0.15,thr=0.7,minr=1.0):
    """after a trend day (close in top/bottom 30% of range, range>=minr*ADR) trade next day in same dir on 5m OR break"""
    from s2 import orb
    pos=(C[:,389]-L.min(1))/np.maximum(RNG,1e-9); sig=np.where((pos>=thr)&(RNG>=minr*ADR),1,np.where((pos<=1-thr)&(RNG>=minr*ADR),-1,0))
    prev=np.r_[0,sig[:-1]]
    d=orb(n=5,stopmode='adr',stopfrac=sf); return d[prev[d.day.values]==d.dir]
if __name__=='__main__':
    nb=noise(); g=god(5,0.1)
    def daily(d): return d.groupby('day').pts.sum().reindex(range(N),fill_value=0)
    X=daily(nb); G=daily(g)
    def rep(d,l):
        stats(d,label=l); y=daily(d); print(f'      corr NBM {np.corrcoef(X,y)[0,1]:.2f} GOD {np.corrcoef(G,y)[0,1]:.2f}')
    for tm in (120,150,210):
        for al in (None,'open'): rep(aft_break(tm,align=al),f'aftbreak tm{tm} {al}')
    rep(aft_break(150,sf=0.2),'aftbreak 150 stop.2')
    for sf in (0.1,0.2): rep(reversal(sf=sf),f'reversal stop{sf}')
    for sf in (0.1,0.2): rep(pullback(sf=sf),f'pullback stop{sf}')
    rep(dayfollow(),'dayfollow'); rep(dayfollow(thr=0.5,minr=0),'dayfollow loose')
