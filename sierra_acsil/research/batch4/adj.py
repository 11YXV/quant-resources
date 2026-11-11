# back-adjusted continuous 1-minute series (difference adjustment at each roll, like Sierra's continuous chart)
import pandas as pd, numpy as np, sys, datetime as dt
s=sys.argv[1]
c=pd.read_pickle(f'{s}/cont.pkl'); raw=pd.read_pickle(f'{s}/raw.pkl')
raw['et']=raw.ts.dt.tz_localize('UTC').dt.tz_convert('America/New_York').dt.tz_localize(None)
c['td']=pd.to_datetime(c.td)
fr=c.groupby('td').sym.first()
rolls=fr[fr!=fr.shift(1)].iloc[1:]           # first trading day of each new front contract
off={}; cum=0.0
syms=list(fr.unique())
adjs={syms[-1]:0.0}
# walk backward
for i in range(len(rolls)-1,-1,-1):
    td=rolls.index[i]; new=rolls.iloc[i]; old=fr[fr.index<td].iloc[-1]
    prevday=fr.index[fr.index<td][-1]
    t0=pd.Timestamp(prevday)+pd.Timedelta(hours=15,minutes=59)
    a=raw[(raw.sym==new)&(raw.et<=t0)&(raw.et>t0-pd.Timedelta(hours=3))]; b=raw[(raw.sym==old)&(raw.et<=t0)&(raw.et>t0-pd.Timedelta(hours=3))]
    if len(a)==0 or len(b)==0: print('no overlap',td,new,old); d=0.0
    else: d=a.c.iloc[-1]-b.c.iloc[-1]
    adjs[old]=adjs[new]+round(d*4)/4
print({k:round(v,2) for k,v in adjs.items()})
ad=c.sym.map(adjs).values
for f in 'ohlc': c[f]=c[f]+ad
c['k']=(c.m-1080)%1440
c=c[c.k<1380]
tds=np.array(sorted(c.td.unique())); ix={t:i for i,t in enumerate(tds)}
di=c.td.map(ix).values; k=c.k.values; A={}
for f in 'ohlcv':
    a=np.full((len(tds),1380),np.nan); a[di,k]=c[f].values; A[f]=a
np.savez_compressed(f'{s}adj.npz',td=tds,**A)
# RTH 1-minute csv for the mock: serial date, seconds, o,h,l,c,v
r=c[(c.m>=565)&(c.m<960)].sort_values('et')
ser=(r.td-pd.Timestamp('1899-12-30')).dt.days
out=pd.DataFrame(dict(d=ser.values,t=(r.m*60).values,o=r.o.values,h=r.h.values,l=r.l.values,c=r.c.values,v=r.v.values))
out.to_csv(f'{s}_bars.csv',header=False,index=False,float_format='%.2f')
print(s,len(tds),len(out))
