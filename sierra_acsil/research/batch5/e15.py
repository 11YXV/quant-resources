from S5 import *
NQ=NQd.copy()
NQ['ret']=NQ.c/NQ.c.shift(1)-1
mo=NQ.index.to_period('M')
NQ['last']=np.r_[mo[1:]!=mo[:-1],False]; NQ['last2']=NQ['last']|np.r_[NQ['last'].values[1:],False]
NQ['first']=np.r_[True,mo[1:]!=mo[:-1]]
# month-to-date return up to yesterday's close (from the last close of the prior month)
prevmo_close=NQ.c.where(NQ['last']).ffill().shift(1)
NQ['mtd']=(NQ.c.shift(1)/prevmo_close.where(~NQ['first'],np.nan)-1)
NQ.loc[NQ['first'],'mtd']=np.nan
vol=NQ.ret.rolling(60).std()
NQ['mtdz']=NQ.mtd/(vol*np.sqrt(15))
print('== A month-end rebalancing: last 1/2 days, fade month-to-date return ==')
for which in ('last','last2'):
  for z in (0,0.5,1.0):
    for ent,nm in ((0,'open'),(270,'14:00'),(330,'15:00')):
        tr=[]
        for td,row in NQ[NQ[which]].iterrows():
            D=DAYS.get(td)
            if D is None or not ok(D) or not np.isfinite(row.mtdz) or abs(row.mtdz)<z: continue
            s=-int(np.sign(row.mtdz)); tr.append(trade_m(D,ent,s,0.5*D.adr))
        L(tr,f'ME {which} |z|>{z} entry {nm}')
print('== B pre-holiday long ==')
hol=[r.halt(t) for t in NQ.index]
nxt=pd.Series(hol,index=NQ.index).shift(-1).fillna(0).astype(bool)
# also day before 3-day weekend where Monday is a full holiday (no session in data): gap in dates
gapnext=pd.Series(NQ.index,index=NQ.index).shift(-1)
long3=((gapnext-NQ.index).dt.days>=4)
for nm,m in (('before CME short session',nxt),('before a >=4-day gap (holiday wknd)',long3),('either',nxt|long3)):
    tr=[trade_m(DAYS[td],0,1,0.75*DAYS[td].adr) for td in NQ.index[m.values] if td in DAYS and ok(DAYS[td])]
    L(tr,f'pre-holiday {nm}')
print('== C cross-day NQ vs ES / YM relative reversal (unhedged MNQ, open->15:55) ==')
for ref,R_ in (('ES',ESd),('YM',YMd)):
    j=NQ[['o','c']].join(R_[['o','c']],rsuffix='_r',how='inner')
    for kind in ('oc','cc'):
        if kind=='oc': rn=j.c/j.o-1; rr=j.c_r/j.o_r-1
        else: rn=j.c/j.c.shift(1)-1; rr=j.c_r/j.c_r.shift(1)-1
        b=(rn.rolling(60).cov(rr)/rr.rolling(60).var()).shift(1)
        for n in (1,3):
            rel=(rn-b*rr).rolling(n).sum(); sd=rel.rolling(60).std()
            zz=(rel/sd).shift(1)
            for z in (1.0,1.5):
                tr=[]
                for td,x in zz.items():
                    D=DAYS.get(td)
                    if D is None or not ok(D) or not np.isfinite(x) or abs(x)<z: continue
                    tr.append(trade_m(D,0,-int(np.sign(x)),0.75*D.adr))
                L(tr,f'rel-rev vs {ref} {kind} n{n} z{z}')
print('== F weekly range position (prior 5 sessions) ==')
hh=NQ.h.rolling(5).max(); ll=NQ.l.rolling(5).min(); pos=((NQ.c-ll)/(hh-ll)).shift(1)
for lo in (0.1,0.2):
    tr=[trade_m(DAYS[td],0,1,0.75*DAYS[td].adr) for td,x in pos.items() if x<lo and td in DAYS and ok(DAYS[td])]; L(tr,f'5d pos<{lo} long')
    tr=[trade_m(DAYS[td],0,-1,0.75*DAYS[td].adr) for td,x in pos.items() if x>1-lo and td in DAYS and ok(DAYS[td])]; L(tr,f'5d pos>{1-lo} short')
print('== L OOPS (Williams): open beyond prior day extreme, stop entry back at it, exit 15:55 ==')
for need in (0.0,0.1):
  for st in (0.25,0.5):
    tr=[]
    for D in r.days:
        if not ok(D): continue
        prev=NQ.index.get_loc(D.td) if D.td in NQ.index else None
        if not prev: continue
        p=NQ.iloc[prev-1]
        d=D.i; m=np.nonzero(r.has[d])[0]
        if D.dOpen<p.l-need*D.adr:
            trig=p.l; hit=np.nonzero(r.h1[d,m]>=trig)[0]
            hit=[q for q in m[hit] if q<240]
            if hit: tr.append(trade_m(D,hit[0],1,st*D.adr) and (lambda t:(t[0],1,t[2],t[3],trig,t[5]-(trig-t[4]) if False else 1*(r.o1[d,t[3]]-trig) if t[6]=='EOD' else -st*D.adr if t[6]=='STOP' else t[5],t[6]))(trade_m(D,hit[0],1,st*D.adr)))
        elif D.dOpen>p.h+need*D.adr:
            trig=p.h; hit=np.nonzero(r.l1[d,m]<=trig)[0]
            hit=[q for q in m[hit] if q<240]
            if hit: tr.append((lambda t:(t[0],-1,t[2],t[3],trig,-1*(r.o1[d,t[3]]-trig) if t[6]=='EOD' else -st*D.adr if t[6]=='STOP' else t[5],t[6]))(trade_m(D,hit[0],-1,st*D.adr)))
    L(tr,f'OOPS gap>{need}adr stop{st} (approx fills)')
