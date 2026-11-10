import warnings; warnings.filterwarnings('ignore')
from ref import *
import pickle
S={'NBM':nbm(),'GOD':god(conservative=False),'TVP':tvp()}
a=pd.concat([d.assign(s=k) for k,d in S.items()]); a=a[TD[a.day.values]>='2017-01-01'].sort_values(['day','em','s']).reset_index(drop=True)
keep=[]; open_=[]
for r in a.itertuples():
    open_=[o for o in open_ if o[0]==r.day and o[1]>r.em]      # still open at this entry minute
    if any(o[2]!=r.dir for o in open_): keep.append(False); continue
    keep.append(True); open_.append((r.day,r.xm,r.dir))
a['keep']=keep
net=lambda d:((d.pts-COST)*2)
print('all signals: n',len(a),'net',round(net(a).sum()),' | kept n',int(a.keep.sum()),'net',round(net(a[a.keep]).sum()),' | skipped (opposite to an open leg) n',int((~a.keep).sum()),'net',round(net(a[~a.keep]).sum()))
print(a[~a.keep].groupby('s').apply(lambda d:pd.Series(dict(n=len(d),net=net(d).sum()))).round(0).to_string())
k=a[a.keep]; dly=net(k).groupby(TD[k.day.values]).sum().reindex(pd.bdate_range('2017-01-01','2024-03-07'),fill_value=0); mo=dly.resample('ME').sum(); eq=dly.cumsum()
print(f"combined single account: $/mo {mo.mean():.0f} med {mo.median():.0f} worst {mo.min():.0f} dd {(eq-eq.cummax()).min():.0f} sharpe {dly.mean()/dly.std()*252**.5:.2f}")
pickle.dump(a,open('comb_ref.pkl','wb'))
