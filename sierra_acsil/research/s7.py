from lib import *
ok=~SHORT&~np.isnan(SIG[:,0])&~np.isnan(PC)
print('--- response: z=(c_t/open-1)/sig_t at time t -> fwd return to 15:59 in units of ADR-ish (bp)')
rows=[]
for t in (29,59,89,119,179,239,299,329,359):
    z=(C[:,t]/DO-1)/SIG[:,t]; f=(O[:,389]/O[:,t+1]-1)*1e4
    for lo,hi in ((-9,-2.5),(-2.5,-1.5),(-1.5,-1),(-1,-0.5),(-0.5,0.5),(0.5,1),(1,1.5),(1.5,2.5),(2.5,9)):
        for nm,m2 in (('IS',TD<'2022'),('OOS',TD>='2022')):
            m=ok&m2&(z>=lo)&(z<hi)
            rows.append((t+1,f'{lo}..{hi}',nm,m.sum(),f[m].mean(),f[m].mean()/f[m].std()*np.sqrt(max(1,m.sum()))))
r=pd.DataFrame(rows,columns=['t','z','s','n','bp','tstat'])
print(r.pivot_table(index='z',columns=['s','t'],values='bp',sort=False).round(0).to_string())
print(r.pivot_table(index='z',columns=['s','t'],values='n',sort=False).round(0).to_string())
