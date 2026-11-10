from lib import *
from s1 import noise
from s2 import orb, sim_bracket
gap=(DO-PC)/ADR
def god(n=5,sf=0.1,ming=0.0,last=180,tgtR=None,stopmode='adr',eod=389,buf=0.25):
    d=orb(n=n,stopmode=stopmode,stopfrac=sf,last=last,tgtR=tgtR,eod=eod,buf=buf); d['g']=gap[d.day.values]
    return d[(np.sign(d.g)==d.dir)&(d.g.abs()>=ming)]
if __name__=='__main__':
  print('--- gap-aligned opening drive sensitivity')
  for n in (3,5,10,15,30):
      for sf in (0.05,0.1,0.15,0.25):
          stats(god(n,sf),label=f'GOD n{n} stop{sf}ADR')
  for mg in (0.05,0.1,0.2,0.4): stats(god(5,0.1,mg),label=f'GOD mingap{mg}')
  stats(god(5,1.0,stopmode='opp'),label='GOD stop=opp side'); stats(god(5,0.5,stopmode='opp'),label='GOD stop=mid')
  for last in (15,30,60,120): stats(god(5,0.1,last=last),label=f'GOD entry by {last}m')
  for tg in (3,5,10): stats(god(5,0.1,tgtR=tg),label=f'GOD tgt {tg}R')
  for e in (120,240,330): stats(god(5,0.1,eod=e),label=f'GOD exit at {e}')
  d=god(5,0.1); stats(d,cost=2.5,label='GOD cost x2'); stats(d[d.dir>0],label='GOD long'); stats(d[d.dir<0],label='GOD short')
  print(d.groupby(TD[d.day.values].year).apply(lambda x:pd.Series(dict(n=len(x),net=((x.pts-COST)*2).sum(),wr=(x.pts>COST).mean()))).round(0).T)
