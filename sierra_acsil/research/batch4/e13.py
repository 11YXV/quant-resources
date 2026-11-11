from M import *
import M
print(M.MKT)
for kind in ('nr7','nr4','inside','low','nr7|inside','nr4|inside','all'):
    for orm in (15,30,60):
        R(CMP(kind,orm),f'CMP {kind} or{orm}')
for st in ('0.15','0.25','0.4'): R(CMP('nr7|inside',30,stopm=st),f'CMP nr7|inside or30 stop{st}adr')
for sf in (0.5,0.75): R(CMP('nr7|inside',30,stopf=sf),f'CMP nr7|inside or30 stopf{sf}')
for la in (k(11,0),k(15,0)): R(CMP('nr7|inside',30,last=la),f'CMP nr7|inside or30 last{la}')
