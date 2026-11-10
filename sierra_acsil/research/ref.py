# Reference models: signals on 5-min RTH bars, fills walked on 1-min bars. Mirrors the ACSIL engines.
from lib import *
B=78
O5=O.reshape(N,B,5)[:,:,0]; C5=C.reshape(N,B,5)[:,:,4]; H5=H.reshape(N,B,5).max(2); L5=L.reshape(N,B,5).min(2); V5=V.reshape(N,B,5).sum(2)
TP5=(H5+L5+C5)/3; W5=np.maximum(V5,1); VW5=np.cumsum(TP5*W5,1)/np.cumsum(W5,1)
FULL=~SHORT
def roll_prior(x,n=14):
    """mean of the previous n FULL sessions (short sessions skipped)"""
    out=np.full(x.shape,np.nan); idx=np.where(FULL)[0]
    for j,i in enumerate(range(N)):
        prev=idx[idx<i][-n:]
        if len(prev)==n: out[i]=x[prev].mean(0)
    return out
ADR5=roll_prior(RNG)
MARK=np.arange(5,B,6)            # bar index whose close is a 30-min mark: 10:00 (b=5) ... 15:30 (b=71); 16:00 is b=77
SIG5=roll_prior(np.abs(C5/DO[:,None]-1))   # per bar; used at marks
TICK=0.25
def walk(i,m0,d,stop,m1,same_bar_from=None):
    """walk 1-min bars m0..m1-1; return (exit minute, price). stop=None -> none."""
    for m in range(m0,m1):
        if stop is not None:
            if d>0 and L[i,m]<=stop: return m,min(stop,O[i,m])
            if d<0 and H[i,m]>=stop: return m,max(stop,O[i,m])
    return m1,O[i,m1]
def valid(i): return FULL[i] and not np.isnan(ADR5[i]) and not np.isnan(PC[i]) and not np.isnan(SIG5[i,0])
def nbm(vm=1.0,cat=0.5,maxtr=3,last_entry_b=71,flat_m=385,use_vwap=True,dirs=(1,-1)):
    tr=[]
    for i in range(N):
        if not valid(i): continue
        ub0=max(DO[i],PC[i]); lb0=min(DO[i],PC[i]); pos=0; n=0; stopped=False
        for b in MARK:
            if b*5+5>flat_m: break
            c=C5[i,b]; UBb=ub0*(1+vm*SIG5[i,b]); LBb=lb0*(1-vm*SIG5[i,b]); vw=VW5[i,b]; m=b*5+5
            if pos!=0:
                # catastrophic stop between marks
                xm,xp=walk(i,lastm,pos,stop,m)
                if xm<m: tr.append((i,em,xm,pos,ep,xp,'stop')); pos=0
                else:
                    trail=(max(UBb,vw) if use_vwap else UBb) if pos>0 else (min(LBb,vw) if use_vwap else LBb)
                    if (pos>0 and c<trail) or (pos<0 and c>trail): tr.append((i,em,m,pos,ep,O[i,m],'trail')); pos=0; continue
                    lastm=m
            if pos==0 and b<=last_entry_b and n<maxtr:
                d=1 if (c>UBb and (c>vw or not use_vwap)) else -1 if (c<LBb and (c<vw or not use_vwap)) else 0
                if d and d in dirs:
                    pos=d; ep=O[i,m]; em=m; lastm=m; n+=1; stop=ep-d*cat*ADR5[i] if cat else None
        if pos!=0:
            xm,xp=walk(i,lastm,pos,stop,flat_m); tr.append((i,em,xm,pos,ep,xp,'stop' if xm<flat_m else 'eod'))
    return pd.DataFrame(tr,columns=['day','em','xm','dir','ep','xp','why']).assign(pts=lambda d:d.dir*(d.xp-d.ep))
