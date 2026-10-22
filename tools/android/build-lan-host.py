"""Build the Android host from the common PC-branch relay source (no fork)."""
import argparse, os, subprocess
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--ndk',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
a.source=a.source.resolve();a.ndk=a.ndk.resolve();a.output=a.output.resolve()
assert (a.source/'hosting.go').is_file(), 'The common player-host source is required'
host='windows-x86_64' if os.name=='nt' else ('darwin-x86_64' if __import__('sys').platform=='darwin' else 'linux-x86_64')
clang=a.ndk/'toolchains/llvm/prebuilt'/host/'bin'/('clang.exe' if os.name=='nt' else 'clang')
assert clang.is_file(),clang
a.output.parent.mkdir(parents=True,exist_ok=True)
env=os.environ.copy()
env.update(GOOS='android',GOARCH='arm64',CGO_ENABLED='1',CC='"'+str(clang)+'" --target=aarch64-linux-android24')
subprocess.run(['go','build','-buildmode=c-shared','-ldflags=-s -w -extldflags=-Wl,-z,max-page-size=16384','-o',str(a.output.resolve()),'.'],cwd=a.source,env=env,check=True)
