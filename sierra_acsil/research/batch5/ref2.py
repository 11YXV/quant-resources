# Reference models with the ENGINE's semantics. 5-minute decisions, 1-minute fills. Back-adjusted data.
import numpy as np, pandas as pd, os, sys
MKT=os.environ.get('MKT','NQ')
z=np.load(f'/home/user/work/{MKT}adj.npz',allow_pickle=True)
TD=pd.to_datetime(z['td']); N=len(TD)
c1=z['c'][:,930:1320]; o1=z['o'][:,930:1320]; h1=z['h'][:,930:1320]; l1=z['l'][:,930:1320]; v1=np.nan_to_num(z['v'][:,930:1320])
has=np.isfinite(c1)
PRE=np.isfinite(z['c'][:,925:930]).any(1)
TICK=0.25; MULT=2.0 if MKT=='NQ' else 5.0; COST=1.25 if MKT=='NQ' else 0.8
def halt(ts):
    y,m,d,wd=ts.year,ts.month,ts.day,(ts.weekday()+1)%7   # wd: 0=Sunday
    if wd in (0,6): return 0
    if m==1 and wd==1 and 15<=d<=21: return 1
    if m==2 and wd==1 and 15<=d<=21: return 1
    if m==5 and wd==1 and d>=25: return 1
    if m==6 and y>=2022 and (d==19 or (d==18 and wd==5) or (d==20 and wd==1)): return 1
    if m==7 and (d==4 or (d==3 and wd==5) or (d==5 and wd==1)): return 1
    if m==9 and wd==1 and d<=7: return 1
    if m==11 and wd==4 and 22<=d<=28: return 1
    if m==7 and d==3 and 1<=wd<=4: return 1
    if m==11 and wd==5 and 23<=d<=29: return 1
    if m==12 and d==24 and 1<=wd<=4: return 1
    return 0
# 5-minute bars per day (only where minutes exist)
class Day: pass
days=[]
for d in range(N):
    if has[d].sum()==0: continue
    D=Day(); D.i=d; D.td=TD[d]; D.halt=halt(TD[d])
    hb=has[d].reshape(78,5); D.bok=hb.any(1)
    O=np.full(78,np.nan);H=O.copy();Lw=O.copy();C=O.copy();V=np.zeros(78)
    for b in np.nonzero(D.bok)[0]:
        m=np.nonzero(hb[b])[0]+b*5
        O[b]=o1[d,m[0]]; C[b]=c1[d,m[-1]]; H[b]=h1[d,m].max(); Lw[b]=l1[d,m].min(); V[b]=v1[d,m].sum()
    D.O,D.H,D.L,D.C,D.V=O,H,Lw,C,V; D.seen=int(D.bok.sum())
    bs=np.nonzero(D.bok)[0]
    D.dOpen=O[bs[0]]; D.dHigh=np.nanmax(H); D.dLow=np.nanmin(Lw); D.dClose=C[bs[-1]]
    D.full=(D.seen>=76) and not D.halt
    D.validClose=D.seen*100>=78*35
    days.append(D)
BIN=float(os.environ.get('BIN','1.0'))
def value_area(D,frac):
    hist={}
    for b in np.nonzero(D.bok)[0]:
        a=int(np.floor(D.L[b]/BIN+1e-9)); e=int(np.floor(D.H[b]/BIN+1e-9)); w=max(D.V[b],1.0)/(e-a+1)
        for i in range(a,e+1): hist[i]=hist.get(i,0.0)+w
    lo=min(hist); hi=max(hist); vol=np.array([hist.get(i,0.0) for i in range(lo,hi+1)])
    p=int(vol.argmax()); tot=vol.sum(); acc=vol[p]; i=j=p; n=len(vol)
    while acc<frac*tot:
        up=vol[j+1] if j+1<n else -1.0; dn=vol[i-1] if i>0 else -1.0
        if up<0 and dn<0: break
        if up>=dn: j+=1; acc+=up
        else: i-=1; acc+=dn
    return (lo+p+0.5)*BIN,(lo+j+1)*BIN,(lo+i)*BIN
