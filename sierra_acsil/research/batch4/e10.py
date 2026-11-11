from M import *
import M
print(M.MKT,'--- DWR')
for th in (10,15,20,25,30,40):
    R(DWR(th=th,stop=0.75),f'DWR rsi2<{th} stop.75')
for st in (None,0.3,0.5,1.0,1.5):
    R(DWR(stop=st),f'DWR rsi2<20 stop{st}')
for ek in (934,944,959): R(DWR(ek=ek),f'DWR entry k{ek}')
for n in (3,4): R(DWR(n=n,th=25),f'DWR rsi{n}<25')
R(DWR(xk=k(12,0)),'DWR exit 12:00'); R(DWR(xk=k(14,0)),'DWR exit 14:00')
print('--- VAR')
for bar in (1,5): 
    for fr in (0.68,0.70,0.75): R(VAR(bar=bar,frac=fr),f'VAR bar{bar} frac{fr}')
for step in (5,15,30,60): 
    for nc in (1,2): R(VAR(step=step,nclose=nc),f'VAR step{step} n{nc}')
for st in (0.1,0.15,0.25,0.3,0.4): R(VAR(stop=st),f'VAR stop{st}')
for la in (k(11,0),k(12,0),k(14,0),k(15,0)): R(VAR(last=la),f'VAR last{la}')
for mg in (0.05,0.1,0.2): R(VAR(mingap=mg),f'VAR mingap{mg}')
for fs in (k(10,0)-1,k(10,30)-1): R(VAR(first=fs),f'VAR first>= {fs}')
print('--- PIF')
for th in (0.1,0.15,0.2,0.25,0.3,0.4):
    R(PIF(th=th),f'PIF th{th}')
for st in (None,0.1,0.15,0.2,0.4): R(PIF(stop=st),f'PIF stop{st}')
for xk in (k(9,45)-1,k(10,15)-1,k(10,30)-1,k(11,0)-1,k(11,30)-1): R(PIF(xk=xk),f'PIF exit{xk}')
for s0 in (k(8,15)-1,k(8,25)-1,k(8,45)-1): R(PIF(s0=s0),f'PIF from{s0}')
