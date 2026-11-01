"""Real normal-mode room test, through named pipes only. No desktop input."""
import json,time,ctypes as C
from pathlib import Path
from demo import run,state,call,wait
from control import screenshot
out=Path('build-test-x86/evidence/roomp2p3');out.mkdir(parents=True,exist_ok=True)
exe=Path('../PvZ/MOD104-RoomP2P-Test/SexyAppFramework.exe').resolve()
a,b='RoomWarmA3','RoomWarmB3'
run(exe,a,b,out,prepare_only=True)
checks={}
def record(key):
 checks[key]={n:state(n) for n in [a,b]}
 (out/'checks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf-8')
def watch(n,seat):
 start=time.monotonic();call(n,'watch.start',seat=seat)
 wait(lambda:state(n)['observer_restored_sequence']>0,12)
 checks[n+'_last_display_ms']=round((time.monotonic()-start)*1000)
record('before_watch')
for n in [a,b]:
 if state(n)['board']['wave']==0:call(n,'ui.click',x=535,y=565)
watch(b,1);record('first_direct');screenshot(b,out/'direct.bmp')
pid=state(b)['observer_pid'];call(b,'watch.stop');time.sleep(.3);record('stopped')
watch(b,1);assert state(b)['observer_pid']==pid;record('reentry')
watch(a,2);record('duplex');call(a,'watch.stop');time.sleep(.2)
assert state(b)['network']['watch_target']==1
record('independent_stop')
call(a,'p2p.set',enabled=False);time.sleep(.2);call(b,'watch.stop');watch(b,1)
wait(lambda:state(b)['network']['p2p']=='fallback',8);record('fallback');screenshot(b,out/'fallback.bmp')
call(b,'p2p.set',enabled=False);record('disabled')
call(a,'p2p.set',enabled=True);call(b,'p2p.set',enabled=True)
wait(lambda:all(state(n)['room_peers'][0]['ready'] for n in [a,b]),8)
call(b,'watch.stop');watch(b,1);record('reenabled')
# Collect genuine dropped sun, plant using current card cooldown/cost, and shovel.
call(b,'watch.stop')
for n in [a,b]:
 s=state(n);print(n,json.dumps(s['board'],ensure_ascii=True)[:1200],flush=True)
record('before_actions')
print('Core room/watch checks passed; clients remain in normal game for action checks.',flush=True)
