import pandas as pd, numpy as np, datetime as dt
a=pd.read_pickle('raw.pkl')
a['et']=a.ts.dt.tz_localize('UTC').dt.tz_convert('America/New_York').dt.tz_localize(None)
a['d']=a.et.dt.date
a['m']=a.et.dt.hour*60+a.et.dt.minute
a['td']=np.where(a.m>=1080,(a.et+pd.Timedelta(days=1)).dt.date,a.d)
def expiry(sym):
    y=int(sym[3:]); mo={'H':3,'M':6,'U':9,'Z':12}[sym[2]]
    d=dt.date(y,mo,1); d+=dt.timedelta((4-d.weekday())%7+14); return d
syms=sorted(a.sym.unique(),key=expiry); ex={s:expiry(s) for s in syms}
tds=sorted(a.td.unique())
front={}
for d in tds:
    for s in syms:
        if ex[s]-dt.timedelta(days=8)>=d: front[d]=s; break
fs=pd.Series(front,name='front')
a=a.merge(fs,left_on='td',right_index=True)
c=a[a.sym==a.front].sort_values('et').reset_index(drop=True)
c=c[pd.to_datetime(c.td).dt.weekday<5]
c.to_pickle('cont.pkl')
rr=c[(c.m>=570)&(c.m<960)]
n=rr.groupby('td').size()
print(len(n),(n>=380).sum(),(n<380).sum())
n.index=pd.to_datetime(n.index)
print((n>=380).groupby(n.index.year).agg(['sum','count']))
bad=n[n<380]; print(bad.groupby(bad.index.year).describe())
print(bad['2019'].head(30))
