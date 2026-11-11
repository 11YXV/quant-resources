# End-of-day flow candidate: robustness. Usage: MKT=NQ|ES|YM python3 e18.py
import os, warnings; warnings.filterwarnings('ignore')
from S5 import *
MK=os.environ.get('MKT','NQ')
def EOD(t=329,z=0.5,ref='pc',stop=0.25,xm=385,side='both'):
    tr=[]
    for D in r.days:
        if not ok(D): continue
        d=D.i
        if not r.has[d,t]: continue
        base=D.prevClose if ref=='pc' else D.dOpen
        x=(r.c1[d,t]-base)/D.adr
        if abs(x)<z: continue
        s=int(np.sign(x))
        if side=='long' and s<0 or side=='short' and s>0: continue
        tr.append(trade_m(D,t+1,s,stop*D.adr if stop else None,xm=xm))
    return [x for x in tr if x]
base=dict(t=329,z=0.5,ref='pc',stop=0.25,xm=385)
def run(lbl,**kw):
    a=dict(base); a.update(kw); return L(EOD(**a),lbl)
print(MK,'== base: 15:00, |prior close -> 15:00| > 0.5 avg range, stop 0.25, exit 15:55')
t=run('base')
if t is not None: print('   by year $:',t.groupby(t.date.dt.year).net.sum().round(0).astype(int).to_dict())
for xm,nm in ((389,'15:59'),(390,'16:00 close')):
    tt=run(f'exit {nm}',xm=min(xm,389) if xm==389 else 389) if xm==389 else None
# exit at the 16:00 close print (last trade of minute 15:59)
def EODclose(**kw):
    out=[]
    for x in EOD(**dict(base,xm=389,**kw)):
        if x[6]=='EOD':
            d=x[0]; c=r.c1[d,389] if r.has[d,389] else r.o1[d,389]
            out.append((x[0],x[1],x[2],389,x[4],x[1]*(c-x[4]),'CLOSE'))
        else: out.append(x)
    return out
t2=L(EODclose(),'exit at the 16:00 close (last trade)')
if t2 is not None: print('   by year $:',t2.groupby(t2.date.dt.year).net.sum().round(0).astype(int).to_dict())
print('-- entry time (exit 15:59)')
for t_,nm in ((299,'14:30'),(314,'14:45'),(329,'15:00'),(344,'15:15'),(359,'15:30')): run(f'entry {nm}',t=t_,xm=389)
print('-- threshold (exit 15:59)')
for z in (0.0,0.25,0.5,0.75,1.0): run(f'|move|>{z}',z=z,xm=389)
print('-- stop (exit 15:59)')
for st in (None,0.1,0.15,0.25,0.4): run(f'stop {st}',stop=st,xm=389)
print('-- reference (exit 15:59)')
run('ref = today open',ref='open',xm=389)
print('-- sides (exit 15:59)')
run('long only',side='long',xm=389); run('short only',side='short',xm=389)
