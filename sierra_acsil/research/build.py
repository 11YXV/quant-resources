import pandas as pd, numpy as np, glob, os, re
D='/home/user/yakki-minati/us.index.futures.data/NQ/'
fr=[]
for f in sorted(glob.glob(D+'*.csv')):
    n=os.path.basename(f)[:-4]
    if n=='NQH20201': n='NQH2021'
    d=pd.read_csv(f,sep='\t')
    d.columns=['date','time','o','h','l','c','tv','v','sp']
    d['ts']=pd.to_datetime(d.date+' '+d.time,format='%Y.%m.%d %H:%M:%S')
    d['sym']=n
    fr.append(d[['ts','o','h','l','c','tv','v','sym']])
    print(n,len(d),d.ts.iloc[0],d.ts.iloc[-1], d.v.sum())
a=pd.concat(fr); a.to_pickle('raw.pkl')
