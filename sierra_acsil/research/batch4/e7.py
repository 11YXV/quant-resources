from L import *
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)
D=np.nonzero(ok)[0]
def P(lbl,pts,dy,**kw):
    r=rep(pts,dy,lbl,show=False,**kw)
    if r is None or r['IS'] is None or r['OOS'] is None: print(lbl,'few'); return
    f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
    flag='**' if r['IS']['pf']>1.15 and r['OOS']['pf']>1.15 else '  '
    print(f"{flag}{lbl:44s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
print('== IB (first 60m) fade on wide-IB days: limit at IB extreme, tgt IB mid, stop k*IBw ==')
for lo,hi in ((0,0.4),(0.4,0.6),(0.6,0.8),(0.8,9)):
    for sm in (0.25,0.5):
        for tg in (0.5,1.0):
            pts=[];dy=[]
            for d in D:
                ih=HF[d,930:990].max(); il=LF[d,930:990].min(); w=ih-il
                if not(lo<=w/ADR[d]<hi): continue
                h=HF[d,990:k(15,0)]; l=LF[d,990:k(15,0)]
                a=np.nonzero(h>=ih+0.25)[0]; b=np.nonzero(l<=il-0.25)[0]
                ia=a[0] if len(a) else 9999; ib=b[0] if len(b) else 9999
                if ia==ib: continue
                if ia<ib: r,_=sim(d,990+ia,-1,stop=sm*w,tgt=tg*w,entry=ih)
                else: r,_=sim(d,990+ib,1,stop=sm*w,tgt=tg*w,entry=il)
                pts.append(r);dy.append(d)
            P(f'IB fade w/ADR {lo}-{hi} stop{sm} tgt{tg}',pts,dy)
print('== daily conditions -> long/short open->15:55 (baseline all days long: IS pf1.08 OOS 1.05) ==')
cl=pd.Series(np.where(FULL,RC,np.nan)).ffill()
# NOTE roll gaps: use back-adjust-free daily returns from same-contract PC
r1=pd.Series(np.where(np.isfinite(PC),RC/PC-1,0.0))
idx=(1+r1).cumprod()            # synthetic continuous daily close
ip=idx.shift(1)                 # known before today's open
def rsi(s,n):
    dlt=s.diff(); up=dlt.clip(lower=0).ewm(alpha=1/n,adjust=False).mean(); dn=(-dlt.clip(upper=0)).ewm(alpha=1/n,adjust=False).mean()
    return 100-100/(1+up/dn)
RS2=rsi(idx,2).shift(1).values; RS3=rsi(idx,3).shift(1).values
ma=lambda n: (ip>ip.rolling(n).mean()).values
ret=lambda n: (ip/ip.shift(n)-1).values
vol=r1.rolling(20).std().shift(1).values
oc=CF[:,1314]-RO
conds={
 'RSI2<10':RS2<10,'RSI2<5':RS2<5,'RSI2<20':RS2<20,'RSI2<10 & >MA200':(RS2<10)&ma(200),'RSI2<10 & <MA200':(RS2<10)&~ma(200),
 'RSI3<15':RS3<15,'5d ret<-2vol':ret(5)<-2*vol*np.sqrt(5)/1,'5d ret<-1vol':ret(5)<-1*vol*np.sqrt(5),'3d ret<-1.5vol':ret(3)<-1.5*vol*np.sqrt(3),
 '>MA50':ma(50),'<MA50':~ma(50),'>MA200':ma(200),'<MA200':~ma(200),
 'new 20d high close':(ip>=ip.rolling(20).max()).values,'new 10d low close':(ip<=ip.rolling(10).min()).values,'new 5d low close':(ip<=ip.rolling(5).min()).values,
}
for nm,mm in conds.items():
    m_=D[np.nan_to_num(mm[D]).astype(bool)]; P(nm+' LONG o->c',oc[m_],m_)
print('-- short mirrors')
sc={'RSI2>90':RS2>90,'RSI2>95':RS2>95,'RSI2>90 & <MA200':(RS2>90)&~ma(200),'new 10d high close':(ip>=ip.rolling(10).max()).values,'5d ret>2vol':ret(5)>2*vol*np.sqrt(5)}
for nm,mm in sc.items():
    m_=D[np.nan_to_num(mm[D]).astype(bool)]; P(nm+' SHORT o->c',-oc[m_],m_)
np.save('RS2.npy',RS2); np.save('idxprev.npy',ip.values)
