from ref import *
import pickle
S={'NBM':nbm(),'GOD':god(),'TVP':tvp()}
pickle.dump(S,open('trades.pkl','wb'))
def net(d,cost=COST): return (d.pts-cost)*2
daily=pd.DataFrame({k:net(v).groupby(TD[v.day.values]).sum() for k,v in S.items()}).reindex(TD[FULL]).fillna(0)
daily=daily[daily.index>='2017-01-01']
print('daily corr\n',daily.corr().round(2)); print('corr on days both trade:',{f'{a}-{b}':round(daily[(daily[a]!=0)&(daily[b]!=0)][[a,b]].corr().iloc[0,1],2) for a,b in (('NBM','GOD'),('NBM','TVP'),('GOD','TVP'))})
daily['PORT']=daily.sum(1)
def summ(x,label):
    m=x.resample('ME').sum(); eq=x.cumsum(); dd=(eq-eq.cummax()).min(); y=x.resample('YE').sum()
    print(f"{label:6s} net={x.sum():8.0f} mo={m.mean():6.0f} med={m.median():6.0f} best={m.max():6.0f} worst={m.min():6.0f} +m={(m>0).sum()}/{len(m)} dd={dd:7.0f} sharpe={x.mean()/x.std()*np.sqrt(252):.2f} worstday={x.min():.0f}")
    return m
print('--- per 1 MNQ each, net of $2.50 RT, 2017-01..2024-03')
M={k:summ(daily[k],k) for k in daily}
print('yearly\n',daily.resample('YE').sum().round(0).to_string())
print('cost x2 ($5 RT):',{k:round(sum(net(v[TD[v.day.values]>='2017-01-01'],2.5))/87) for k,v in S.items()},'per month')
# trade stats
for k,v in S.items():
    v=v[TD[v.day.values]>='2017-01-01']; n=net(v); w=n[n>0]; l=n[n<=0]
    streak=mx=0
    for x in n: streak=streak+1 if x<=0 else 0; mx=max(mx,streak)
    top3=n.nlargest(3).sum()/n.sum()
    print(f"{k} trades={len(n)} wr={len(w)/len(n):.2f} pf={w.sum()/-l.sum():.2f} exp={n.mean():.1f} avgW={w.mean():.0f} avgL={l.mean():.0f} maxL={l.min():.0f} maxW={w.max():.0f} loseStreak={mx} top3share={top3:.2f} trades/day={len(n)/len(daily):.2f} avgRiskPts(stop)= long%={(v.dir>0).mean():.2f}")
# start shifted by 2 weeks, by half year
for k in daily: print(k,'start+10d mo:',round(daily[k].iloc[10:].resample('ME').sum().mean()),' rolling 12m min:',round(daily[k].rolling(252).sum().min()),' rolling 6m min:',round(daily[k].rolling(126).sum().min()))
# pct-of-price scaling: P&L per contract as bp of index notional
px=pd.Series(DO,index=TD).reindex(daily.index)
bp=(daily['PORT']/2/px*1e4)
print('PORT avg bp/day of index:',bp.groupby(bp.index.year).mean().round(2).to_dict())
for lvl in (18000,21000,25000): print(f'implied $/mo for 1 MNQ each at NQ={lvl}: all-years avg {bp.mean()*lvl*2/1e4*21:.0f}; worst-year {bp.groupby(bp.index.year).mean().min()*lvl*2/1e4*21:.0f}')
# Monte Carlo: bootstrap months / daily blocks
rng=np.random.default_rng(7); x=daily['PORT'].values; nb=5
res=[]; 
for _ in range(5000):
    idx=rng.integers(0,len(x)-nb,size=252//nb); p=np.concatenate([x[i:i+nb] for i in idx]); eq=np.cumsum(p); res.append((eq[-1],(eq-np.maximum.accumulate(eq)).min()))
res=np.array(res); print('MC 1y (block bootstrap) PORT: P(loss year)=%.3f  median=%.0f  5%%=%.0f  DD median=%.0f  DD 95%%=%.0f DD 99%%=%.0f'%((res[:,0]<0).mean(),np.median(res[:,0]),np.percentile(res[:,0],5),np.median(res[:,1]),np.percentile(res[:,1],5),np.percentile(res[:,1],1)))
daily.to_pickle('daily.pkl')
# ---- overnight edge
F=pickle.load(open('full.pkl','rb')); tds=sorted(F)
rows=[]
for a,b in zip(tds[:-1],tds[1:]):
    if (b-a).days>4: continue
    A=F[a];Bn=F[b]
    rows.append((b,Bn[930,0]-A[1314,3],Bn[540,3]-Bn[0,0],Bn[930,0]-Bn[540,3],A[1314,3]-A[930,0]))
ov=pd.DataFrame(rows,columns=['d','c2o','asia','eu_pre','rth']).set_index('d'); ov=ov[ov.index>='2017-01-01']
for c in ov:
    x=(ov[c]-1.25)*2; m=x.resample('ME').sum(); eq=x.cumsum()
    print(f"hold long {c:7s} 1 MNQ: mo={m.mean():6.0f} med={m.median():6.0f} worstM={m.min():6.0f} dd={(eq-eq.cummax()).min():7.0f} sharpe={x.mean()/x.std()*np.sqrt(252):.2f}  IS={x[:'2021'].mean():.1f}/day OOS={x['2022':].mean():.1f}/day worst night={x.min():.0f}")
