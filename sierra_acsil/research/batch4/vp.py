from L import *
# prior-day RTH volume profile: POC, VAH, VAL (70%), computed per day then shifted
def profile(d,frac=0.70):
    h=HF[d,930:1320]; l=LF[d,930:1320]; v=V[d,930:1320]
    lo=np.floor(l.min()); hi=np.ceil(h.max()); bs=max(1.0,round((hi-lo)/120))
    nbin=int((hi-lo)/bs)+1; vol=np.zeros(nbin)
    a=((l-lo)/bs).astype(int); b=((h-lo)/bs).astype(int)
    for i in range(len(v)):
        vol[a[i]:b[i]+1]+=v[i]/(b[i]-a[i]+1)
    p=vol.argmax(); tot=vol.sum(); acc=vol[p]; i=j=p
    while acc<frac*tot:
        up=vol[j+1] if j+1<nbin else -1; dn=vol[i-1] if i>0 else -1
        if up<0 and dn<0: break
        if up>=dn: j+=1; acc+=up
        else: i-=1; acc+=dn
    return lo+(p+0.5)*bs, lo+(j+1)*bs, lo+i*bs
poc=np.full(N,np.nan); vah=poc.copy(); val=poc.copy()
for d in range(N):
    if FULL[d]: poc[d],vah[d],val[d]=profile(d)
np.savez('vp.npz',poc=prevref(poc),vah=prevref(vah),val=prevref(val))
print(np.nanmean((vah-val)/RNG))
