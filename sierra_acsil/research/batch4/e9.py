from L import *
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)&(nb[:,k(8,0):930].sum(1)>60)
D=np.nonzero(ok)[0]
def P(lbl,pts,dy,**kw):
    r=rep(pts,dy,lbl,show=False,**kw)
    if r is None or r['IS'] is None or r['OOS'] is None: print(lbl,'few'); return
    f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
    flag='**' if r['IS']['pf']>1.15 and r['OOS']['pf']>1.15 else '  '
    print(f"{flag}{lbl:44s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
print('== fade pre-market impulse at RTH open ==')
for s0,nm0 in ((k(8,30)-1,'0830'),(k(8,0)-1,'0800'),(k(7,0)-1,'0700'),(k(9,0)-1,'0900'),(k(4,0)-1,'0400')):
    pm=CF[:,929]-CF[:,s0]
    for z in (0.15,0.2,0.25,0.3):
        m_=D[(np.abs(pm[D])>z*ADR[D])]
        for xk,nx in ((k(10,0)-1,'1000'),(k(10,30)-1,'1030'),(k(11,30)-1,'1130'),(1314,'cls')):
            P(f'fade {nm0}->open >{z} x{nx}',-np.sign(pm[m_])*(CF[m_,xk]-RO[m_]),m_)