def prep(lookback=14,rsiN=2,frac=0.70):
    ring=[]; prevClose=0.0; up=dn=0.0; cnt=0; rsiPrev=0.0; prev=None
    for D in days:
        if prev is not None:
            if prev.full: ring.append(prev.dHigh-prev.dLow)
            prevClose=prev.dClose if prev.validClose else 0.0
            if prev.validClose:
                if rsiPrev>0:
                    r=prev.dClose/rsiPrev-1; u=max(r,0); w=max(-r,0)
                    if cnt==0: up,dn=u,w
                    else: up=(up*(rsiN-1)+u)/rsiN; dn=(dn*(rsiN-1)+w)/rsiN
                    cnt+=1
                rsiPrev=prev.dClose
            D.va=value_area(prev,frac) if prev.full else None
            D.nr=[ (prev.full and len(ring)>=n and ring[-1]<=min(ring[-n:])) for n in range(0,9)]
        else: D.va=None; D.nr=[False]*9
        D.adr=np.mean(ring[-lookback:]) if len(ring)>=lookback else 0.0
        D.prevClose=prevClose
        D.rsi=(100.0 if dn==0 else 100-100/(1+up/dn)) if cnt>=10 else None
        D.ok=D.adr>0 and prevClose>0 and not D.halt
        prev=D
def rnd(x): return max(np.floor(x/TICK+0.5)*TICK,2*TICK)
def trade(D,b,side,dist,eodb=77):
    """signal at close of 5-min bar b -> entry at open of next existing bar; stop-first on 1-min bars; EOD = open of the 15:55 bar"""
    nb=[x for x in np.nonzero(D.bok)[0] if x>b]
    if not nb or nb[0]>=eodb: return None
    eb=nb[0]; d=D.i; m0=eb*5
    mins=np.nonzero(has[d])[0]; mins=mins[mins>=m0]
    e=o1[d,mins[0]]; dist=rnd(dist); stop=e-side*dist
    for m in mins:
        if m>=eodb*5: return (D.i,side,eb,m,e,side*(o1[d,m]-e),'EOD')
        if side>0 and l1[d,m]<=stop: x=min(stop,o1[d,m]) if m>mins[0] else stop; return (D.i,side,eb,m,e,side*(x-e),'STOP')
        if side<0 and h1[d,m]>=stop: x=max(stop,o1[d,m]) if m>mins[0] else stop; return (D.i,side,eb,m,e,side*(x-e),'STOP')
    return (D.i,side,eb,mins[-1],e,side*(c1[d,mins[-1]]-e),'EOD')
def DWR(th=30.0,stop=0.75,short_th=None):
    """decided on the bar that ends at the open (must exist) -> entry at the open"""
    out=[]
    for D in days:
        if not (D.adr>0 and D.prevClose>0 and not D.halt) or D.rsi is None or not PRE[D.i]: continue
        if D.rsi<th: t=trade(D,-1,1,stop*D.adr)
        elif short_th is not None and D.rsi>short_th: t=trade(D,-1,-1,stop*D.adr)
        else: continue
        if t: out.append(t)
    return out
def VAR(mark=30,first=6,last=36,stop=0.20):
    """marks: bar close index (b+1) multiple of mark/5; first..last in bars-from-open of the close time (6 = 10:00, 36 = 12:30)"""
    out=[]; step=mark//5
    for D in days:
        if not D.ok or D.va is None or not D.bok[0]: continue
        poc,vah,val=D.va
        side=-1 if D.dOpen>vah else (1 if D.dOpen<val else 0)
        if side==0: continue
        for b in range(78):
            e=b+1
            if e<first or e>last or e%step or not D.bok[b]: continue
            if val<D.C[b]<vah:
                t=trade(D,b,side,stop*D.adr)
                if t: out.append(t)
                break
    return out
