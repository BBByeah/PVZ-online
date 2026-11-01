"""Start two ordinary MOD104 clients via local control only. Never sends desktop input."""
import argparse
import ctypes as C
from ctypes import wintypes as W
import json
from pathlib import Path
import subprocess
import time
from control import request, screenshot

def call(n,op,**kw):
    r=request(n,op,**kw)
    if not r['ok']:raise RuntimeError((n,op,r))
    return r['result']

def state(n):return call(n,'state')
def click(n,x,y):return call(n,'ui.click',x=x,y=y)
def wait(test,seconds=30):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        value=test()
        if value:return value
        time.sleep(.15)
    raise TimeoutError('Game did not reach requested stage')

def initialize(n):
    wait(lambda:not state(n)['loading'],60)
    if state(n)['scene']==0:click(n,395,555)
    time.sleep(.3)
    for _ in range(20):
        s=state(n);ds=[w for w in s['widgets'] if w['kind']=='dialog']
        if any(d.get('id')==33 for d in ds):
            click(n,395,370)
        elif any(d.get('id')==30 for d in ds):
            click(n,390,312);time.sleep(.3)
            edits=[w for w in state(n)['widgets'] if w['kind']=='edit']
            if edits and not edits[-1].get('text'):
                call(n,'ui.text',text=n);time.sleep(.2)
            edits=[w for w in state(n)['widgets'] if w['kind']=='edit']
            if edits and edits[-1].get('text'):click(n,305,395)
        elif any(w.get('label')=='知道了' for w in s['widgets']):click(n,395,500)
        # Announcements can be created a few frames after the main menu appears.
        time.sleep(.3)

def enter_online(n):
    for _ in range(3):
        click(n,550,140);time.sleep(.4)
        if any(w.get('kind')=='dialog' and w.get('id')==56 for w in state(n)['widgets']):
            click(n,570,133);time.sleep(.2);return
        # A first adventure click can create the name dialog after loading.
        initialize(n)
    raise RuntimeError('Online lobby did not open after profile initialization')

def prepare(a,b):
    end=time.monotonic()+60
    while time.monotonic()<end:
        for n in [a,b]:
            s=state(n)
            if s.get('team',{}).get('can_vote'):
                choices=s['team']['choices'];choice=next((v['id'] for v in choices if v['enabled']),-1)
                call(n,'team.vote',choice=choice);continue
            ds=[w for w in s['widgets'] if w['kind']=='dialog']
            if any(d.get('title')=='房主开始了游戏' for d in ds):click(n,323,418);continue
            if any(d.get('id')==57 for d in ds):
                ws=[w for w in s['widgets'] if w['kind']=='button' and w.get('id')==100 and w['enabled']]
                if ws:
                    w=ws[-1];click(n,w['x']+w['w']//2,w['y']+w['h']//2)
            elif s['scene']==2 and not s.get('team',{}).get('vote_open'):
                # The normal preparation panel is drawn directly on the lawn.
                click(n,630,190)
        if all(state(n)['scene']==3 for n in [a,b]):return
        time.sleep(.25)
    raise TimeoutError('Preparation unfinished; inspect state and screenshot')

def run(exe,a,b,out,launch=True,prepare_only=False,extra_args=()):
    out.mkdir(parents=True,exist_ok=True)
    if launch:
        for n in [a,b]:subprocess.Popen([str(exe),'-control='+n,*extra_args],cwd=exe.parent)
    for n,x in [(a,-1880),(b,-1000)]:
        wait(lambda: not state(n)['loading'],60);call(n,'window.position',x=x,y=120);initialize(n)
    user=C.windll.user32
    class Point(C.Structure):_fields_=[('x',W.LONG),('y',W.LONG)]
    user.GetForegroundWindow.restype=W.HWND
    p=Point();user.GetCursorPos(C.byref(p));before={'cursor':[p.x,p.y],'foreground':int(user.GetForegroundWindow() or 0)}
    for n in [a,b]:enter_online(n)
    click(a,230,382);room=wait(lambda:state(a).get('network',{}).get('room'))
    click(a,620,477) # make this demonstration room private
    click(b,350,318);time.sleep(.15);call(b,'ui.text',text=room);click(b,566,382)
    wait(lambda:state(b).get('network',{}).get('connected'))
    time.sleep(.5);click(a,654,520);time.sleep(.4)
    if any(w.get('label')=='新开一局' or '继续闯关' in w.get('title','') for w in state(a)['widgets']):
        click(a,466,420);time.sleep(.3)
    # Normal mode, standard x1. No advanced/custom options.
    click(a,400,235);time.sleep(.4);click(b,323,418)
    prepare(a,b)
    if not prepare_only:
        for n in [a,b]:
            if state(n)['board']['wave']==0:click(n,535,565)
        # Leave the second client actually watching the first. P2P stays off by default.
        call(b,'watch.start',seat=1)
        wait(lambda:state(b)['network']['picture_age_ms']>=0,15)
    user.GetCursorPos(C.byref(p));after={'cursor':[p.x,p.y],'foreground':int(user.GetForegroundWindow() or 0)}
    evidence={'room':room,'before':before,'after':after,'instances':[a,b],'initial_states':{a:state(a),b:state(b)}}
    (out/'startup.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2),encoding='utf-8')
    for n in [a,b]:screenshot(n,out/(n+'.bmp'))
    print(json.dumps({'started':True,'room':room,'desktop_unchanged':before==after},ensure_ascii=False))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,default=Path(__file__).parent/'SexyAppFramework.exe');p.add_argument('--a',default='DemoA');p.add_argument('--b',default='DemoB');p.add_argument('--out',type=Path,default=Path('evidence'));p.add_argument('--running',action='store_true');args=p.parse_args()
    run(args.exe.resolve(),args.a,args.b,args.out,not args.running)
