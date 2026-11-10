import sys
from ref import *
names={'NBM':'NoiseBandMomentum_NQ_MNQ','GOD':'GapOpeningDrive_NQ_MNQ','TVP':'TrendDayVwapPullback_NQ_MNQ'}
R={'NBM':nbm(),'GOD':god(conservative=False),'TVP':tvp()}
suffix=sys.argv[1] if len(sys.argv)>1 else ''
for k,nm in names.items():
    m=pd.read_csv(f'mock/tr_{nm}{suffix}.csv',header=None,names=['ed','et','xd','xt','dir','ep','xp','q','pl'])
    m['date']=pd.Timestamp('1899-12-30')+pd.to_timedelta(m.ed,unit='D'); m['em']=(m.et//60)-570; m['xm']=(m.xt//60)-570
    r=R[k].copy(); r['date']=TD[r.day.values]; r=r[r.date>=m.date.min()]
    j=r.merge(m,on=['date'],how='outer',suffixes=('_r','_m'),indicator=True)
    # match trades on same date & entry minute within 1
    a=r.merge(m,on='date',suffixes=('_r','_m')); a=a[(a.em_r-a.em_m).abs()<=1]
    same=a[(a.dir_r==a.dir_m)]
    dpx=(same.ep_r-same.ep_m).abs(); dxp=(same.xp_r-same.xp_m).abs(); dpl=(same.pts*2-same.pl)
    print(f"{k}: ref {len(r)} trades ${r.pts.sum()*2:.0f} gross | mock {len(m)} trades ${m.pl.sum():.0f} | matched {len(same)} | entry px identical {int((dpx<0.01).sum())} | exit within 1 pt {int((dxp<=1.0).sum())} | P&L diff on matched: sum ${dpl.sum():.0f}, |diff|>$20 on {int((dpl.abs()>20).sum())}")
    un_r=r[~r.set_index(['date','em']).index.isin(list(zip(same.date,same.em_r)))]; un_m=m[~m.set_index(['date','em']).index.isin(list(zip(same.date,same.em_m)))]
    print(f"   unmatched: ref-only {len(un_r)} (${un_r.pts.sum()*2:.0f}), mock-only {len(un_m)} (${un_m.pl.sum():.0f})")
    big=same[dpl.abs()>20].assign(d=dpl)
    if len(big): print(big[['date','em_r','dir_r','ep_r','ep_m','xm_r','xm_m','xp_r','xp_m','why','d']].head(6).to_string())
    if len(un_r): print(un_r[['date','em','xm','dir','ep','xp','why']].head(4).to_string())
    if len(un_m): print(un_m[['date','em','xm','dir','ep','xp','pl']].head(4).to_string())
