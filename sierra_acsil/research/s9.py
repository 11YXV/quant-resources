from lib import *
from s1 import noise
from s2 import sim_bracket
from s6 import god
def mk(tr):
    df=pd.DataFrame(tr,columns=['day','et','xt','dir','ep','xp']); df['pts']=df.dir*(df.xp-df.ep); return df
def lvl_break(level='pd',sf=0.1,t0=0,last=330,eod=389,buf=0.25,dirs=(1,-1),need_inside_open=True,first_only=True):
    tr=[]
    for i in range(20,N):
        if SHORT[i] or np.isnan(ADR[i]) or np.isnan(PH[i]): continue
        hi,lo=(PH[i],PL[i]) if level=='pd' else (ONH[i],ONL[i]) if level=='on' else (max(PH[i],ONH[i]),min(PL[i],ONL[i]))
        if np.isnan(hi): continue
        if need_inside_open and not(lo<DO[i]<hi): continue
        for t in range(t0,last):
            up=H[i,t]>=hi+buf; dn=L[i,t]<=lo-buf
            if up and dn: break
            if not(up or dn): continue
            d=1 if up else -1
            if d not in dirs: break
            ep=max(hi+buf,O[i,t]) if d>0 else min(lo-buf,O[i,t]); stop=ep-d*sf*ADR[i]
            if (d>0 and L[i,t]<=stop) or (d<0 and H[i,t]>=stop): xt,xp=t,stop
            else: xt,xp=sim_bracket(i,t,d,ep,stop,None,eod)
            tr.append((i,t,xt,d,ep,xp)); break
    return mk(tr)
if __name__=='__main__':
    nb=noise(); g=god(5,0.1)
    def daily(d): return d.groupby('day').pts.sum().reindex(range(N),fill_value=0)
    X=daily(nb); G=daily(g)
    def rep(d,l):
        stats(d,label=l); y=daily(d); print(f'      corr NBM {np.corrcoef(X,y)[0,1]:.2f} GOD {np.corrcoef(G,y)[0,1]:.2f}')
    for lv in ('pd','on','both'):
        for sf in (0.1,0.2):
            rep(lvl_break(lv,sf),f'break {lv} stop{sf}')
    rep(lvl_break('pd',0.1,need_inside_open=False),'break pd anyopen')
    rep(lvl_break('on',0.1,t0=5),'break on from 9:35')
