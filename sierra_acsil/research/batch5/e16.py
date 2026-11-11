from S5 import *
NQ=NQd.copy()
NQ['pc']=NQ.c.shift(1); NQ['ppc']=NQ.c.shift(2); NQ['po']=NQ.o.shift(1)
gapd=(NQ.index.to_series().diff().dt.days<=4)
NQ['yday']=np.where(gapd,NQ.pc/NQ.po-1,np.nan)          # yesterday's open->close
NQ['night']=np.where(gapd,NQ.o/NQ.pc-1,np.nan)         # last night's close->open
NQ['mon']=NQ.index.weekday==0
print('== Yu-Rentzler-Wolf: signs of yesterday (open-close) and last night -> today open->15:55 ==')
for mon in (None,True,False):
  for ys in (1,-1):
    for ns in (1,-1):
        for side in (1,-1):
            m=(np.sign(NQ.yday)==ys)&(np.sign(NQ.night)==ns)
            if mon is not None: m&=(NQ.mon==mon)
            tr=[trade_m(DAYS[td],0,side,0.75*DAYS[td].adr) for td in NQ.index[m.values] if td in DAYS and ok(DAYS[td])]
            t=r.stats([x for x in tr if x])
            if len(t)<20: continue
            pf=lambda s: s.net[s.net>0].sum()/max(1e-9,-s.net[s.net<=0].sum())
            a=t[t.date<'2022-01-01']; b=t[t.date>='2022-01-01']
            if side==1 or True:
                print(f"mon={str(mon):5s} yday {'+' if ys>0 else '-'} night {'+' if ns>0 else '-'} {'LONG ' if side>0 else 'SHORT'} IS n={len(a):3d} pf={pf(a):.2f} ${a.net.sum()/60:5.0f}/mo | OOS n={len(b):3d} pf={pf(b):.2f} ${b.net.sum()/26.2:5.0f}/mo")
print('== FOMC day: long from open to 13:55 (announcement 14:00) ==')
F=['2017-02-01','2017-03-15','2017-05-03','2017-06-14','2017-07-26','2017-09-20','2017-11-01','2017-12-13','2018-01-31','2018-03-21','2018-05-02','2018-06-13','2018-08-01','2018-09-26','2018-11-08','2018-12-19',
'2019-01-30','2019-03-20','2019-05-01','2019-06-19','2019-07-31','2019-09-18','2019-10-30','2019-12-11','2020-01-29','2020-04-29','2020-06-10','2020-07-29','2020-09-16','2020-11-05','2020-12-16',
'2021-01-27','2021-03-17','2021-04-28','2021-06-16','2021-07-28','2021-09-22','2021-11-03','2021-12-15','2022-01-26','2022-03-16','2022-05-04','2022-06-15','2022-07-27','2022-09-21','2022-11-02','2022-12-14',
'2023-02-01','2023-03-22','2023-05-03','2023-06-14','2023-07-26','2023-09-20','2023-11-01','2023-12-13','2024-01-31']
F=pd.to_datetime(F)
for x,nm in ((265,'13:55'),(385,'15:55')):
    for st in (None,0.5):
        tr=[trade_m(DAYS[td],0,1,st*DAYS[td].adr if st else None,xm=x) for td in F if td in DAYS and ok(DAYS[td])]
        L(tr,f'FOMC long open->{nm} stop{st}')
prev=[NQ.index[NQ.index.get_loc(td)-1] for td in F if td in NQ.index]
tr=[trade_m(DAYS[td],0,1,0.5*DAYS[td].adr) for td in prev if td in DAYS and ok(DAYS[td])]; L(tr,'day before FOMC long o->c')
print('== same-time-of-day persistence (Heston-Korajczyk-Sadka): 30-min slot, sign of the same slot over last N days ==')
ret30=np.full((r.N,13),np.nan)
for D in r.days:
    d=D.i
    for j in range(13):
        a=j*30; b=a+29
        if r.has[d,a] and r.has[d,b]: ret30[D.i,j]=r.c1[d,b]-r.o1[d,a]
R30=pd.DataFrame(ret30)
for N in (5,10,20):
    mean=R30.rolling(N,min_periods=N-1).mean().shift(1).values
    sd=R30.rolling(60,min_periods=40).std().shift(1).values
    for z in (0.5,0.8):
        tr=[]
        for D in r.days:
            if not ok(D): continue
            for j in range(12):           # last slot 15:30-15:55 skipped
                m=mean[D.i,j]; s=sd[D.i,j]
                if not np.isfinite(m) or not np.isfinite(s) or abs(m)/(s/np.sqrt(N))<z: continue
                tr.append(trade_m(D,j*30,int(np.sign(m)),None,xm=j*30+30))
        L(tr,f'slot persistence N{N} z{z} (30-min holds)')
print('== IB compression: first-hour range small vs its 14-day average -> close break of IB, hold ==')
ib=np.full(r.N,np.nan)
for D in r.days:
    if D.bok[:12].all(): ib[D.i]=np.max(D.H[:12])-np.min(D.L[:12])
aib=pd.Series(ib).rolling(14,min_periods=10).mean().shift(1).values
for th in (0.6,0.75,1.0,99):
  for st in (0.25,'ib'):
    tr=[]
    for D in r.days:
        if not ok(D) or not np.isfinite(ib[D.i]) or ib[D.i]>th*aib[D.i]: continue
        hi=np.max(D.H[:12]); lo=np.min(D.L[:12])
        for b in range(12,60):
            if not D.bok[b]: continue
            s=1 if D.C[b]>hi else (-1 if D.C[b]<lo else 0)
            if s:
                dist=st*D.adr if st!='ib' else (hi-lo)
                tr.append(r.trade(D,b,s,dist)); break
    L(tr,f'IB<{th}x avg close-break stop {st}')
