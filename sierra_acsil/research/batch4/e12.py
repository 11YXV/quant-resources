from M import *
import M
D=base()
oc=pd.Series(np.where(g.FULL,g.RC/g.RO-1,np.nan)); cc=pd.Series(np.where(np.isfinite(g.PC)&g.FULL,g.RC/g.PC-1,np.nan))
vol=oc.rolling(20,min_periods=15).std().shift(1).values
def run(n,z,src='oc',side='both',stop=None,xk=1314,lbl=None,show=True,ret=False):
    s_=(oc if src=='oc' else cc).rolling(n,min_periods=n).sum().shift(1).values
    zz=s_/(vol*np.sqrt(n)); out=[]
    for d in D:
        if not np.isfinite(zz[d]) or abs(zz[d])<z: continue
        sd=-int(np.sign(zz[d]))
        if side=='long' and sd<0: continue
        if side=='short' and sd>0: continue
        e=g.RO[d]; p,x=simx(d,930,sd,stop*g.ADR[d] if stop else None,xk,entry=e); out.append((d,sd,930,x,e,p))
    if ret: return out
    return R(out,lbl or f'{src} n{n} z{z} {side} stop{stop}',show=show)
print(M.MKT)
for src in ('oc','cc'):
    for n in (1,2,3,5,8,10):
        for z in (0,0.5,1.0):
            run(n,z,src)
print('-- sides (oc n5)')
for n in (3,5):
  for z in (0,0.5,1.0):
    run(n,z,'oc','long'); run(n,z,'oc','short')
print('-- stops (oc n5 z0.5)')
for st in (0.3,0.5,0.75,1.0): run(5,0.5,'oc',stop=st)
