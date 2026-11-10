import pandas as pd, numpy as np, warnings; warnings.filterwarnings('ignore')
names={'NBM':'NoiseBandMomentum_NQ_MNQ','GOD':'GapOpeningDrive_NQ_MNQ','TVP':'TrendDayVwapPullback_NQ_MNQ'}
COST=2.50
T={}
for k,nm in names.items():
    m=pd.read_csv(f'mock/tr_{nm}.csv',header=None,names=['ed','et','xd','xt','dir','ep','xp','q','pl'])
    m['date']=pd.Timestamp('1899-12-30')+pd.to_timedelta(m.ed,unit='D'); m=m[m.date>='2017-01-01'].reset_index(drop=True)
    m['net']=m.pl-COST*m.q; T[k]=m
days=pd.bdate_range('2017-01-01','2024-03-07')
def rep(net,dates,label,ntr=None,tr=None):
    d=net.groupby(dates).sum().reindex(days,fill_value=0.0); mo=d.resample('ME').sum(); eq=d.cumsum(); dd=(eq-eq.cummax()).min()
    s=f"{label:5s} net ${d.sum():8.0f} | mo avg {mo.mean():6.0f} med {mo.median():6.0f} best {mo.max():6.0f} worst {mo.min():6.0f} | +months {(mo>0).sum()}/{len(mo)} | maxDD {dd:7.0f} | daily Sharpe {d.mean()/d.std()*np.sqrt(252):.2f}"
    if tr is not None:
        n=tr.net; w=n[n>0]; l=n[n<=0]; st=mx=0
        for x in n: st=st+1 if x<=0 else 0; mx=max(mx,st)
        s+=f"\n      trades {len(n)} | win {len(w)/len(n)*100:.1f}% | PF {w.sum()/-l.sum():.2f} | exp ${n.mean():.2f} | avg win {w.mean():.0f} avg loss {l.mean():.0f} | worst {n.min():.0f} best {n.max():.0f} | longest losing streak {mx} | trades/day {len(n)/len(days):.2f} | costs ${COST*len(n):.0f} | top-3 share {n.nlargest(3).sum()/n.sum()*100:.0f}% | long {(tr.dir>0).mean()*100:.0f}% (long net {n[tr.dir>0].sum():.0f}, short net {n[tr.dir<0].sum():.0f})"
    print(s); return d
D={}
for k,m in T.items(): D[k]=rep(m.net,m.date,k,tr=m)
P=pd.DataFrame(D); P['PORT']=P.sum(axis=1)
allt=pd.concat(T.values()).sort_values(['ed','xt']); rep(allt.net,allt.date,'PORT',tr=allt)
print('daily corr:\n',P[['NBM','GOD','TVP']].corr().round(2).to_string())
print('yearly net per 1 MNQ each:\n',P.resample('YE').sum().round(0).astype(int).to_string())
for nm,sl in (('IS 2017-2021',slice('2017','2021')),('OOS 2022-2024',slice('2022','2024'))):
    x=P.loc[sl]; mo=x.resample('ME').sum(); print(nm,'monthly avg:',mo.mean().round(0).to_dict(),'| PORT maxDD',round((x.PORT.cumsum()-x.PORT.cumsum().cummax()).min()))
print('cost x2 ($5.00/RT) PORT monthly avg:',round((allt.pl-5*allt.q).sum()/87),' | cost x3:',round((allt.pl-7.5*allt.q).sum()/87))
for sh in (10,21,63): print(f'start +{sh} sessions: PORT monthly avg',round(P.PORT.iloc[sh:].resample("ME").sum().mean()))
print('rolling 12-month PORT min/median:',round(P.PORT.rolling(252).sum().min()),round(P.PORT.rolling(252).sum().median()),'| losing calendar months in a row max:',max(len(list(g)) for kk,g in __import__("itertools").groupby(P.PORT.resample("ME").sum()<=0) if kk))
rng=np.random.default_rng(11); x=P.PORT.values; res=[]
for _ in range(10000):
    idx=rng.integers(0,len(x)-5,size=50); p=np.concatenate([x[i:i+5] for i in idx]); e=np.cumsum(p); res.append((e[-1],(e-np.maximum.accumulate(e)).min()))
res=np.array(res); print('Monte Carlo, 1 year, 5-day blocks: P(losing year) %.1f%% | median $%.0f | 5th pct $%.0f | drawdown median $%.0f, 95th pct $%.0f, 99th pct $%.0f'%((res[:,0]<0).mean()*100,np.median(res[:,0]),np.percentile(res[:,0],5),np.median(res[:,1]),np.percentile(res[:,1],5),np.percentile(res[:,1],1)))
for acct in (5000,10000):
    for q in (1,2,3):
        ruin=(res[:,1]*q<=-0.5*acct).mean()*100; print(f'  account ${acct}, {q} MNQ per script: P(drawdown >= 50% of account within a year) {ruin:.1f}%')
px=pd.read_pickle('daily.pkl'); 
import pickle; days_=pickle.load(open('days.pkl','rb')); lvl=pd.Series({d['td']:d['o'][0] for d in days_}).reindex(days).ffill()
bp=(P.PORT/2/lvl*1e4); yr=bp.groupby(bp.index.year).mean()
print('PORT edge in bp of index per day by year:',yr.round(2).to_dict(),'| avg index level',round(lvl.mean()))
for L in (20000,25000): print(f'  if the same bp edge held at NQ {L}: ${bp.mean()*L*2/1e4*21:.0f}/month per 1 MNQ each (all years), ${bp["2022":].mean()*L*2/1e4*21:.0f} (2022-24 rate), ${yr.min()*L*2/1e4*21:.0f} (worst year rate)')
P.to_pickle('P.pkl')
