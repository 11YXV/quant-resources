import pandas as pd, numpy as np, pickle
c=pd.read_pickle('cont.pkl'); c['td']=pd.to_datetime(c.td)
# full-day minute grid: 18:00 prev (minute index 0) .. 16:59 (index 1379)
c['k']=np.where(c.m>=1080,c.m-1080,c.m+360)
out={}
for td,g in c.groupby('td'):
    g=g[g.k<1380].drop_duplicates('k').set_index('k')[['o','h','l','c','v']].reindex(range(1380))
    n=g.c.notna().sum()
    if n<900: continue
    cl=g.c.ffill().bfill()
    for q in 'ohl': g[q]=g[q].fillna(cl)
    g['c']=cl; g['v']=g.v.fillna(0)
    out[td]=g.values.astype(float)
pickle.dump(out,open('full.pkl','wb')); print(len(out))
