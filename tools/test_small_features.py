"""Background dual-client regression for MOD104 small feature ports."""
import ctypes as C
from ctypes import wintypes as W
import json
from pathlib import Path
import time
from demo import call, state, click, wait, run

OUT = Path('build-test-x86/evidence/small-features-final')
NAMES = ['SmallFinalA', 'SmallFinalB']
checks = []

def pair():
    return [state(n) for n in NAMES]

def check(name, ok, detail=None):
    checks.append(dict(test=name, passed=bool(ok), detail=detail))
    (OUT/'checks.json').write_text(json.dumps(checks, ensure_ascii=False, indent=2), encoding='utf-8')
    print(name, bool(ok), detail, flush=True)
    assert ok, name

def counters():
    return [s['board']['main_counter'] for s in pair()]

def balances():
    return [s['board']['sun'] for s in pair()]

def child_window(pid):
    found=[]
    callback=C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
    @callback
    def visit(hwnd,_):
        owner=W.DWORD()
        C.windll.user32.GetWindowThreadProcessId(hwnd,C.byref(owner))
        if owner.value==pid and C.windll.user32.IsWindowVisible(hwnd):found.append(hwnd)
        C.windll.user32.EnumChildWindows(hwnd,child_visit,0)
        return True
    @callback
    def child_visit(hwnd,_):
        owner=W.DWORD()
        C.windll.user32.GetWindowThreadProcessId(hwnd,C.byref(owner))
        if owner.value==pid and C.windll.user32.IsWindowVisible(hwnd):found.append(hwnd)
        return True
    C.windll.user32.EnumWindows(visit,0)
    assert found, 'visible observer window missing'
    return found[0]

def window_key(hwnd,key,hold=0):
    send=C.windll.user32.PostMessageW
    send.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM]
    send(hwnd,0x100,key,1)
    end=time.monotonic()+hold
    while time.monotonic()<end:
        time.sleep(.2)
        send(hwnd,0x100,key,0x40000001)
    send(hwnd,0x101,key,0xC0000001)

def main():
    a,b=NAMES
    exe=Path('../PvZ/MOD104-RoomP2P-Test/SexyAppFramework.exe').resolve()
    run(exe,a,b,OUT,prepare_only=True,extra_args=('-bench-edge-kill=1',))
    for n in NAMES:click(n,535,565)
    call(a,'menu.open');time.sleep(.6)
    ss=pair();c=counters();time.sleep(.8);d=counters()
    check('menu_runs_both_without_peer_dialog',all(y>x for x,y in zip(c,d)) and
          any(w['kind']=='dialog' for w in ss[0]['widgets']) and
          not any(w['kind']=='dialog' for w in ss[1]['widgets']),[c,d])
    call(a,'ui.key',key=80);time.sleep(.6);c=counters();time.sleep(.6);d=counters()
    check('P_inside_menu_pauses_both',c==d and all(s['board']['paused'] for s in pair()),[c,d])
    call(a,'menu.close');time.sleep(.3)
    check('menu_close_preserves_pause',all(s['board']['paused'] for s in pair()))
    call(b,'ui.key',key=80);time.sleep(.5);c=counters();time.sleep(.6);d=counters()
    check('peer_P_resumes_both_without_dialog',all(y>x for x,y in zip(c,d)) and
          not any(w['kind']=='dialog' for s in pair() for w in s['widgets']),[c,d])
    call(a,'ui.key',key=32);time.sleep(.3)
    check('space_no_local_pause',not state(a)['board']['paused'])
    end=time.monotonic()+100
    while time.monotonic()<end:
        for n in NAMES:
            for coin in state(n)['board']['coins']:
                if coin['sun'] and not coin['collecting']:call(n,'collect',coin_id=coin['id'])
        if min(balances())>=100:break
        time.sleep(.5)
    time.sleep(.6)
    before=balances()
    for key in [112,114,115,116,117]:call(a,'ui.key',key=key);time.sleep(.15)
    check('self_and_empty_seats_do_not_spend',balances()==before,[before,balances()])
    before=balances();call(a,'ui.key',key=113);time.sleep(.8);after=balances()
    check('normal_F2_gift',after==[before[0]-100,before[1]+90],[before,after])
    before=balances();call(a,'ui.key',key=113);time.sleep(.3)
    check('insufficient_sun_or_cooldown_no_spend',balances()==before)
    call(b,'watch.start',seat=1)
    wait(lambda:state(b)['network']['picture_age_ms']>=0,15);time.sleep(.7)
    before=balances();call(b,'ui.key',key=112);time.sleep(.8);after=balances()
    check('parent_F1_during_watch',after==[before[0]+90,before[1]-100],[before,after])
    hwnd=child_window(state(b)['observer_pid'])
    window_key(hwnd,80);time.sleep(.8)
    check('observer_window_P_pauses_both',all(s['board']['paused'] for s in pair()))
    window_key(hwnd,80);time.sleep(.8)
    check('observer_window_P_resumes_both',all(not s['board']['paused'] for s in pair()))
    # Received aid is restricted by MOD104 and cannot fund a second gift.
    call(b,'watch.stop');time.sleep(.5)
    start=state(b)['board']['sun'];end=time.monotonic()+75
    while state(b)['board']['sun']<start+100 and time.monotonic()<end:
        for coin in state(b)['board']['coins']:
            if coin['sun'] and not coin['collecting']:call(b,'collect',coin_id=coin['id'])
        time.sleep(.5)
    call(b,'watch.start',seat=1)
    wait(lambda:state(b)['network']['picture_age_ms']>=0,15);time.sleep(1)
    hwnd=child_window(state(b)['observer_pid'])
    before=balances();window_key(hwnd,112,hold=3.8);time.sleep(.8);after=balances()
    check('observer_held_F1_sends_once_using_own_sun',after==[before[0]+90,before[1]-100],[before,after])
    call(b,'watch.stop');time.sleep(.5)
    check('watch_exit',state(b)['network']['watch_target']==0)
    (OUT/'final-states.json').write_text(json.dumps(pair(),ensure_ascii=False,indent=2),encoding='utf-8')
    for n in NAMES:call(n,'close')

if __name__=='__main__':main()
