import pandas as pd, numpy as np, sys
s=sys.argv[1]
c=pd.read_pickle(f'{s}/cont.pkl')
c['td']=pd.to_datetime(c.td)
c['k']=(c.m-1080)%1440
c=c[c.k<1380]
tds=np.array(sorted(c.td.unique()))
ix={t:i for i,t in enumerate(tds)}
di=c.td.map(ix).values; k=c.k.values
A={}
for f in 'ohlcv':
    a=np.full((len(tds),1380),np.nan); a[di,k]=c[f].values; A[f]=a
sym=c.groupby('td').sym.first().reindex(tds).values
np.savez_compressed(f'{s}.npz',td=tds,sym=sym,**A)
n=np.isfinite(A['c'])
print(s,len(tds),tds[0],tds[-1],'rth bars median',np.median(n[:,930:1320].sum(1)),'on bars median',np.median(n[:,:930].sum(1)))
