from lib import *
def noise(vm=1.0, step=30, use_vwap=True, start=30, last_entry=389, eod=389, gapadj=True, allow_long=True, allow_short=True, chk_exit_step=None, maxtr=99):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(SIG[i,0]) or np.isnan(PC[i]): continue
        o,c,h,l=O[i],C[i],H[i],L[i]
        ub0=max(DO[i],PC[i]) if gapadj else DO[i]; lb0=min(DO[i],PC[i]) if gapadj else DO[i]
        UB=ub0*(1+vm*SIG[i]); LB=lb0*(1-vm*SIG[i])
        pos=0; ep=0; et=0; nt=0
        es=chk_exit_step or step
        for t in range(start-1,eod):   # bar t closed -> act at open of t+1
            px=c[t]
            is_e=((t+1)%step==0); is_x=((t+1)%es==0)
            if pos!=0 and is_x:
                if pos>0:
                    stop=max(UB[t],VW[i,t]) if use_vwap else UB[t]
                    if px<stop: tr.append((i,et,t+1,pos,ep,o[t+1])); pos=0
                else:
                    stop=min(LB[t],VW[i,t]) if use_vwap else LB[t]
                    if px>stop: tr.append((i,et,t+1,pos,ep,o[t+1])); pos=0
            if pos==0 and is_e and t+1<=last_entry-1 and nt<maxtr:
                if px>UB[t] and allow_long and (not use_vwap or px>VW[i,t]): pos=1; ep=o[t+1]; et=t+1; nt+=1
                elif px<LB[t] and allow_short and (not use_vwap or px<VW[i,t]): pos=-1; ep=o[t+1]; et=t+1; nt+=1
        if pos!=0: tr.append((i,et,eod,pos,ep,o[eod]))
    d=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp']); d['pts']=d.dir*(d.xp-d.ep); return d
if __name__=='__main__':
    for vm in (0.8,1.0,1.2,1.5):
        for step in (30,15,5):
            d=noise(vm=vm,step=step); stats(d,label=f'noise vm{vm} step{step}')
    d=noise(use_vwap=False); stats(d,label='noise noVWAP')
    d=noise(gapadj=False); stats(d,label='noise nogap')
    d=noise(allow_short=False); stats(d,label='long only')
    d=noise(allow_long=False); stats(d,label='short only')
