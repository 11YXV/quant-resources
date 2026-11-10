import pandas as pd, numpy as np, pickle
c=pd.read_pickle('cont.pkl')
c['td']=pd.to_datetime(c.td)
days=[]
prev=None
for td,g in c.groupby('td'):
    r=g[(g.m>=570)&(g.m<960)]
    if len(r)<380:   # holiday/short/incomplete: skip but keep as prev ref? skip entirely
        prev=None if len(r)<150 else prev
        short=True
    else: short=False
    if len(r)<150: continue
    idx=np.arange(570,960)
    r2=r.set_index('m')[['o','h','l','c','v']].reindex(idx)
    cl=r2.c.ffill().bfill()
    for k in 'ohl': r2[k]=r2[k].fillna(cl)
    r2['c']=cl; r2['v']=r2.v.fillna(0)
    on=g[(g.m>=1080)|(g.m<570)]
    d=dict(td=td,sym=g.sym.iloc[0],o=r2.o.values,h=r2.h.values,l=r2.l.values,c=r2.c.values,v=r2.v.values,
           onh=on.h.max() if len(on) else np.nan,onl=on.l.min() if len(on) else np.nan,short=short,n=len(r))
    days.append(d)
# previous-day refs only when same contract (else adjust by overlap gap: use raw table)
raw=pd.read_pickle('raw.pkl')
raw['et']=raw.ts.dt.tz_localize('UTC').dt.tz_convert('America/New_York').dt.tz_localize(None)
raw['m']=raw.et.dt.hour*60+raw.et.dt.minute; raw['d']=raw.et.dt.normalize()
rr=raw[(raw.m>=570)&(raw.m<960)]
dl=rr.groupby(['sym','d']).agg(h=('h','max'),l=('l','min'),c=('c','last'),o=('o','first'),n=('c','size'))
for i,d in enumerate(days):
    d['ph']=d['pl']=d['pc']=np.nan
    if i==0: continue
    p=days[i-1]
    key=(d['sym'],p['td'])
    if key in dl.index and dl.loc[key,'n']>=140:
        x=dl.loc[key]; d['ph'],d['pl'],d['pc']=x.h,x.l,x.c
    d['gapdays']=(d['td']-p['td']).days
pickle.dump(days,open('days.pkl','wb'))
print(len(days),sum(d['short'] for d in days),sum(np.isnan(d['pc']) for d in days),days[0]['td'],days[-1]['td'])