def god(sf=0.10,ming=0.05,maxg=9,valid_min=60,flat_m=385,orb=1,conservative=True,be=None):
    tr=[]
    for i in range(N):
        if not valid(i): continue
        g=(DO[i]-PC[i])/ADR5[i]
        if abs(g)<ming or abs(g)>maxg: continue
        d=1 if g>0 else -1
        orh=H5[i,:orb].max(); orl=L5[i,:orb].min(); m0=orb*5
        trig=orh+TICK if d>0 else orl-TICK; opp=orl-TICK if d>0 else orh+TICK
        for m in range(m0,valid_min):
            hit=(H[i,m]>=trig) if d>0 else (L[i,m]<=trig)
            oppo=(L[i,m]<=opp) if d>0 else (H[i,m]>=opp)
            if oppo and not hit: break            # opposite side broke first: no trade today
            if not hit: continue
            if oppo and conservative: break       # both in one minute: treat as cancelled
            ep=max(trig,O[i,m]) if d>0 else min(trig,O[i,m]); stop=ep-d*sf*ADR5[i]
            if conservative and ((d>0 and L[i,m]<=stop) or (d<0 and H[i,m]>=stop)): tr.append((i,m,m,d,ep,stop,'stop')); break
            xm,xp=walk(i,m+1,d,stop,flat_m); tr.append((i,m,xm,d,ep,xp,'stop' if xm<flat_m else 'eod')); break
    return pd.DataFrame(tr,columns=['day','em','xm','dir','ep','xp','why']).assign(pts=lambda d:d.dir*(d.xp-d.ep))
def tvp(vm=1.0,sf=0.15,cbars=3,last_b=65,flat_m=385,maxn=1):
    tr=[]
    for i in range(N):
        if not valid(i): continue
        ub0=max(DO[i],PC[i]); lb0=min(DO[i],PC[i]); td=0; n=0; busy_until=0
        for b in range(5,last_b+1):
            c=C5[i,b]; m=b*5+5
            if (b+1)%6==0:
                if c>ub0*(1+vm*SIG5[i,b]): td=1
                elif c<lb0*(1-vm*SIG5[i,b]): td=-1
            if td==0 or (b+1)%cbars or n>=maxn or m<busy_until: continue
            lo=L5[i,b-cbars+1:b+1].min(); hi=H5[i,b-cbars+1:b+1].max(); vw=VW5[i,b]
            d=1 if (td>0 and lo<=vw and c>vw) else -1 if (td<0 and hi>=vw and c<vw) else 0
            if not d: continue
            ep=O[i,m]; stop=ep-d*sf*ADR5[i]; xm,xp=walk(i,m,d,stop,flat_m)
            tr.append((i,m,xm,d,ep,xp,'stop' if xm<flat_m else 'eod')); n+=1; busy_until=xm+1
    return pd.DataFrame(tr,columns=['day','em','xm','dir','ep','xp','why']).assign(pts=lambda d:d.dir*(d.xp-d.ep))
if __name__=='__main__':
    for cat in (None,0.3,0.5,0.75): stats(nbm(cat=cat),label=f'NBM cat{cat}')
    for mt in (1,2,3,9): stats(nbm(maxtr=mt),label=f'NBM maxtr{mt}')
    for vm in (0.8,1.2): stats(nbm(vm=vm),label=f'NBM vm{vm}')
    print(nbm().why.value_counts().to_dict())
    stats(god(),label='GOD cons'); stats(god(conservative=False),label='GOD optimistic')
    for sf in (0.075,0.15,0.2): stats(god(sf=sf),label=f'GOD sf{sf}')
    for o in (2,3): stats(god(orb=o),label=f'GOD or{o*5}m')
    stats(god(ming=0),label='GOD ming0'); stats(god(ming=0.1),label='GOD ming.1')
    stats(tvp(),label='TVP'); 
    for sf in (0.1,0.2): stats(tvp(sf=sf),label=f'TVP sf{sf}')
    stats(tvp(vm=0.8),label='TVP vm.8'); stats(tvp(vm=1.3),label='TVP vm1.3'); stats(tvp(maxn=2),label='TVP max2')
