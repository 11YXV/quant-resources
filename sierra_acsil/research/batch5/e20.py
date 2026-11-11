import os, warnings; warnings.filterwarnings('ignore')
from S5 import *
def EOD(t=329,p=0.01,stop=0.25,xm=385):
    tr=[]
    for D in r.days:
        if not ok(D) or not r.has[D.i,t]: continue
        mv=r.c1[D.i,t]-D.prevClose
        if abs(mv/D.prevClose)<p: continue
        tr.append(trade_m(D,t+1,int(np.sign(mv)),stop*D.adr if stop else None,xm=xm))
    return [x for x in tr if x]
t=L(EOD(),'base: 15:00, |move|>=1.0%, stop 0.25, exit 15:55')
print('  by year $:',t.groupby(t.date.dt.year).net.sum().round(0).astype(int).to_dict(),'| trades/yr',t.groupby(t.date.dt.year).size().to_dict())
print('  long:',round(t[t.side>0].net.sum()),'short:',round(t[t.side<0].net.sum()))
cnt=0; pos=0
for tt,nm in ((299,'14:30'),(314,'14:45'),(329,'15:00'),(344,'15:15')):
  for p in (0.0075,0.01,0.0125,0.015):
    for st in (0.15,0.25,0.4,None):
        x=r.stats(EOD(t=tt,p=p,stop=st)); a=x[x.date<'2022-01-01']; b=x[x.date>='2022-01-01']
        pf=lambda s: s.net[s.net>0].sum()/max(1e-9,-s.net[s.net<=0].sum())
        cnt+=1; pos+=(pf(a)>1)&(pf(b)>1)
        print(f'  entry {nm} |move|>={p*100:.2f}% stop {st}: IS PF {pf(a):.2f} ${a.net.sum()/60:4.0f}/mo | OOS PF {pf(b):.2f} ${b.net.sum()/26.2:4.0f}/mo')
print(f'positive in both periods: {pos} of {cnt}')
for xm,nm in ((379,'15:50'),(385,'15:55'),(389,'15:59')): L(EOD(xm=xm),f'exit {nm}')
P4=pd.read_pickle('P4.pkl')
m=pd.read_csv('build/tr_suite.csv',header=None,names=['ed','et','xd','xt','dir','ep','xp','q','pl'])
m['date']=pd.Timestamp('1899-12-30')+pd.to_timedelta(m.ed,unit='D'); suite=(m.pl-2.5*m.q).groupby(m.date).sum().reindex(P4.index,fill_value=0)
dd=t.groupby('date').net.sum().reindex(P4.index,fill_value=0)
print('daily corr: suite',round(dd.corr(suite),2),'| DWR',round(dd.corr(P4.DWR),2),'VAR',round(dd.corr(P4.VAR),2),'CEB',round(dd.corr(P4.CEB),2),'| trio',round(dd.corr(P4.PORT),2))
