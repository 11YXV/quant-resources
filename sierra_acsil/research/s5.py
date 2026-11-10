from lib import *
from s1 import noise
from s2 import orb
def cond(d,mask_day,label):
    stats(d[mask_day[d.day.values]],label=label)
# --- ORB filters
n=5
base=orb(n=n,stopmode='adr',stopfrac=0.1)
rv=V[:,:n].sum(1); rvol=rv/pd.Series(rv).rolling(14).mean().shift(1).values
orr=(H[:,:n].max(1)-L[:,:n].min(1))/ADR
dr=pd.Series(RNG); nr7=(dr.shift(1)<=dr.shift(1).rolling(7).min()).values
dh=pd.Series(H.max(1)); dl=pd.Series(L.min(1)); inside=((dh.shift(1)<dh.shift(2))&(dl.shift(1)>dl.shift(2))).values
print('--- ORB5 adr0.1 run-to-close filters')
stats(base,label='base')
for lo,hi in ((0,0.8),(0.8,1.2),(1.2,9)): cond(base,(rvol>=lo)&(rvol<hi),f'rvol {lo}-{hi}')
for lo,hi in ((0,0.12),(0.12,0.2),(0.2,9)): cond(base,(orr>=lo)&(orr<hi),f'orRange/ADR {lo}-{hi}')
cond(base,nr7,'after NR7'); cond(base,inside,'after inside day'); cond(base,~nr7,'not NR7')
gap=(DO-PC)/ADR
b=base.copy(); b['g']=gap[b.day.values]
stats(b[np.sign(b.g)==b.dir],label='dir==gap'); stats(b[np.sign(b.g)!=b.dir],label='dir!=gap')
# first bar direction
fb=np.sign(C[:,n-1]-O[:,0]); b['fb']=fb[b.day.values]
stats(b[b.fb==b.dir],label='dir==firstbar'); stats(b[b.fb!=b.dir],label='dir!=firstbar')
stats(b[b.dir>0],label='long'); stats(b[b.dir<0],label='short')
# --- daily correlation with NBM
nb=noise()
def daily(d): return d.groupby('day').pts.sum().reindex(range(N),fill_value=0)
for nm,o in (('orb5',base),('orb30',orb(n=30)),('orb60',orb(n=60,stopfrac=0.5))):
    x=daily(nb); y=daily(o); print(nm,'daily corr with NBM',round(np.corrcoef(x,y)[0,1],2), 'days both trade',((x!=0)&(y!=0)).sum())
# --- daily-conditioned open->close
print('--- open->close conditional')
oc=C[:,388]-O[:,1]
dc=pd.Series(C[:,389]); ret1=(dc/dc.shift(1)-1).shift(1).values
ma=dc.rolling(50).mean().shift(1).values; up=dc.shift(1).values>ma
def rsi(s,n=2):
    d=s.diff(); u=d.clip(lower=0).ewm(alpha=1/n,adjust=False).mean(); dn=(-d.clip(upper=0)).ewm(alpha=1/n,adjust=False).mean(); return 100-100/(1+u/dn)
r2=rsi(dc).shift(1).values
def oc_tr(mask,d=1): 
    idx=np.where(mask&~SHORT&~np.isnan(ADR))[0]; return pd.DataFrame(dict(day=idx,pts=d*oc[idx]))
stats(oc_tr(np.ones(N,bool)),label='always long O->C')
stats(oc_tr(r2<10),label='RSI2<10 long'); stats(oc_tr((r2<10)&up),label='RSI2<10 & >MA50 long'); stats(oc_tr(r2>90,-1),label='RSI2>90 short'); stats(oc_tr(r2>90),label='RSI2>90 long')
stats(oc_tr(up),label='>MA50 long'); stats(oc_tr(~up,-1),label='<MA50 short')
for wd in range(5): stats(oc_tr(TD.weekday==wd),label=f'weekday{wd} long',show=True)
