import pandas as pd, numpy as np, warnings, itertools; warnings.filterwarnings('ignore')
names={'DWR':'DailyWashoutRebound','VAR':'ValueAreaReentry','CEB':'CompressionExpansionBreakout'}
COST=2.50; T={}
for k,nm in names.items():
    m=pd.read_csv(f'build/tr_{nm}.csv',header=None,names=['ed','et','xd','xt','dir','ep','xp','q','pl'])
    m['date']=pd.Timestamp('1899-12-30')+pd.to_timedelta(m.ed,unit='D'); m=m[m.date>='2017-01-01'].reset_index(drop=True)
    m['net']=m.pl-COST*m.q; T[k]=m
days=pd.bdate_range('2017-01-01','2024-03-07')
def rep(label,tr):
    n=tr.net; d=n.groupby(tr.date).sum().reindex(days,fill_value=0.0); mo=d.resample('ME').sum(); eq=d.cumsum(); dd=(eq-eq.cummax()).min()
    teq=n.cumsum(); tdd=(teq-teq.cummax()).min()
    w=n[n>0]; l=n[n<=0]; st=mx=0
    for x in n: st=st+1 if x<=0 else 0; mx=max(mx,st)
    print(f"{label}: net ${n.sum():.0f} | month avg {mo.mean():.0f} median {mo.median():.0f} best {mo.max():.0f} worst {mo.min():.0f} | +months {(mo>0).sum()} / -months {(mo<0).sum()} / flat {(mo==0).sum()} of {len(mo)} | trades {len(n)} | win {len(w)/len(n)*100:.1f}% | PF {w.sum()/-l.sum():.2f} | exp ${n.mean():.2f} | maxDD (daily) {dd:.0f} (trade seq {tdd:.0f}) | longest losing streak {mx} | avg win {w.mean():.0f} avg loss {l.mean():.0f} | trades/day {len(n)/len(days):.2f} | costs ${COST*tr.q.sum():.0f} | worst trade {n.min():.0f} best {n.max():.0f} | top-3 share {n.nlargest(3).sum()/n.sum()*100:.0f}% | long n {(tr.dir>0).sum()} net {n[tr.dir>0].sum():.0f}, short n {(tr.dir<0).sum()} net {n[tr.dir<0].sum():.0f} | Sharpe {d.mean()/d.std()*np.sqrt(252):.2f} | t {n.mean()/n.std()*np.sqrt(len(n)):.2f}")
    is_=tr[tr.date<'2022-01-01'].net; oo=tr[tr.date>='2022-01-01'].net
    pf=lambda x: x[x>0].sum()/-x[x<=0].sum()
    print(f"     2017-21: {len(is_)} trades PF {pf(is_):.2f} ${is_.sum()/60:.0f}/mo | 2022-24: {len(oo)} trades PF {pf(oo):.2f} ${oo.sum()/(26+7/31):.0f}/mo | cost $5.00: ${(tr.pl-5*tr.q).sum()/87:.0f}/mo PF {pf(tr.pl-5*tr.q):.2f} | cost $7.50: ${(tr.pl-7.5*tr.q).sum()/87:.0f}/mo")
    return d
D={k:rep(k,m) for k,m in T.items()}
P=pd.DataFrame(D); P['PORT']=P.sum(axis=1)
allt=pd.concat(T.values()).sort_values(['ed','et']).reset_index(drop=True); rep('PORT',allt)
print('daily corr:\n',P[['DWR','VAR','CEB']].corr().round(2).to_string())
print('monthly corr:\n',P[['DWR','VAR','CEB']].resample('ME').sum().corr().round(2).to_string())
print('yearly net, 1 MNQ each:\n',P.resample('YE').sum().round(0).astype(int).to_string())
x=P.PORT; print('rolling 12m min/median:',round(x.rolling(252).sum().min()),round(x.rolling(252).sum().median()),'| max losing months in a row:',max(len(list(g)) for kk,g in itertools.groupby(x.resample('ME').sum()<=0) if kk))
both=((P.DWR!=0).astype(int)+(P.VAR!=0)+(P.CEB!=0)); print('days with 0/1/2/3 strategies trading:',both.value_counts().sort_index().to_dict(),'| worst day',round(x.min()),'best day',round(x.max()))
rng=np.random.default_rng(11); v=x.values
for nm,series in (('PORT',v),('DWR',P.DWR.values),('VAR',P.VAR.values),('CEB',P.CEB.values)):
    res=[]
    for _ in range(10000):
        idx=rng.integers(0,len(series)-5,size=50); p=np.concatenate([series[i:i+5] for i in idx]); e=np.cumsum(p); res.append((e[-1],(e-np.maximum.accumulate(e)).min()))
    res=np.array(res); print(f'MC 1y {nm}: P(losing year) {(res[:,0]<0).mean()*100:.1f}% | median ${np.median(res[:,0]):.0f} | 5th pct ${np.percentile(res[:,0],5):.0f} | DD median ${np.median(res[:,1]):.0f}, 95th ${np.percentile(res[:,1],5):.0f}, 99th ${np.percentile(res[:,1],1):.0f}')
    if nm=='PORT': RES=res
for acct in (10000,25000,50000):
    for q in (1,2,4,6): print(f'  account ${acct}, {q} MNQ per script: P(DD >= 25% of account in a year) {(RES[:,1]*q<=-0.25*acct).mean()*100:.1f}% | P(>=50%) {(RES[:,1]*q<=-0.5*acct).mean()*100:.1f}% | median year ${np.median(RES[:,0])*q:.0f}')
# edge in bp of index level (unadjusted level)
z=np.load('NQ.npz',allow_pickle=True); lv=pd.Series(pd.DataFrame(z['c']).ffill(axis=1).iloc[:,1319].values,index=pd.to_datetime(z['td'])).reindex(days).ffill()
bp=(P.PORT/2/lv*1e4); yr=bp.groupby(bp.index.year).mean()
print('PORT edge, bp of index per day by year:',yr.round(2).to_dict(),'| avg index level',round(lv.mean()),'| mean bp/day',round(bp.mean(),3))
for L in (20000,25000): print(f'  same bp edge at NQ {L}: ${bp.mean()*L*2/1e4*21:.0f}/month (all years), ${bp["2022":].mean()*L*2/1e4*21:.0f} (2022-24 rate), ${bp[:"2021"].mean()*L*2/1e4*21:.0f} (2017-21 rate), worst-year rate ${yr.min()*L*2/1e4*21:.0f}; drawdown scale x{L/lv.mean():.2f}')
# losing-streak expectation
for k,m in T.items():
    p=(m.net<=0).mean(); n=len(m); print(f'{k}: loss rate {p:.2f}, expected longest losing streak over {n} trades ~{np.log(n)/-np.log(p):.1f}')
P.to_pickle('P4.pkl')
