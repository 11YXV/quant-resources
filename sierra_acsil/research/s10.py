from lib import *
from s2 import sim_bracket
def mk(tr):
    df=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp']); df['pts']=df.dir*(df.xp-df.ep); return df
def pullback(vm=1.0,t1=330,sf=0.1,eod=389,bar=5,maxn=1,mode='confirm',trail_vwap=False,stopmode='adr',minwait=0,t0=29):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(SIG[i,0]) or np.isnan(PC[i]) or np.isnan(ADR[i]): continue
        UB=max(DO[i],PC[i])*(1+vm*SIG[i]); LB=min(DO[i],PC[i])*(1-vm*SIG[i]); td=0; n=0; t=t0; tset=0
        while t<t1 and n<maxn:
            c=C[i,t]
            if (t+1)%30==0:
                nd=1 if c>UB[t] else -1 if c<LB[t] else td
                if nd!=td: td=nd; tset=t
            if td==0 or t-tset<minwait or (t+1)%bar: t+=1; continue
            lo=L[i,t-bar+1:t+1].min(); hi=H[i,t-bar+1:t+1].max(); vw=VW[i,t]
            if td>0 and lo<=vw and c>vw: d=1
            elif td<0 and hi>=vw and c<vw: d=-1
            else: t+=1; continue
            ep=O[i,t+1]
            stop=ep-d*sf*ADR[i] if stopmode=='adr' else (lo if d>0 else hi)-d*sf*ADR[i]
            xt,xp=sim_bracket(i,t+1,d,ep,stop,None,eod)
            tr.append((i,t+1,xt,d,ep,xp)); n+=1; t=xt+1
    return mk(tr)
if __name__=='__main__':
    rows=[]
    for vm in (0.8,1.0,1.3):
        for sf in (0.05,0.1,0.15,0.2,0.3):
            for bar in (5,15):
                for maxn in (1,2):
                    d=pullback(vm=vm,sf=sf,bar=bar,maxn=maxn); r=stats(d,show=False)
                    rows.append((vm,sf,bar,maxn,r['IS']['n'],r['IS']['pf'],r['OOS']['pf'],r['ALL']['mo'],r['ALL']['dd'],r['ALL']['sh']))
    df=pd.DataFrame(rows,columns='vm sf bar maxn nIS pfIS pfOOS mo dd sh'.split()); print(df.round(2).to_string())
    print((df.pfIS>1).mean(),(df.pfOOS>1).mean(),df.pfIS.median(),df.pfOOS.median())
    for sm in ('adr','swing'): stats(pullback(sf=0.1 if sm=='adr' else 0.02,stopmode=sm),label=f'pullback stop {sm}')
