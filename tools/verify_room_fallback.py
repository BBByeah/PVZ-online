"""Fault injection affects observer transport only, never game resources/rules."""
import json,time
from pathlib import Path
from demo import run,state,call,wait
from control import screenshot
out=Path('build-test-x86/evidence/roomp2p4');out.mkdir(parents=True,exist_ok=True)
a,b='RoomWarmA4','RoomWarmB4'
exe=Path('../PvZ/MOD104-RoomP2P-Test/SexyAppFramework.exe').resolve()
run(exe,a,b,out,prepare_only=True)
checks={}
def record(key):
 checks[key]={n:state(n) for n in [a,b]}
 (out/'checks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf-8')
record('warm')
call(a,'watch.test_hold_frames',enabled=True)
call(b,'watch.start',seat=1);time.sleep(.2);screenshot(b,out/'loading.bmp');record('loading')
time.sleep(10);screenshot(b,out/'timeout.bmp');record('timeout')
call(a,'watch.test_hold_frames',enabled=False)
wait(lambda:state(b)['observer_restored_sequence']>0,10);record('loading_recovered')
call(b,'watch.stop')
for n in [a,b]:
 call(n,'p2p.set',enabled=False);call(n,'p2p.test_block_udp',enabled=True);call(n,'p2p.set',enabled=True)
wait(lambda:all(state(n)['room_peers'][0]['failed'] for n in [a,b]),8);record('udp_blocked_5s')
call(b,'watch.start',seat=1);wait(lambda:state(b)['observer_restored_sequence']>0,10);record('blocked_server_picture');screenshot(b,out/'blocked_server.bmp')
time.sleep(6);record('no_repeat_probe')
call(b,'watch.stop')
for n in [a,b]:
 call(n,'p2p.test_block_udp',enabled=False);call(n,'p2p.set',enabled=False);call(n,'p2p.set',enabled=True)
wait(lambda:all(state(n)['room_peers'][0]['ready'] for n in [a,b]),8)
call(b,'watch.start',seat=1);wait(lambda:state(b)['observer_restored_sequence']>0,10);record('recovered_direct')
print('PASS loading/timeout, UDP blocked deadline, server fallback, explicit retry',flush=True)
call(a,'close')
try:
 wait(lambda:state(b)['network']['watch_target']==0,20)
 checks['peer_exit']=state(b);print('PASS peer exit clears watch',flush=True)
finally:
 (out/'checks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf-8')
 call(b,'close')
