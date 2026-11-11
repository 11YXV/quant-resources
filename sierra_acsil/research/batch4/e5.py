from L import *
z=np.load('vp.npz'); POC=z['poc']; VAH=z['vah']; VAL=z['val']
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)&np.isfinite(POC)
D=np.nonzero(ok)[0]
def P(lbl,pts,dy,**kw):
    r=rep(pts,dy,lbl,show=False,**kw)
    if r is None or r['IS'] is None or r['OOS'] is None: print(lbl,'few'); return
    f=lambda v: f"n={v['n']:5d} pf={v['pf']:.2f} mo={v['mo']:6.0f} exp={v['exp']:6.1f} wr={v['wr']:.2f} t={v['t']:5.2f} dd={v['dd']:7.0f}"
    flag='**' if r['IS']['pf']>1.15 and r['OOS']['pf']>1.15 else '  '
    print(f"{flag}{lbl:44s} IS {f(r['IS'])} | OOS {f(r['OOS'])}")
print('== 80% rule: open outside VA, two consecutive 30m closes inside VA -> trade to other side ==')
for nclose in (1,2):
  for tgtm in ('far','poc','none'):
    for stopm in ('edge','0.15','0.3'):
        pts=[];dy=[]
        for d in D:
            o=RO[d]
            if VAL[d]<=o<=VAH[d]: continue
            side=-1 if o>VAH[d] else 1
            cnt=0
            for kk in range(930+29,k(13,0),30):
                p=CF[d,kk]
                if VAL[d]<p<VAH[d]: cnt+=1
                else: cnt=0
                if cnt>=nclose:
                    w=VAH[d]-VAL[d]
                    if stopm=='edge': st=(VAH[d]-p if side<0 else p-VAL[d])+0.1*w
                    else: st=float(stopm)*ADR[d]
                    if tgtm=='far': tg=(p-VAL[d]) if side<0 else (VAH[d]-p)
                    elif tgtm=='poc': tg=side*(POC[d]-p)
                    else: tg=None
                    if tg is not None and tg<=2: break
                    r,_=sim(d,kk,side,stop=st,tgt=tg); pts.append(r);dy.append(d); break
        P(f'80%rule n{nclose} tgt={tgtm} stop={stopm}',pts,dy)
print('== open inside VA: fade touches of VAH/VAL with limit, tgt POC, stop k*VAwidth ==')
for sm in (0.25,0.5):
  for tg in ('poc','far'):
    pts=[];dy=[]
    for d in D:
        o=RO[d]
        if not(VAL[d]<o<VAH[d]): continue
        w=VAH[d]-VAL[d]
        h=HF[d,931:k(14,0)]; l=LF[d,931:k(14,0)]
        a=np.nonzero(h>=VAH[d]+0.25)[0]; b=np.nonzero(l<=VAL[d]-0.25)[0]
        ia=a[0] if len(a) else 9999; ib=b[0] if len(b) else 9999
        if ia==ib: continue
        if ia<ib: side=-1; e=VAH[d]; kk=931+ia
        else: side=1; e=VAL[d]; kk=931+ib
        tgt=abs(POC[d]-e) if tg=='poc' else w
        if tgt<3: continue
        r,_=sim(d,kk,side,stop=sm*w,tgt=tgt,entry=e); pts.append(r);dy.append(d)
    P(f'inside-VA edge fade stop{sm}w tgt={tg}',pts,dy)
print('== open outside VA and acceptance (stays outside first 60m) -> go with (trend) ==')
pts=[];dy=[]
for d in D:
    o=RO[d]
    if VAL[d]<=o<=VAH[d]: continue
    side=1 if o>VAH[d] else -1
    seg=CF[d,930:990]
    if side>0 and LF[d,930:990].min()>VAH[d] or side<0 and HF[d,930:990].max()<VAL[d]:
        r,_=sim(d,989,side,stop=0.3*ADR[d]); pts.append(r);dy.append(d)
P('acceptance outside VA trend',pts,dy)
print('== POC magnet: open away from prior POC by x ADR (inside VA or not), limit? market at 10:00 toward POC ==')
for lo,hi in ((0.1,0.3),(0.3,0.6),(0.6,1.0)):
    pts=[];dy=[]
    for d in D:
        dist=(RO[d]-POC[d])/ADR[d]
        if not(lo<=abs(dist)<hi): continue
        side=-np.sign(dist); e=RO[d]
        r,_=sim(d,930,side,stop=0.5*abs(RO[d]-POC[d]),tgt=abs(RO[d]-POC[d]),entry=e); pts.append(r);dy.append(d)
    P(f'POC magnet from open dist {lo}-{hi}',pts,dy)
