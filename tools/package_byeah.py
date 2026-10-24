"""Build a player package from licensed local assets, never from QA folders."""
import argparse
import hashlib
from pathlib import Path
import shutil
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--assets', type=Path, required=True)
p.add_argument('--exe', type=Path, required=True)
p.add_argument('--host', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--revision', required=True)
a = p.parse_args()
assert not a.output.exists(), 'Output already exists'
assert not a.output.with_suffix('.zip').exists(), 'Archive already exists'
for path in [a.exe, a.host, a.assets/'main.pak', a.assets/'pvz_zh.pak', a.assets/'bass.dll']:
    assert path.is_file(), path
a.output.mkdir(parents=True)
for name in ['main.pak', 'pvz_zh.pak', 'LICENSE']:
    shutil.copy2(a.assets/name, a.output/name)
for path in a.assets.glob('*.dll'):
    if path.name.lower() == 'bass.dll' or path.name.lower().startswith(('msvcp', 'vcruntime', 'vccorlib', 'concrt')):
        shutil.copy2(path, a.output/path.name)
shutil.copytree(a.assets/'properties', a.output/'properties')
shutil.copy2(a.exe, a.output/'SexyAppFramework.exe')
shutil.copy2(a.host, a.output/'pvz-lan-host.exe')
(a.output/'启动游戏.bat').write_bytes(b'@echo off\r\ncd /d "%~dp0"\r\nstart "" "SexyAppFramework.exe"\r\n')
(a.output/'使用说明.txt').write_text(f'''植物大战僵尸 MOD95 · ByeahFeature
源码提交：{a.revision}

完整解压后，双击“启动游戏.bat”或 SexyAppFramework.exe。
包含正式游戏、资源、汉化、运行库以及游戏内开房需要的 pvz-lan-host.exe。
不包含双端测试程序或测试存档。

沿用 MOD90 存档位置：%ProgramData%\\PlantsVsZombies\\userdata。
同机已有玩家资料直接读取；换电脑请自行备份并迁移此目录。
包内不携带个人存档。升级前建议保留旧存档备份。

保留本地模拟观战、增量/完全更新、卡槽和阳光数值，隐藏掉落阳光。
F1–F6 向对应队友赠送阳光，扣自己 100、对方直接获得 90。
菜单仅本机打开且不暂停；P 切换全队真暂停。
联机请所有人使用同一个包，观战协议与原版 MOD95 不同。

持续日志
自动保存在游戏目录的 logs 文件夹，不需要手动开启。
每次启动和每个进程分别建文件，包含时间、线程、网络事件、观战事件。
每 5 秒记录战场对象数量、模拟进度、暂停、联机及 3D 状态。
游戏日志达到 16 MB 后续写下一文件，重启不覆盖之前记录。
隐藏的开房服务也保留 host- 开头的日志。
目录无法写入时，备用位置为 %LOCALAPPDATA%\\PvZ-ByeahFeature-Logs。
正常退出会有 shutdown complete；崩溃可能没有结束标记。
如需排查，把问题发生时间、操作经过和 logs 文件夹一起发来。
日志只保存本地，不自动上传；可能包含房间和网络地址信息。
旧日志不会自动删除，确认无需排查后可在游戏关闭时自行清理。
原有 crash.txt 仍按游戏的存档/观战数据目录保存。

本分支适配代码由 AI 生成，仅做初步测试。
''', encoding='utf-8-sig')
files = sorted(x for x in a.output.rglob('*') if x.is_file())
(a.output/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(x.read_bytes()).hexdigest()+'  '+x.relative_to(a.output).as_posix()+'\n' for x in files), encoding='utf-8')
archive = a.output.with_suffix('.zip')
with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for path in sorted(a.output.rglob('*')):
        if path.is_file(): z.write(path, path.relative_to(a.output.parent))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert not any(x in name.lower() for name in z.namelist() for x in ('watchprobe', '/userdata/', '/logs/', 'qa-'))
print(archive)
print('SHA256', hashlib.sha256(archive.read_bytes()).hexdigest())
