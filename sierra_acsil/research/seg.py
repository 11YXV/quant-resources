import pandas as pd, numpy as np, pickle
F=pickle.load(open('full.pkl','rb'))
tds=sorted(F); 
def px(a,k): return a[k,3]
# segment boundaries in k (18:00=0): 03:00=540, 08:30=870, 09:30=930, 10:00=960, 10:30=990, 12:00=1080,14:00=1200,15:00=1260,15:30=1290,16:00=1320
B=[0,540,870,930,960,990,1080,1200,1260,1290,1319]
nm=['asia','eu','pre','f30','s30','am','mid','pm1','pm2','l30']
R=[]
for i,td in enumerate(tds):
    a=F[td]; r=[np.log(a[B[j+1],3]/a[B[j],3]) if j>0 else np.log(a[B[1],3]/a[0,0]) for j in range(len(B)-1)]
    R.append(r)
R=pd.DataFrame(R,index=tds,columns=nm)
print(R.describe().loc[['mean','std']].T.assign(t=lambda x:x['mean']/x['std']*np.sqrt(len(R))).round(5))
print(R.corr().round(2))
for split in (slice(None,'2021'),slice('2022',None)):
    print(R.loc[split].corr().round(2).loc[['asia','eu','pre','f30'],['eu','pre','f30','s30','am','mid','pm1','pm2','l30']])
# cumulative: return from 18:00 to X vs rest-of-day
R.to_pickle('seg.pkl')
