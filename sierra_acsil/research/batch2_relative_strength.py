import warnings, os, pickle; warnings.filterwarnings('ignore')
from ref import *
exec(open('g1.py').read().split("ok=np.array")[0].split("from ref import *")[1])
ok=np.array([valid(i) for i in range(N)])
def bands(i,vm=1.0): return max(DO[i],PC[i])*(1+vm*SIG5[i]), min(DO[i],PC[i])*(1-vm*SIG5[i])
OTH={k:{d['td']:d for d in pickle.load(open(f'days_{k}.pkl','rb'))} for k in ('ES','YM')}
def rs(t=30,k=0.3,sf=0.2,beta=1.2,oth='ES',flat=385,need=None,recheck=None):
    tr=[]
    for i in np.where(ok)[0]:
        e=OTH[oth].get(days[i]['td'])
        if e is None or e['n']<380: continue
        rn=C[i,t-1]/DO[i]-1; re=e['c'][t-1]/e['o'][0]-1; z=(rn-beta*re)/SIG5[i,t//5-1]
        if abs(z)<k: continue
        d=int(np.sign(z))
        if need=='same' and np.sign(rn)!=d: continue
        if need=='opp' and np.sign(rn)==d: continue
        ep=O[i,t]; stop=ep-d*sf*ADR5[i]; end=flat
        if recheck:
            for m in range(t+recheck,flat,recheck):
                z2=((C[i,m-1]/DO[i]-1)-beta*(e['c'][m-1]/e['o'][0]-1))
                if np.sign(z2)!=d: end=m; break
        xm,xp=walk(i,t,d,stop,end); tr.append((i,t,xm,d,ep,xp))
    return mk(tr)
print('--- ES leads: ES beyond its own band at a mark, NQ not yet -> enter NQ')
ES=pickle.load(open('days_ES.pkl','rb')); 
def eslead(sf=0.2,last_b=65):
    # ES noise profile
    eo=np.array([d['o'] for d in ES]); ec=np.array([d['c'] for d in ES]); etd=[d['td'] for d in ES]; ef=np.array([not d['short'] for d in ES]); epc=np.array([d['pc'] for d in ES])
    mv=np.abs(ec/eo[:,:1]-1); idx=np.where(ef)[0]; sig={}
    for j in range(len(ES)):
        p=idx[idx<j][-14:]
        if len(p)==14: sig[etd[j]]=(mv[p].mean(0),j)
    tr=[]
    for i in np.where(ok)[0]:
        t=days[i]['td']
        if t not in sig or not ef[sig[t][1]] or np.isnan(epc[sig[t][1]]): continue
        sg,j=sig[t]; UB,LB=bands(i)
        for b in MARK:
            if b>last_b: break
            m=b*5+5; ce=ec[j,m-1]; eu=max(eo[j,0],epc[j])*(1+sg[m-1]); el=min(eo[j,0],epc[j])*(1-sg[m-1]); c=C5[i,b]
            d=1 if (ce>eu and c<=UB[b] and c>VW5[i,b]) else -1 if (ce<el and c>=LB[b] and c<VW5[i,b]) else 0
            if not d: continue
            ep=O[i,m]; xm,xp=walk(i,m,d,ep-d*sf*ADR5[i],385); tr.append((i,m,xm,d,ep,xp)); break
    return mk(tr)
for sf in (0.15,0.25): S(eslead(sf),f'ES leads, stop {sf}')
for t in (120,180,210,270): S(rs(t=t,k=0.45,sf=0.15),f'RS entry at minute {t}')
print('--- NBM trades split by ES state at entry (ES beyond its own band same side?)')
nb=BASE['NBM']; eo=np.array([d['o'] for d in ES]); ec=np.array([d['c'] for d in ES]); etd={d['td']:k for k,d in enumerate(ES)}; ef=np.array([not d['short'] for d in ES]); epc=np.array([d['pc'] for d in ES])
mv=np.abs(ec/eo[:,:1]-1); idx=np.where(ef)[0]; conf=[]
for r in nb.itertuples():
    j=etd.get(days[r.day]['td']); 
    if j is None: conf.append(np.nan); continue
    p=idx[idx<j][-14:]
    if len(p)<14 or np.isnan(epc[j]): conf.append(np.nan); continue
    sg=mv[p].mean(0)[r.em-1]; ce=ec[j,r.em-1]
    conf.append(1.0 if (r.dir>0 and ce>max(eo[j,0],epc[j])*(1+sg)) or (r.dir<0 and ce<min(eo[j,0],epc[j])*(1-sg)) else 0.0)
nb=nb.assign(conf=conf); S(nb[nb.conf==1],'NBM, ES confirms'); S(nb[nb.conf==0],'NBM, ES does not confirm')
print('--- Monday long open->close by year'); 
oc=(O[:,385]-O[:,1]); m=(TD.weekday==0)&ok; x=pd.Series((oc[m]-COST)*2,index=TD[m]); print(x.groupby(x.index.year).sum().round(0).astype(int).to_dict(),'t=',round(x.mean()/x.std()*len(x)**.5,2))
