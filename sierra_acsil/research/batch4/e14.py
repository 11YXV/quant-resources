from M import *
import M
D=base()
# NBM proxy
MOVE=np.abs(g.CF[:,930:1320]/g.RO[:,None]-1); MOVE[~g.FULL]=np.nan
SIG=pd.DataFrame(MOVE).rolling(14,min_periods=10).mean().shift(1).values
TPx=(g.HF[:,930:1320]+g.LF[:,930:1320]+g.CF[:,930:1320])/3; Vv=np.maximum(g.V[:,930:1320],1)
VW=np.cumsum(TPx*Vv,1)/np.cumsum(Vv,1)
def NBM():
    out=[]
    for d in D:
        if not np.isfinite(SIG[d,30]): continue
        pos=0;e=0
        for t in range(29,385,30):
            p=g.CF[d,930+t]; ub=max(g.RO[d]*(1+SIG[d,t]),VW[d,t]); lb=min(g.RO[d]*(1-SIG[d,t]),VW[d,t])
            if pos==0 and t<=359:
                if p>ub: pos=1;e=p;ek=930+t
                elif p<lb: pos=-1;e=p;ek=930+t
            elif pos==1 and p<ub: out.append((d,1,ek,930+t,e,p-e)); pos=0
            elif pos==-1 and p>lb: out.append((d,-1,ek,930+t,e,e-p)); pos=0
        if pos: out.append((d,pos,ek,1314,e,pos*(g.CF[d,1314]-e)))
    return out
S={'NBMproxy':NBM(),'DWR':DWR(th=25,stop=0.75),'VAR':VAR(step=30,nclose=1,stop=0.2),'PIF':PIF(th=0.2,stop=0.4),'CMP':CMP('nr4',30,stopm='0.25')}
dly={}
for nm,tr in S.items():
    R(tr,nm)
    s=pd.Series([ (t[5]-1.25)*2 for t in tr],index=[g.TD[t[0]] for t in tr]).groupby(level=0).sum()
    dly[nm]=s
df=pd.DataFrame(dly).reindex(g.TD[D]).fillna(0)
print(df.corr().round(2))
mo=df.groupby(df.index.to_period('M')).sum()
print('monthly corr\n',mo.corr().round(2))
yr=df.groupby(df.index.year).sum().round(0); print(yr)
for combo in (['DWR','VAR','PIF'],['DWR','VAR','CMP'],['DWR','VAR','PIF','CMP']):
    p=df[combo].sum(1); m=mo[combo].sum(1); eq=p.cumsum()
    print(combo,'mo',round(m.mean()),'med',round(m.median()),'+m',round((m>0).mean(),2),'worst',round(m.min()),'dd',round((eq-eq.cummax()).min()),'IS',round(m[:'2021-12'].mean()),'OOS',round(m['2022-01':].mean()),'sharpe',round(p.mean()/p.std()*np.sqrt(252),2))
import pickle; pickle.dump(S,open('S.pkl','wb'))
