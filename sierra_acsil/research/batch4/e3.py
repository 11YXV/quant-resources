from L import *
import sys
# Overnight session (Globex) mean reversion. ref = 18:00 open. sigma_k=14d mean |c_k/o18-1|
ok=GOODON&np.isfinite(ADR)
O18=OF[:,0]
MOVE=np.abs(CF/O18[:,None]-1)
MOVE[~GOODON]=np.nan
SIG=pd.DataFrame(MOVE).rolling(14,min_periods=10).mean().shift(1).values
D=np.nonzero(ok&np.isfinite(SIG[:,100]))[0]
# running overnight VWAP
TPx=(HF+LF+CF)/3; Vv=np.maximum(V,1)
VW=np.cumsum(TPx*Vv,1)/np.cumsum(Vv,1)
def run(band,step,k0,k1,xk,stopm,exitmode,maxtr=1,cost=COST,side_f=-1,label=None,show=True):
    pts=[];dy=[]
    for d in D:
        ntr=0; kk=k0
        while kk<=k1 and ntr<maxtr:
            p=CF[d,kk]; ub=O18[d]*(1+band*SIG[d,kk]); lb=O18[d]*(1-band*SIG[d,kk])
            s=0
            if p>ub: s=side_f
            elif p<lb: s=-side_f
            if s==0: kk+=step; continue
            # manage: check at each step mark; stop on minute bars
            e=p; stop=stopm*ADR[d]; res=None; j=kk
            while True:
                j2=min(j+step,xk)
                h=HF[d,j+1:j2+1].max(); l=LF[d,j+1:j2+1].min()
                if s>0 and l<=e-stop: res=-stop; break
                if s<0 and h>=e+stop: res=-stop; break
                j=j2; q=CF[d,j]
                if j>=xk: res=s*(q-e); break
                if exitmode=='ref' and s*(q-O18[d])*side_f<=0 and side_f<0: res=s*(q-e); break
                if exitmode=='vwap' and ((s>0 and q>=VW[d,j]) or (s<0 and q<=VW[d,j])): res=s*(q-e); break
                if exitmode=='half':
                    mid=O18[d]*(1+ (0.3*SIG[d,j] if e>O18[d] else -0.3*SIG[d,j]))
                    if (s>0 and q>=mid) or (s<0 and q<=mid): res=s*(q-e); break
            pts.append(res);dy.append(d);ntr+=1; kk=j+step
    return rep(pts,dy,label or f'b{band} st{step} {k0}-{k1} x{xk} stop{stopm} {exitmode} f{side_f}',cost=cost,show=show)
print('--- fade, whole night 18:30-08:00, exit by 09:25')
for band in (1.0,1.5,2.0,2.5):
    for ex in ('ref','vwap','half'):
        for st in (0.15,0.3,0.6):
            run(band,30,30,k(8,0),k(9,25),st,ex)
print('--- momentum for reference')
run(1.0,30,30,k(8,0),k(9,25),0.3,'none',side_f=1)
print('--- by window (band1.5, stop .3, vwap)')
for a,b,x,nm in ((30,k(0,0),k(2,0),'asia 18:30-00:00 x02:00'),(k(2,0),k(7,0),k(9,25),'europe 02-07 x09:25'),(k(3,0),k(8,0),k(9,25),'europe 03-08'),(30,k(0,0),k(9,25),'asia entry x09:25')):
    run(1.5,30,a,b,x,0.3,'vwap',label=nm); run(2.0,30,a,b,x,0.3,'vwap',label=nm+' b2')
