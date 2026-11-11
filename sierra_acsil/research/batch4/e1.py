from L import *
# 1. mean return (bp) per 30-min slot over 23h, IS/OOS, t-stats
ok=FULL&GOODON
ret=[]
print('slot   IS_bp  t   OOS_bp  t   | after down day (prev RTH ret<-1%): IS OOS | after up>1%')
PR=prevref(RC/RO-1)  # prev day RTH o->c return
PCC=prevref(RC)/prevref(prevref(RC))-1
for s in range(0,1380,30):
    a=CF[:,s-1] if s>0 else OF[:,0]; b=CF[:,s+29]
    r=(b/a-1)*1e4
    def ms(m):
        x=r[m&np.isfinite(r)]; return x.mean(), x.mean()/x.std()*np.sqrt(len(x)) if len(x)>5 else 0
    hh=(18*60+s)//60%24; mm=s%60
    i=ms(ok&IS); o=ms(ok&OOS); di=ms(ok&IS&(PR<-0.01)); do=ms(ok&OOS&(PR<-0.01)); ui=ms(ok&IS&(PR>0.01)); uo=ms(ok&OOS&(PR>0.01))
    print(f'{hh:02d}:{mm:02d} {i[0]:6.2f} {i[1]:5.1f} {o[0]:6.2f} {o[1]:5.1f} | {di[0]:6.2f} {di[1]:5.1f} {do[0]:6.2f} {do[1]:5.1f} | {ui[0]:6.2f} {ui[1]:5.1f} {uo[0]:6.2f} {uo[1]:5.1f}')
