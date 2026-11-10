import warnings, sys, pickle; warnings.filterwarnings('ignore')
from ref import *
S={'NBM':nbm(),'GOD':god(),'TVP':tvp()}
c=float(os.environ.get('COST')); m=float(os.environ.get('MULT'))
out={}
for k,d in S.items():
    d=d[TD[d.day.values]>='2017-01-01']; net=(d.pts-c)*m; dly=net.groupby(TD[d.day.values]).sum(); out[k]=dly
    mo=dly.resample('ME').sum(); w=net[net>0]; l=net[net<=0]; eq=dly.cumsum()
    i=dly[:'2021'].resample('ME').sum().mean(); o=dly['2022':].resample('ME').sum().mean()
    print(f"{k} n={len(d)} PF {w.sum()/-l.sum():.2f} $/mo {mo.mean():.0f} (IS {i:.0f} OOS {o:.0f}) dd {(eq-eq.cummax()).min():.0f}")
pickle.dump(out,open(sys.argv[1],'wb'))
