from lib import *
def sim_bracket(i,t0,d,ep,stop,tgt,eod=389,be_at=None,trail=None):
    """enter at ep at bar t0 (already filled at bar t0); walk from t0 (same bar after fill is unknown -> start next bar unless stop entry: check same bar conservative stop only)"""
    h,l,o=H[i],L[i],O[i]
    for t in range(t0+1,eod):
        if d>0:
            if l[t]<=stop: return t,min(stop,o[t])
            if tgt is not None and h[t]>=tgt: return t,max(tgt,o[t]) if o[t]>tgt else tgt
        else:
            if h[t]>=stop: return t,max(stop,o[t])
            if tgt is not None and l[t]<=tgt: return t,tgt
        if trail is not None:
            if d>0: stop=max(stop,h[t]-trail)
            else: stop=min(stop,l[t]+trail)
    return eod,o[eod]
def orb(n=15, stopmode='opp', stopfrac=1.0, tgtR=None, last=180, minr=0.0, maxr=9, onlydir=0, eod=389, buf=0.25, one=True, slip=0.0, gapfilter=None, trendf=None):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(ADR[i]): continue
        o,c,h,l=O[i],C[i],H[i],L[i]
        rh=h[:n].max(); rl=l[:n].min(); r=rh-rl
        if r<minr*ADR[i] or r>maxr*ADR[i]: continue
        done=False
        for t in range(n,last):
            up=h[t]>=rh+buf; dn=l[t]<=rl-buf
            if up and dn: break   # ambiguous bar: skip the day
            if not(up or dn): continue
            d=1 if up else -1
            if onlydir and d!=onlydir: break
            if trendf is not None and d!=trendf[i]: break
            ep=(max(rh+buf,o[t]) if d>0 else min(rl-buf,o[t]))+d*slip
            risk=r*stopfrac if stopmode=='opp' else ADR[i]*stopfrac
            stop=ep-d*risk
            tgt=None if tgtR is None else ep+d*risk*tgtR
            # same-bar stop check (conservative): if bar's opposite extreme breaches stop after entry
            if (d>0 and c[t]<=stop) or (d<0 and c[t]>=stop): xt,xp=t,stop
            else: xt,xp=sim_bracket(i,t,d,ep,stop,tgt,eod)
            tr.append((i,t,xt,d,ep,xp,risk)); break
    df=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp','risk']); df['pts']=df.dir*(df.xp-df.ep); return df
if __name__=='__main__':
    for n in (5,15,30,60):
        for sm,sf in (('opp',1.0),('opp',0.5),('adr',0.10),('adr',0.2)):
            for tg in (None,2.0):
                d=orb(n=n,stopmode=sm,stopfrac=sf,tgtR=tg,last=max(n+150,180)); stats(d,label=f'orb{n} {sm}{sf} tg{tg}')
