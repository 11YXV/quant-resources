from L import *
E=load('ES'); etd=E['td']
ei=pd.Series(np.arange(len(etd)),index=etd).reindex(TD).values
has=np.isfinite(ei); ei2=np.where(has,ei,0).astype(int)
EC=pd.DataFrame(E['c']).ffill(axis=1).values[ei2]; EC[~has]=np.nan
EO=np.where(np.isfinite(E['o'][ei2]),E['o'][ei2],EC); EO[~has]=np.nan
ES_SYMOK=has
np.save('EC.npy',EC)
ok=FULL&np.isfinite(ADR)&has&(np.isfinite(EC[:,930:1320]).sum(1)>=380)
D=np.nonzero(ok)[0]
# intraday log-return from RTH open for both
rn=np.log(CF[:,930:1320]/RO[:,None]); re=np.log(EC[:,930:1320]/EO[:,930][:,None])
for beta in (1.0,1.2):
    sp=rn-beta*re      # spread path
    sd=pd.DataFrame(np.abs(sp)).rolling(14,min_periods=10).mean().shift(1).values  # mean abs spread by minute
    print(f'beta {beta}: mean |spread| at 10:00 {np.nanmean(np.abs(sp[:,29]))*1e4:.1f}bp, close {np.nanmean(np.abs(sp[:,-6]))*1e4:.1f}bp')
    # costs in return units per round trip
    def run(mode,z,t0,t1,step=30,xk=384,lbl=''):
        res=[];dy=[]
        for d in D:
            if not np.isfinite(sd[d,60]): continue
            for t in range(t0,t1+1,step):
                x=sp[d,t]/sd[d,t]
                if abs(x)>z:
                    s=np.sign(x)*(1 if mode=='mom' else -1)
                    e=sp[d,t]
                    if mode=='rev':
                        # exit when spread crosses back half way or at xk; check at step marks
                        out=sp[d,xk]
                        for u in range(t+step,xk,step):
                            if abs(sp[d,u])<0.3*sd[d,u] or np.sign(sp[d,u])!=np.sign(e): out=sp[d,u]; break
                    else: out=sp[d,xk]
                    gross=s*(out-e)
                    cost=2.5/(2*RO[d])+beta*(RO[d]*2/(EO[d,930]*5))*4.0/(2*RO[d])   # in return units of NQ notional
                    res.append((gross-cost)*RO[d]); dy.append(d)   # points of NQ per 1 MNQ => *2 = $ ; rep multiplies by 2 with cost arg
                    break
        r=rep(res,dy,lbl,cost=0,show=False)
        f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
        print(f"{lbl:40s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
    for mode in ('mom','rev'):
        for z in (1.0,1.5,2.0):
            for t0,t1,nm in ((29,29,'10:00 only'),(59,269,'10:30-14:00 first'),(149,299,'12:00-14:30')):
                run(mode,z,t0,t1,lbl=f'b{beta} {mode} z{z} {nm}')
