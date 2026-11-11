from L import *
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)
D=np.nonzero(ok)[0]
def P(lbl,pts,dy,**kw):
    r=rep(pts,dy,lbl,show=False,**kw)
    if r is None or r['IS'] is None or r['OOS'] is None: print(lbl,'few'); return
    f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:5.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
    flag='**' if r['IS']['pf']>1.15 and r['OOS']['pf']>1.15 else '  '
    print(f"{flag}{lbl:40s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
# 5-min bars RTH
c5=CF[:,930:1320].reshape(N,78,5)[:,:,-1]; o5=OF[:,930:1320].reshape(N,78,5)[:,:,0]
h5=HF[:,930:1320].reshape(N,78,5).max(2); l5=LF[:,930:1320].reshape(N,78,5).min(2); v5=V[:,930:1320].reshape(N,78,5).sum(2)
v5[~FULL]=np.nan
av=pd.DataFrame(v5).rolling(14,min_periods=10).mean().shift(1).values
rg=h5-l5; rg[~FULL]=np.nan; ar=pd.DataFrame(rg).rolling(14,min_periods=10).mean().shift(1).values
print('== m: volume climax reversal (5m bar vol>zv*avg(tod), range>zr*avg, close in opposite half) ==')
for zv in (2.0,3.0):
  for zr in (1.5,2.5):
    for hold in (6,12,999):
      for mode in ('rev','cont'):
        pts=[];dy=[]
        for d in D:
            if not np.isfinite(av[d,5]): continue
            for b in range(3,72):
                if v5[d,b]>zv*av[d,b] and rg[d,b]>zr*ar[d,b]:
                    pos=(c5[d,b]-l5[d,b])/max(rg[d,b],0.25); dn=c5[d,b]<o5[d,b]
                    if mode=='rev':
                        if dn and pos>0.5: s=1
                        elif (not dn) and pos<0.5: s=-1
                        else: continue
                        st=(c5[d,b]-l5[d,b]+1) if s>0 else (h5[d,b]-c5[d,b]+1)
                    else:
                        if dn and pos<0.25: s=-1
                        elif (not dn) and pos>0.75: s=1
                        else: continue
                        st=rg[d,b]
                    ek=930+b*5+4; xk=min(1314,ek+hold*5)
                    r,_=sim(d,ek,s,stop=st,xk=xk); pts.append(r);dy.append(d); break
        P(f'climax {mode} zv{zv} zr{zr} hold{hold}',pts,dy)
print('== n: intraday sweep of session low/high (>=60m old) and reclaim on 5m close ==')
for age in (12,24):
  for tg in (None,2.0):
    pts=[];dy=[]
    for d in D:
        for b in range(age+1,72):
            ll=l5[d,:b].min(); hh=h5[d,:b].max()
            il=l5[d,:b].argmin(); ih=h5[d,:b].argmax()
            s=0
            if l5[d,b]<ll and c5[d,b]>ll and b-il>=age: s=1; st=c5[d,b]-l5[d,b]+1
            elif h5[d,b]>hh and c5[d,b]<hh and b-ih>=age: s=-1; st=h5[d,b]-c5[d,b]+1
            if s:
                r,_=sim(d,930+b*5+4,s,stop=st,tgt=tg*st if tg else None); pts.append(r);dy.append(d); break
    P(f'sweep reclaim age{age} tgt{tg}',pts,dy)
print('== q: tug of war: sum of last n RTH o->c returns / overnight returns -> today o->c ==')
oc=pd.Series(np.where(FULL,RC/RO-1,np.nan)); on=pd.Series(RO/PC-1)
for n in (5,10,20,40):
    a=oc.rolling(n,min_periods=n-2).sum().shift(1).values; b=on.rolling(n,min_periods=n-2).sum().shift(1).values
    for nm,x in (('intraday',a),('overnight',b),('intra-minus-on',a-b)):
        m_=D[np.isfinite(x[D])]; s=np.sign(x[m_]); P(f'{nm} {n}d sign follow',s*(CF[m_,1314]-RO[m_]),m_)
print('== j: monthly opex Friday / week: noise fade ==')
wd=TD.weekday; dom=TD.day; opex=(wd==4)&(dom>=15)&(dom<=21)
m_=D[opex[D]]; mv=CF[m_,k(10,30)-1]-RO[m_]; P('opex fri fade 10:30 move ->close',-np.sign(mv)*(CF[m_,1314]-CF[m_,k(10,30)-1]),m_)
