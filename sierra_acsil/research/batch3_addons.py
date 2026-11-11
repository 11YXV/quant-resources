import warnings, pickle; warnings.filterwarnings('ignore')
src=open('/home/user/work/h1.py').read(); exec(src[:src.index("volp=pd.Series")])
OTH={k:{d['td']:d for d in pickle.load(open(f'days_{k}.pkl','rb'))} for k in ('ES','YM')}
print('--- overnight relative strength: (NQ gap - beta x ES gap)/ADR%, enter 09:35 open, hold to close')
def gaprs(k=0.05,beta=1.2,sf=0.15,oth='ES',t=5):
    tr=[]
    for i in np.where(ok)[0]:
        e=OTH[oth].get(days[i]['td'])
        if e is None or e['n']<380 or np.isnan(e['pc']): continue
        z=((DO[i]/PC[i]-1)-beta*(e['o'][0]/e['pc']-1))/(ADR5[i]/DO[i])
        if abs(z)<k: continue
        d=int(np.sign(z)); ep=O[i,t]; xm,xp=walk(i,t,d,ep-d*sf*ADR5[i],385); tr.append((i,t,xm,d,ep,xp))
    return mk(tr)
for k in (0.03,0.06,0.12):
    for b in (1.0,1.2): S(gaprs(k,b),f'gap RS k{k} beta{b}')
S(gaprs(0.06,1.2,oth='YM'),'gap RS vs YM')
print('--- pyramid on TVP / RSL winners at a later 30m mark (ahead by x ADR), own stop, exit EOD')
def pyr2(base,prof=0.15,sf=0.25,last=360):
    tr=[]
    for r in base.itertuples():
        i=r.day
        for b in MARK:
            m=b*5+5
            if m<=r.em or m>=r.xm or m>last: continue
            if r.dir*(C5[i,b]-r.ep)>=prof*ADR5[i]:
                ep=O[i,m]; xm,xp=walk(i,m,r.dir,ep-r.dir*sf*ADR5[i],min(int(r.xm),385))
                if xm>=r.xm and r.xm<385: xp=r.xp
                tr.append((i,m,xm,r.dir,ep,xp)); break
    return mk(tr)
for nm,bs in (('TVP',BASE['TVP']),('RSL',N3['RSL']),('GOD',BASE['GOD']),('ESL',N3['ESL'])):
    for pr in (0.15,0.3): S(pyr2(bs,pr),f'add to {nm} ahead {pr}')
print('--- consensus RS: NQ leads BOTH ES and YM at 10:00')
def rs2(k=0.45,sf=0.15,t=30):
    tr=[]
    for i in np.where(ok)[0]:
        e=OTH['ES'].get(days[i]['td']); y=OTH['YM'].get(days[i]['td'])
        if e is None or y is None or e['n']<380 or y['n']<380: continue
        rn=C[i,t-1]/DO[i]-1; z1=(rn-1.2*(e['c'][t-1]/e['o'][0]-1))/SIG5[i,t//5-1]; z2=(rn-1.2*(y['c'][t-1]/y['o'][0]-1))/SIG5[i,t//5-1]
        if not(abs(z1)>=k and abs(z2)>=k and np.sign(z1)==np.sign(z2)): continue
        d=int(np.sign(z1)); ep=O[i,t]; xm,xp=walk(i,t,d,ep-d*sf*ADR5[i],385); tr.append((i,t,xm,d,ep,xp))
    return mk(tr)
S(rs2(),'RS consensus ES+YM'); 
print('--- quiet open: first-30m volume < x of 14d avg, follow first-30m direction')
v30=V[:,:30].sum(1); rv=v30/pd.Series(v30).rolling(14).mean().shift(1).values
def quiet(x=0.85,sf=0.2):
    tr=[]
    for i in np.where(ok&(rv<x))[0]:
        d=int(np.sign(C[i,29]-DO[i]))
        if d: ep=O[i,30]; xm,xp=walk(i,30,d,ep-d*sf*ADR5[i],385); tr.append((i,30,xm,d,ep,xp))
    return mk(tr)
for x in (0.75,0.85,0.95): S(quiet(x),f'quiet open <{x}')
print('--- early breakout: overlap'); 
exec(src[src.index("def early("):src.index("for b0 in (1,2,3)")]); E=early(2,1.0)
g=BASE['GOD']; both=set(E.day)&set(g.day); print('early days',len(E),'also GOD days',len(both)); S(E[~E.day.isin(g.day)],'early on days GOD did not trade'); S(E[E.day.isin(g.day)],'early on GOD days')
pickle.dump(E,open('early.pkl','wb'))
