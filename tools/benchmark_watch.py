"""Measure actual socket traffic over one normal level; no resource cheats."""
import argparse
import json
from pathlib import Path
import time
from control import request, screenshot
from demo import run, call, state, click, wait


def measure(exe, engine, out, tag, timeout=1200):
    out.mkdir(parents=True, exist_ok=True)
    a, b = tag+'A', tag+'B'
    run(exe, a, b, out, prepare_only=True,
        extra_args=('-watchengine='+engine, '-bench-edge-kill=1'))
    initial = state(a)
    assert initial['edge_kill'] and initial['run']['mode'] == 1 and initial['run']['difficulty'] == 1, initial.get('run')
    for n in (a,b):call(n,'p2p.set',enabled=engine=='author')
    # Include initial config, subscription, handshake, and first frame in totals.
    baseline={n:call(n,'metrics') for n in (a,b)}
    t0=time.monotonic()
    call(b,'watch.start',seat=1)
    if engine=='author':
        wait(lambda:state(b)['network']['p2p']=='direct',30)
    else:
        wait(lambda:state(b)['observer_restored_sequence']>0,30)
    ready={n:call(n,'metrics') for n in (a,b)}
    for n in (a,b):click(n,535,565)
    started=time.monotonic()
    samples=[]
    end_reason='timeout'
    stream=out/'samples.jsonl'
    with stream.open('w',encoding='utf-8') as f:
        while time.monotonic()-started<timeout:
            sa,sb=state(a),state(b)
            ba=sa.get('board',{})
            sample={'elapsed_s':time.monotonic()-t0,'scene':sa['scene'],
                    'level':ba.get('level'),'wave':ba.get('wave'),
                    'num_waves':ba.get('num_waves'),'complete':ba.get('complete'),
                    'zombies':len(ba.get('zombies',[])),'coins':len(ba.get('coins',[])),
                    'plants':len(ba.get('plants',[])),
                    'watch':sb.get('network'),
                    'restored_sequence':sb.get('observer_restored_sequence'),
                    'source':call(a,'metrics'),'viewer':call(b,'metrics')}
            samples.append(sample);f.write(json.dumps(sample,ensure_ascii=False)+'\n');f.flush()
            if len(samples)%20==1:
                print(json.dumps({'engine':engine,**{k:sample[k] for k in ('elapsed_s','wave','zombies','restored_sequence')},'p2p':sample['watch']['p2p']},ensure_ascii=False),flush=True)
            if sa['scene']==4:
                end_reason='unexpected_loss';break
            if ba.get('complete') or ba.get('level')!=initial['board']['level']:
                end_reason='level_complete';break
            if not sa.get('network',{}).get('connected') or not sb.get('network',{}).get('connected'):
                end_reason='disconnected';break
            time.sleep(.5)
    end={n:call(n,'metrics') for n in (a,b)}
    elapsed=time.monotonic()-t0
    def delta(x,y):return {k:x[k]-y[k] for k in x if k!='time_ms'}
    totals={n:delta(end[n],baseline[n]) for n in (a,b)}
    init={n:delta(ready[n],baseline[n]) for n in (a,b)}
    result={'engine':engine,'source_instance':a,'viewer_instance':b,'room':initial['network']['room'],
            'run':initial['run'],'initial_level':initial['board']['level'],
            'num_waves':initial['board']['num_waves'],'duration_s':elapsed,
            'battle_duration_s':time.monotonic()-started,'end_reason':end_reason,
            'max_zombies':max(s['zombies'] for s in samples),
            'totals':totals,'initialization':init,
            'source_upload_Bps':(totals[a]['tcp_tx']+totals[a]['udp_tx'])/elapsed,
            'viewer_download_Bps':(totals[b]['tcp_rx']+totals[b]['udp_rx'])/elapsed,
            'observer_source_Bps':(totals[a]['tcp_observer_tx']+totals[a]['udp_tx'] if engine=='author' else totals[a]['tcp_local_tx'])/elapsed}
    (out/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    for n in (a,b):screenshot(n,out/(n+'-end.bmp'))
    print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
    for n in (b,a):request(n,'close')
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--engine',choices=['author','local'],required=True)
    p.add_argument('--tag',required=True)
    p.add_argument('--out',type=Path,required=True)
    args=p.parse_args()
    try:
        measure(args.exe.resolve(),args.engine,args.out,args.tag)
    finally:
        # Close only this run's explicitly named instances, including on failure.
        for suffix in ('B','A'):
            try:request(args.tag+suffix,'close',timeout=1)
            except (OSError,TimeoutError):pass
