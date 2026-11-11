from L import *
ok=FULL&np.isfinite(ADR)&np.isfinite(PC)
D=np.nonzero(ok)[0]
print('== T2 daily mean reversion, RTH only ==')
IBS=(PC-PL)/(PH-PL)
PR1=PC/prevref(PC)-1
for lo in (0.1,0.2,0.3):
    for xk,nm in ((k(10,0)-1,'10:00'),(k(12,0)-1,'12:00'),(1314,'15:55')):
        m=D[IBS[D]<lo]; rep(CF[m,xk]-RO[m],m,f'IBS<{lo} long open->{nm}')
for hi in (0.8,0.9):
    m=D[IBS[D]>hi]; rep(-(CF[m,1314]-RO[m]),m,f'IBS>{hi} short open->15:55')
# down streak
dn=(PR1<0); dn2=dn&np.roll(dn,1); dn3=dn2&np.roll(dn,2)
for nm,mm in (('1 down day',dn),('2 down',dn2),('3 down',dn3)):
    m=D[mm[D]]; rep(CF[m,1314]-RO[m],m,f'{nm} long open->close')
up=(PR1>0); up3=up&np.roll(up,1)&np.roll(up,2)
m=D[up3[D]]; rep(-(CF[m,1314]-RO[m]),m,'3 up short open->close')
# big down day prev (ret< -1.5*avg)
for th in (-0.01,-0.015,-0.02):
    m=D[PR1[D]<th]; rep(CF[m,1314]-RO[m],m,f'prev cc<{th} long o->c'); rep(CF[m,k(10,30)-1]-RO[m],m,f'prev cc<{th} long o->10:30')
print('== T3 Baltussen ROD -> last 30 ==')
for ek,nm in ((k(15,30)-1,'15:30'),(k(15,0)-1,'15:00')):
    rod=CF[D,ek]/PC[D]-1; rodo=CF[D,ek]/RO[D]-1
    for th in (0,0.5,1.0,1.5):
        sg=ADR[D]/PC[D]
        m=np.abs(rod)>th*sg; s=np.sign(rod[m]); dd=D[m]
        rep(s*(CF[dd,1314]-CF[dd,ek]),dd,f'ROD(pc) {nm} |r|>{th}adr')
        m=np.abs(rodo)>th*sg; s=np.sign(rodo[m]); dd=D[m]
        rep(s*(CF[dd,1314]-CF[dd,ek]),dd,f'ROD(open) {nm} |r|>{th}adr')
print('== T4 compression ==')
r=pd.Series(RNG)
nr7=(r.shift(1)==r.shift(1).rolling(7).min()).values
inside=(PH<prevref(PH))&(PL>prevref(PL))
lowv=(prevref(RNG)<0.6*ADR)
for nm,mm in (('NR7',nr7),('inside',inside),('prevrng<0.6ADR',lowv),('all',np.ones(N,bool))):
    for orm in (15,30):
        pts=[];dy=[]
        for d in D[mm[D]]:
            oh=HF[d,930:930+orm].max(); ol=LF[d,930:930+orm].min()
            h=HF[d,930+orm:1315]; l=LF[d,930+orm:1315]
            a=np.nonzero(h>oh)[0]; b=np.nonzero(l<ol)[0]
            ia=a[0] if len(a) else 9999; ib=b[0] if len(b) else 9999
            if ia==9999 and ib==9999: continue
            if ia<ib: p,_=sim(d,930+orm+ia,1,stop=oh-ol+0.25,entry=oh+0.25)
            elif ib<ia: p,_=sim(d,930+orm+ib,-1,stop=oh-ol+0.25,entry=ol-0.25)
            else: continue
            pts.append(p);dy.append(d)
        rep(pts,dy,f'{nm} ORB{orm} stop=other side')
