"""Summarize completed real-game observer measurements; no estimated bandwidth."""
import argparse
import json
from pathlib import Path


def summarize(author_path, local_path, output):
    results=[json.loads(p.read_text(encoding='utf-8')) for p in (author_path,local_path)]
    assert all(r['end_reason']=='level_complete' for r in results)
    lines=['# MOD104 观战整关带宽实测（2026-11-01）','',
           '测试底座为作者 MOD104（52d7ebca）加后台控制、日志和可选的原 ByeahFeature 本地模拟观战路径。两轮依次实测，未与编译并行。普通版、标准 ×1、第一关共 10 波，两端不种植物、不催波，僵尸到原生底线时无掉落死亡。使用作者云服务器房间。','',
           '平均值从开始订阅之前计到该关完成，包含初始化、连接建立及首次出怪前的正常等待。单位 KiB = 1024 字节，MiB = 1048576 字节。','',
           '| 方案 | 传输路径 | 时长 | 观战平均上传 | 整关观战数据 | 含普通联机的总上传 | 观看端总下载 |',
           '|---|---|---:|---:|---:|---:|---:|']
    names=['作者 MOD104 绘图预测','我们的自定义状态包＋本地模拟']
    for r,name in zip(results,names):
        src=r['totals'][r['source_instance']]
        data=src['tcp_observer_tx']+src['udp_tx'] if r['engine']=='author' else src['tcp_local_tx']
        lines.append(f"| {name} | {'P2P / UDP，首次快照含 TCP' if r['engine']=='author' else 'TCP 中继'} | {r['duration_s']:.2f} 秒 | {data/r['duration_s']/1024:.3f} KiB/s | {data/1024**2:.3f} MiB（{data:,} 字节） | {r['source_upload_Bps']/1024:.3f} KiB/s | {r['viewer_download_Bps']/1024:.3f} KiB/s |")
    lines+=['','| 方案 | 实际采集帧数 | 平均每帧压缩数据 | 平均采集间隔 | 同屏僵尸峰值 | 关卡种子 |','|---|---:|---:|---:|---:|---:|']
    for r,name in zip(results,names):
        src=r['totals'][r['source_instance']]
        lines.append(f"| {name} | {src['captures']} | {src['packed']/src['captures']/1024:.3f} KiB | {r['duration_s']/src['captures']:.3f} 秒 | {r['max_zombies']} | {r['run']['seed']} |")
    ratio=results[0]['observer_source_Bps']/results[1]['observer_source_Bps']
    lines+=['',f'本次作者方案的观战平均上传约为我们的 **{ratio:.2f} 倍**；我们的流量低约 **{(1-1/ratio)*100:.1f}%**。这个比值包括两套实现原有的发送频率、压缩与传输封装差异，不能全部归因于数据格式。','',
            '我们的普通周期为 5 秒，种植/铲除或状态变化触发更新的最小间隔为 1 秒；这一轮没有种植、铲除操作。首次发送关卡配置，关卡完成时配置版本变化会再发送一次，普通中间帧不重复发送。接收端有独立观战进程，实际恢复对象并持续模拟，保留逐对象的小误差位置容忍。','',
            '作者方案使用原来的绘图帧采集节奏与预测，开启 P2P 后持续传输。作者默认关闭 P2P 时的单次快照未混入持续观战的平均值。','',
            '计数来自成功的 socket 收发，不是发送队列长度或未发送的压缩包体积。作者“观战数据”包含 UDP 探测、心跳、帧分片与首次 TCP 快照；我们的“观战数据”为 BOARD_SNAPSHOT 实际 TCP 报文字节（含 flow 封装），其单独订阅与配置 ACK 在连接总量中。均不计 IP/TCP/UDP 底层头部、链路层及系统 TCP 重传。','',
            '两轮为同模式、同难度，但随机种子不同；具体怪种、同屏数量和持续时间并不完全相同。此结果是两次真实普通关卡的观测，不是严格相同战场逐帧回放，也不代表有大量植物、子弹、特效或高倍率僵尸时的带宽。P2P 在本机双端实际成功，不保证其他 NAT 环境也能直连。','',
            '实测记录：','',f'- 作者：[result.json]({author_path.resolve().as_posix()})',f'- 我们：[result.json]({local_path.resolve().as_posix()})',
            '- 对应目录内 samples.jsonl 是逐次采样，startup.json 是初始状态，截图用于确认实际游戏画面。',
            '- 之前中断、未开局或未收到有效观战画面的尝试未纳入结果。',
            '- AI 生成与移植代码，仅做本次初步验证；未提交或推送。','']
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('\n'.join(lines),encoding='utf-8')
    print(json.dumps({'author_KiBps':results[0]['observer_source_Bps']/1024,'local_KiBps':results[1]['observer_source_Bps']/1024,'ratio':ratio,'report':str(output.resolve())},ensure_ascii=False))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('author',type=Path);p.add_argument('local',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();summarize(a.author,a.local,a.output)
