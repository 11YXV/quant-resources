import pandas as pd, numpy as np, pickle
from lib import stats, TD as RTD, COST
F=pickle.load(open('full.pkl','rb')); tds=sorted(F)
A=np.array([F[t] for t in tds]); FT=pd.DatetimeIndex(tds)
O,H,L,C,V=[A[:,:,j] for j in range(5)]
import lib
def st(d,label,cost=COST):
    # map day index to lib's TD by temporarily swapping
    lib.TD=FT; r=stats(d,cost=cost,label=label); lib.TD=RTD; return r
def onoise(k0=0,k1=925,vm=1.0,step=30,look=14,use_vwap=True,anchor_prev=True,first=30,last_entry=None,dirs=(1,-1)):
    """noise band anchored at session-start (k0) open; exit by k1"""
    n=len(tds); o0=O[:,k0]
    mv=np.abs(C[:,k0:k1+1]/o0[:,None]-1)
    sig=pd.DataFrame(mv).rolling(look).mean().shift(1).values
    tp=(H+L+C)/3; w=np.maximum(V,1)
    vw=np.cumsum((tp*w)[:,k0:k1+1],1)/np.cumsum(w[:,k0:k1+1],1)
    tr=[]; le=last_entry or (k1-k0-30)
    for i in range(look+1,n):
        if np.isnan(sig[i,0]): continue
        if (FT[i]-FT[i-1]).days>4: continue
        pc=C[i-1,1319] if k0==0 else C[i,k0-1]  # prior RTH close (16:00) when starting at 18:00
        ub0=max(o0[i],pc) if anchor_prev else o0[i]; lb0=min(o0[i],pc) if anchor_prev else o0[i]
        pos=0
        for t in range(first-1,k1-k0):
            px=C[i,k0+t]; UBt=ub0*(1+vm*sig[i,t]); LBt=lb0*(1-vm*sig[i,t])
            if (t+1)%step: continue
            if pos>0 and px<(max(UBt,vw[i,t]) if use_vwap else UBt): tr.append((i,et,t+1,pos,ep,O[i,k0+t+1])); pos=0
            elif pos<0 and px>(min(LBt,vw[i,t]) if use_vwap else LBt): tr.append((i,et,t+1,pos,ep,O[i,k0+t+1])); pos=0
            if pos==0 and t<le:
                if px>UBt and 1 in dirs: pos=1; ep=O[i,k0+t+1]; et=t+1
                elif px<LBt and -1 in dirs: pos=-1; ep=O[i,k0+t+1]; et=t+1
        if pos: tr.append((i,et,k1-k0,pos,ep,O[i,k1]))
    d=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp']); d['pts']=d.dir*(d.xp-d.ep); return d
if __name__=='__main__':
    st(onoise(930,1319),'RTH check (should ~match)')
    for k0,k1,nm in ((0,925,'18:00-09:25'),(540,925,'03:00-09:25'),(120,925,'20:00-09:25'),(0,540,'18:00-03:00'),(480,925,'02:00-09:25')):
        for vm in (1.0,1.5):
            st(onoise(k0,k1,vm=vm,anchor_prev=(k0==0)),f'ON {nm} vm{vm}')
    st(onoise(0,925,vm=1.0),'ON 18-0925 cost x2',cost=2.5)
    # 24h single anchor at 18:00 through 15:59
    st(onoise(0,1319,vm=1.0),'24h anchor 18:00')
