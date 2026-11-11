# exit variants on the three final rules: profit target (x avg range), breakeven after +x, trailing stop
import ref2 as r, numpy as np
r.prep()
def trade2(D,b,side,dist,tgt=None,be=None,trail=None,eodb=77):
    nb=[x for x in np.nonzero(D.bok)[0] if x>b]
    if not nb or nb[0]>=eodb: return None
    eb=nb[0]; d=D.i; mins=np.nonzero(r.has[d])[0]; mins=mins[mins>=eb*5]
    e=r.o1[d,mins[0]]; dist=r.rnd(dist); stop=e-side*dist; best=e
    for m in mins:
        if m>=eodb*5: return (D.i,side,eb,m,e,side*(r.o1[d,m]-e),'EOD')
        lo,hi=r.l1[d,m],r.h1[d,m]
        if (side>0 and lo<=stop) or (side<0 and hi>=stop): return (D.i,side,eb,m,e,side*(stop-e),'STOP')
        if tgt and ((side>0 and hi>=e+tgt) or (side<0 and lo<=e-tgt)): return (D.i,side,eb,m,e,tgt,'TGT')
        best=max(best,hi) if side>0 else min(best,lo)
        if be and side*(best-e)>=be: stop=max(stop,e) if side>0 else min(stop,e)
        if trail: stop=max(stop,best-trail) if side>0 else min(stop,best+trail)
    return (D.i,side,eb,mins[-1],e,side*(r.c1[d,mins[-1]]-e),'EOD')
base=r.trade
for nm,fn in (('DWR',r.DWR),('VAR',r.VAR),('CEB',r.CEB)):
    r.trade=base; r.line(r.stats(fn()),f'{nm} as built (stop, hold to 15:55)')
    for lbl,kw in (('target 0.5 x range',dict(tgt=0.5)),('target 1.0 x range',dict(tgt=1.0)),('breakeven after +0.25',dict(be=0.25)),('breakeven after +0.5',dict(be=0.5)),('trail 0.5 x range',dict(trail=0.5)),('trail 1.0 x range',dict(trail=1.0))):
        def t(D,b,side,dist,eodb=77,kw=kw):
            k2={k:v*D.adr for k,v in kw.items()}; return trade2(D,b,side,dist,eodb=eodb,**k2)
        r.trade=t; r.line(r.stats(fn()),f'{nm} {lbl}')
