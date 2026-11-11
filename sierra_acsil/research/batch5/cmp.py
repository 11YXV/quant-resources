import ref2 as r, pandas as pd, numpy as np, sys
r.prep()
names={'DWR':'DailyWashoutRebound','VAR':'ValueAreaReentry','CEB':'CompressionExpansionBreakout','EDF':'EndOfDayFlow'}
sfx=sys.argv[1] if len(sys.argv)>1 else ''
for k,nm in names.items():
    ref=r.stats(getattr(r,k)(),start='2016-01-01')
    m=pd.read_csv(f'build/tr_{nm}{sfx}.csv',header=None,names=['ed','et','xd','xt','dir','ep','xp','q','pl'])
    m['date']=pd.Timestamp('1899-12-30')+pd.to_timedelta(m.ed,unit='D')
    ref=ref[ref.date>=m.date.min()]
    j=ref.merge(m,on='date',how='outer',indicator=True)
    both=j[j._merge=='both']
    same_e=(np.abs(both.e-both.ep)<0.01).sum(); dpl=both.pl-both.pts*2
    print(f"{k}: model {len(ref)} trades ${ref.pts.sum()*2:.0f} gross | mock {len(m)} trades ${m.pl.sum():.0f} | same day {len(both)} | same side {(both.side==both.dir).sum()} | entry px identical {same_e} | P&L diff sum ${dpl.sum():.0f}, |diff|>$20 on {(dpl.abs()>20).sum()} | model-only {(j._merge=='left_only').sum()} mock-only {(j._merge=='right_only').sum()}")
    lo=j[j._merge=='left_only'][['date','side','eb','e','pts','why']].head(4); ro=j[j._merge=='right_only'][['date','et','dir','ep','pl']].head(4)
    if len(lo): print(lo.to_string())
    if len(ro): print(ro.to_string())
    big=both[dpl.abs()>20]
    if len(big): print(big[['date','side','e','ep','pts','pl','why','xt']].head(4).to_string())