def CEB(nr=4,orb=6,last=42,stop=0.25,confirm=True):
    out=[]
    for D in days:
        if not D.ok or not D.nr[nr] or not D.bok[:orb].all(): continue
        orh=np.max(D.H[:orb]); orl=np.min(D.L[:orb])
        for b in range(orb,78):
            if b+1>last: break
            if not D.bok[b]: continue
            s=1 if D.C[b]>orh else (-1 if D.C[b]<orl else 0)
            if s:
                t=trade(D,b,s,stop*D.adr)
                if t: out.append(t)
                break
    return out

def EDF(mark=66,minpct=1.0,stop=0.40):
    """bar whose close is 'mark' bars after the open (66 = 15:00 on 5-min bars): |close/prior close - 1| >= minpct% -> trade that way"""
    out=[]
    for D in days:
        if not D.ok: continue
        b=mark-1
        if not D.bok[b]: continue
        mv=D.C[b]/D.prevClose-1
        if abs(mv)*100<minpct: continue
        t=trade(D,b,1 if mv>0 else -1,stop*D.adr)
        if t: out.append(t)
    return out
IS=TD<'2022-01-01'
def stats(tr,cost=None,start='2017-01-01'):
    cost=COST if cost is None else cost
    t=pd.DataFrame(tr,columns=['i','side','eb','xm','e','pts','why']); t['date']=TD[t.i.values]; t=t[t.date>=start]
    t['net']=(t.pts-cost)*MULT; return t
def line(t,lbl):
    def f(s,per):
        if len(s)==0: return 'none'
        mo=s.groupby(s.date.dt.to_period('M')).net.sum().reindex(per,fill_value=0)
        eq=s.net.cumsum(); w=s.net[s.net>0].sum(); l=-s.net[s.net<=0].sum()
        return f"n={len(s):4d} pf={w/max(l,1e-9):.2f} mo={mo.mean():5.0f} exp={s.net.mean():5.1f} wr={(s.net>0).mean():.2f} t={s.net.mean()/s.net.std()*np.sqrt(len(s)):5.2f} dd={(eq-eq.cummax()).min():6.0f}"
    pi=pd.period_range('2017-01','2021-12',freq='M'); po=pd.period_range('2022-01','2024-03',freq='M')
    print(f"{lbl:34s} IS {f(t[t.date<'2022-01-01'],pi)} | OOS {f(t[t.date>='2022-01-01'],po)}")
if __name__=='__main__':
    prep()
    print(MKT,'sessions',len(days),'full',sum(D.full for D in days))
    line(stats(DWR()),'DWR rsi2<30 stop.75 at open')
    for th in (15,20,25,35,40): line(stats(DWR(th=th)),f'DWR th{th}')
    for st in (0.5,1.0,1.5): line(stats(DWR(stop=st)),f'DWR stop{st}')
    line(stats(VAR()),'VAR 30m 10:00-12:30 stop.20')
    for mk in (15,): line(stats(VAR(mark=mk)),f'VAR mark{mk}')
    for st in (0.15,0.25,0.3): line(stats(VAR(stop=st)),f'VAR stop{st}')
    for la in (24,30,48): line(stats(VAR(last=la)),f'VAR last{la}')
    line(stats(CEB()),'CEB nr4 or30 close-confirm stop.25')
    for nr in (3,5,7): line(stats(CEB(nr=nr)),f'CEB nr{nr}')
    for ob in (3,12): line(stats(CEB(orb=ob)),f'CEB or{ob*5}')
    for st in (0.15,0.2,0.3,0.4): line(stats(CEB(stop=st)),f'CEB stop{st}')
    for la in (30,54,66): line(stats(CEB(last=la)),f'CEB last{la}')
    line(stats(CEB(nr=0)),'CEB every day (no compression)')
