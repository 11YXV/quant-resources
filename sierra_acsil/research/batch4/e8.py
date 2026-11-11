from L import *
EC=np.load('EC.npy')
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)
D=np.nonzero(ok)[0]
def P(lbl,pts,dy,**kw):
    r=rep(pts,dy,lbl,show=False,**kw)
    if r is None or r['IS'] is None or r['OOS'] is None: print(lbl,'few'); return
    f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
    flag='**' if r['IS']['pf']>1.15 and r['OOS']['pf']>1.15 else '  '
    print(f"{flag}{lbl:44s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
print('== extreme opening move reversal: first m min move > z*avg -> fade to xk ==')
for m in (5,15,30):
    mv=CF[:,930+m-1]-RO; am=pd.Series(np.abs(mv)).rolling(14).mean().shift(1).values
    for z in (2.0,3.0):
        for xk,nm in ((k(11,0),'11:00'),(1314,'close')):
            m_=D[np.abs(mv[D])>z*am[D]]
            pts=[sim(d,930+m-1,-np.sign(mv[d]),stop=abs(mv[d]),xk=xk)[0] for d in m_]
            P(f'fade first{m} z{z} x{nm}',pts,m_)
print('== late-day VWAP reversion: at t, |px-vwap|>z*ADR -> fade to 15:55 ==')
TPx=(HF[:,930:1320]+LF[:,930:1320]+CF[:,930:1320])/3; Vv=np.maximum(V[:,930:1320],1)
VW=np.cumsum(TPx*Vv,1)/np.cumsum(Vv,1)
for t,nm in ((k(14,0)-1,'14:00'),(k(15,0)-1,'15:00'),(k(15,30)-1,'15:30')):
    for lo,hi in ((0.15,0.3),(0.3,0.5),(0.5,9)):
        dv=(CF[:,t]-VW[:,t-930])/ADR
        m_=D[(np.abs(dv[D])>=lo)&(np.abs(dv[D])<hi)]
        P(f'VWAP fade {nm} dev {lo}-{hi}',-np.sign(dv[m_])*(CF[m_,1314]-CF[m_,t]),m_)
print('== day after trend day: prev range > x ADR -> fade first noise move at 10:00/10:30 ==')
pr=prevref(RNG)/ADR
for x in (1.3,1.6):
    for t in (k(10,0)-1,k(10,30)-1,k(11,0)-1):
        mv=CF[:,t]-RO
        m_=D[(pr[D]>x)&(np.abs(mv[D])>0.2*ADR[D])]
        pts=[sim(d,t,-np.sign(mv[d]),stop=0.3*ADR[d])[0] for d in m_]
        P(f'after big day>{x} fade move@{t}',pts,m_)
print('== cross-day relative strength NQ vs ES (prior n days) -> NQ o->c ==')
ERC=EC[:,1319]; epc=np.r_[np.nan,ERC[:-1]]
rn=pd.Series(np.where(np.isfinite(PC),RC/PC-1,np.nan)); rE=pd.Series(ERC/epc-1); rE[(rE.abs()>0.2)]=np.nan
for n in (1,3,5):
    rel=(rn.rolling(n).sum()-1.2*rE.rolling(n).sum()).shift(1).values
    sd=pd.Series(rel).rolling(60).std().values
    zz=rel/sd
    for z in (1.0,1.5):
        m_=D[zz[D]<-z]; P(f'NQ lagged ES {n}d z<-{z} LONG',CF[m_,1314]-RO[m_],m_)
        m_=D[zz[D]>z]; P(f'NQ led ES {n}d z>{z} LONG',CF[m_,1314]-RO[m_],m_)
print('== pre-market 08:30->09:29 move vs first hour ==')
pm=CF[:,929]-CF[:,k(8,30)-1]
for z in (0.1,0.2,0.3):
    m_=D[(np.abs(pm[D])>z*ADR[D])]
    P(f'premkt move>{z}ADR follow ->10:30',np.sign(pm[m_])*(CF[m_,k(10,30)-1]-RO[m_]),m_)
    P(f'premkt move>{z}ADR follow ->close',np.sign(pm[m_])*(CF[m_,1314]-RO[m_]),m_)
