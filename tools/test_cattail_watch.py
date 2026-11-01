"""Build three Cattails with ordinary sun/cooldown, then record real spectating."""
import sys,json,time
from pathlib import Path
from demo import state,call
from control import request,screenshot
from PIL import Image
out=Path('build-test-x86/evidence/cattail');out.mkdir(parents=True,exist_ok=True)
a,b='CattailA','CattailB';start=time.monotonic();watching=False;lastshot=0;lastnote=0
def plant(n,s,kind,col,row):
 card=next((c for c in s['cards'] if c['type']==kind and c['ready']),None)
 if not card:return False
 return request(n,'plant',slot=card['slot'],col=col,row=row)['ok']
with (out/'samples.jsonl').open('a',encoding='utf-8') as log:
 while time.monotonic()-start<360:
  states={n:state(n) for n in [a,b]}
  if any(s['scene']!=3 for s in states.values()):
   (out/'stopped.json').write_text(json.dumps(states,ensure_ascii=False,indent=2),encoding='utf-8');print('Scene changed',[(n,s['scene']) for n,s in states.items()],flush=True);break
  for n,s in states.items():
   board=s['board']
   if s['network']['watch_target']:continue
   for c in board['coins']:
    if c['sun'] and not c['collecting']:request(n,'collect',coin_id=c['id'])
   board=state(n)['board'];plants=board['plants'];flowers=sum(p['type']==1 for p in plants);cats=sum(p['type']==43 for p in plants)
   cells={(round((p['x']-40)/80),p['row']) for p in plants}
   if n==a and cats<3 and (flowers>=5 or cats>0):
    for row in [2,1,3]:
     if (2,row) not in cells and plant(n,board,43,2,row):break
   elif flowers<5:
    for row in [2,1,3,0,4]:
     if (0,row) not in cells and plant(n,board,1,0,row):break
   if n==b and board['sun']>=150:
    request(n,'ui.key',key=71);request(n,'ui.key',key=49)
   if n==b:
    rows=sorted(range(5),key=lambda r:min([z['x'] for z in board['zombies'] if z['row']==r]+[1000]))
    for row in rows:
     if (1,row) not in cells and plant(n,board,0,1,row):break
  states={n:state(n) for n in [a,b]};cats=sum(p['type']==43 for p in states[a]['board']['plants'])
  if cats>=3 and not watching:
   for n in [a,b]:call(n,'p2p.set',enabled=False);call(n,'p2p.set',enabled=True)
   call(b,'watch.start',seat=1);watching=True;print('THREE CATTAILS: spectating started',flush=True)
  now=time.monotonic();log.write(json.dumps({'elapsed':now-start,'states':states},ensure_ascii=False)+'\n');log.flush()
  if now-lastnote>=15:
   print(round(now-start),[(n,s['board']['sun'],s['board']['wave'],len(s['board']['plants']),len(s['board']['zombies']),len(s['board']['projectiles'])) for n,s in states.items()],'cats',cats,flush=True);lastnote=now
  if watching and now-lastshot>=1:
   for n in [a,b]:
    tmp=out/(n+'.bmp');screenshot(n,tmp)
    with Image.open(tmp) as im:im.save(out/(n+'-'+str(round((now-start)*1000))+'.png'))
    tmp.unlink()
   lastshot=now
  time.sleep(.35)
