from S5 import *
import warnings; warnings.filterwarnings('ignore')
NQ=NQd.copy()
# regime: average range relative to price, percentile vs the past 250 sessions (known before the open)
adrp=pd.Series({D.td:D.adr/D.dOpen if D.adr>0 else np.nan for D in r.days})
pct=adrp.rolling(250,min_periods=120).apply(lambda x:(x[:-1]<x[-1]).mean() if len(x)>1 else np.nan,raw=True)
print('== a: VWAP fade only in high-volatility regimes (30-min marks 10:30-14:30, exit at VWAP touch or 15:55) ==')
for hi in (0.67,0.8):
  for k in (0.25,0.4):
    for st in (0.25,0.5):
        tr=[]
        for D in r.days:
            if not ok(D) or not np.isfinite(pct.get(D.td,np.nan)) or pct[D.td]<hi: continue
            d=D.i; pv=0; vv=0; vw=np.full(390,np.nan)
            for q in range(390):
                if r.has[d,q]: w=max(r.v1[d,q],1); pv+=w*(r.h1[d,q]+r.l1[d,q]+r.c1[d,q])/3; vv+=w
                vw[q]=pv/vv if vv else np.nan
            for q in range(59,300,30):
                if not r.has[d,q]: continue
                dev=(r.c1[d,q]-vw[q])/D.adr
                if abs(dev)>k:
                    s=-int(np.sign(dev)); e=q+1; dist=st*D.adr; mins=np.nonzero(r.has[d])[0]; mins=mins[mins>=e]
                    if len(mins)==0: break
                    px=r.o1[d,mins[0]]; stop=px-s*dist; res=None
                    for m in mins:
                        if m>=385: res=s*(r.o1[d,m]-px); break
                        if (s>0 and r.l1[d,m]<=stop) or (s<0 and r.h1[d,m]>=stop): res=-dist; break
                        if (s>0 and r.h1[d,m]>=vw[m-1]) or (s<0 and r.l1[d,m]<=vw[m-1]): res=s*(vw[m-1]-px); break
                    if res is None: res=s*(r.c1[d,mins[-1]]-px)
                    tr.append((d,s,e,0,px,res,'X')); break
        L(tr,f'hi-vol(>{hi}) VWAP fade k{k} stop{st}')
print('== c: sell-side washout (RSI2 of day closes high) short open->15:55 ==')
for th in (90,95,97):
  for st in (0.5,0.75):
    tr=[r.trade(D,-1,-1,st*D.adr) for D in r.days if D.adr>0 and D.prevClose>0 and not D.halt and D.rsi is not None and r.PRE[D.i] and D.rsi>th]
    L(tr,f'short RSI2>{th} stop{st}')
print('== b: end-of-day hedging flow (Baltussen): sign of prior close->t, trade t->15:55 ==')
P4=pd.read_pickle('P4.pkl')
m=pd.read_csv('build/tr_suite.csv',header=None,names=['ed','et','xd','xt','dir','ep','xp','q','pl'])
m['date']=pd.Timestamp('1899-12-30')+pd.to_timedelta(m.ed,unit='D'); suite=(m.pl-2.5*m.q).groupby(m.date).sum()
for t,nm in ((329,'15:00'),(299,'14:30'),(359,'15:30')):
    for z in (0.25,0.5,0.75):
        tr=[]
        for D in r.days:
            if not ok(D): continue
            d=D.i
            if not r.has[d,t]: continue
            x=(r.c1[d,t]-D.prevClose)/D.adr
            if abs(x)<z: continue
            tr.append(trade_m(D,t+1,int(np.sign(x)),0.25*D.adr))
        t_=L(tr,f'EOD flow from {nm} |move|>{z}adr')
        if t_ is not None and z==0.5:
            dd=t_.groupby('date').net.sum().reindex(P4.index,fill_value=0)
            print(f'      daily corr with MomentumSuite {dd.corr(suite.reindex(P4.index,fill_value=0)):.2f}, with new trio {dd.corr(P4.PORT):.2f}')
