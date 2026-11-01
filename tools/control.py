"""Local MOD104 test controller. Python 3, standard library only; no desktop input."""
import argparse
import ctypes as C
from ctypes import wintypes as W
import json
from pathlib import Path
import struct
import subprocess
import time
import uuid

k = C.WinDLL('kernel32', use_last_error=True)
k.CreateFileW.argtypes = [W.LPCWSTR, W.DWORD, W.DWORD, C.c_void_p, W.DWORD, W.DWORD, W.HANDLE]
k.CreateFileW.restype = W.HANDLE
k.ReadFile.argtypes = [W.HANDLE, C.c_void_p, W.DWORD, C.POINTER(W.DWORD), C.c_void_p]
k.WriteFile.argtypes = k.ReadFile.argtypes
k.CloseHandle.argtypes = [W.HANDLE]
k.PeekNamedPipe.argtypes = [W.HANDLE,C.c_void_p,W.DWORD,C.c_void_p,C.POINTER(W.DWORD),C.c_void_p]

def request(instance, op, timeout=12, **params):
    name = r'\\.\pipe\PVZ-MOD104-test-' + instance
    end = time.monotonic() + timeout
    while True:
        h = k.CreateFileW(name, 0xC0000000, 0, None, 3, 0, None)
        if h != C.c_void_p(-1).value:
            break
        if time.monotonic() >= end:
            raise TimeoutError(f'Control pipe unavailable: {instance}, error={C.get_last_error()}')
        time.sleep(.05)
    ident = uuid.uuid4().hex
    payload = (json.dumps(dict(id=ident, op=op, **params), ensure_ascii=False) + '\n').encode('utf-8')
    try:
        n = W.DWORD()
        if not k.WriteFile(h, payload, len(payload), C.byref(n), None) or n.value != len(payload):
            raise OSError(C.get_last_error(), 'request write failed')
        data = bytearray()
        while b'\n' not in data:
            available = W.DWORD()
            if not k.PeekNamedPipe(h,None,0,None,C.byref(available),None):
                raise OSError(C.get_last_error(), 'pipe disconnected')
            if available.value:
                buf = C.create_string_buffer(min(available.value,65536))
                if not k.ReadFile(h,buf,len(buf),C.byref(n),None):
                    raise OSError(C.get_last_error(), 'response read failed')
                data.extend(buf.raw[:n.value])
            else:
                if time.monotonic() >= end:
                    raise TimeoutError('Command outcome unknown; query state before retrying')
                time.sleep(.01)
        k.WriteFile(h,b'\x06',1,C.byref(n),None)
        result = json.loads(data.split(b'\n',1)[0])
        if result['id'] != ident:
            raise RuntimeError('Response ID mismatch')
        return result
    finally:
        k.CloseHandle(h)

def screenshot(instance, path):
    status = request(instance,'state')
    if not status['ok']:
        raise RuntimeError(status)
    pid = status['result']['pid']
    u=C.WinDLL('user32',use_last_error=True);g=C.WinDLL('gdi32',use_last_error=True)
    u.SetProcessDPIAware()
    found=[]
    @C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
    def enum(h,l):
        p=W.DWORD();u.GetWindowThreadProcessId(h,C.byref(p));r=W.RECT();u.GetClientRect(h,C.byref(r))
        if p.value==pid and r.right>=790 and r.bottom>=590:found.append(h)
        return True
    u.EnumWindows(enum,0)
    if not found:raise RuntimeError('No game window')
    h=found[0];r=W.RECT();u.GetClientRect(h,C.byref(r));w,hh=r.right,r.bottom
    u.GetDC.argtypes=[W.HWND];u.GetDC.restype=W.HDC
    u.ReleaseDC.argtypes=[W.HWND,W.HDC]
    u.PrintWindow.argtypes=[W.HWND,W.HDC,W.UINT]
    g.CreateCompatibleDC.argtypes=[W.HDC];g.CreateCompatibleDC.restype=W.HDC
    g.CreateCompatibleBitmap.argtypes=[W.HDC,C.c_int,C.c_int];g.CreateCompatibleBitmap.restype=W.HBITMAP
    g.SelectObject.argtypes=[W.HDC,W.HANDLE];g.SelectObject.restype=W.HANDLE
    g.DeleteObject.argtypes=[W.HANDLE];g.DeleteDC.argtypes=[W.HDC]
    g.GetDIBits.argtypes=[W.HDC,W.HBITMAP,W.UINT,W.UINT,C.c_void_p,C.c_void_p,W.UINT]
    dc=u.GetDC(h);mem=g.CreateCompatibleDC(dc);bmp=g.CreateCompatibleBitmap(dc,w,hh);old=g.SelectObject(mem,bmp)
    try:
        if not u.PrintWindow(h,mem,1):raise RuntimeError('PrintWindow failed')
        header=struct.pack('<IiiHHIIiiII',40,w,-hh,1,32,0,w*hh*4,0,0,0,0)
        info=C.create_string_buffer(header);bits=C.create_string_buffer(w*hh*4)
        g.SelectObject(mem,old)
        if not g.GetDIBits(mem,bmp,0,hh,bits,info,0):raise RuntimeError('GetDIBits failed')
        path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(struct.pack('<2sIHHI',b'BM',54+len(bits),0,0,54)+header+bits.raw)
        return {'ok':True,'path':str(path.resolve())}
    finally:
        g.SelectObject(mem,old);g.DeleteObject(bmp);g.DeleteDC(mem);u.ReleaseDC(h,dc)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('instance');p.add_argument('op');p.add_argument('params',nargs='*')
    a=p.parse_args();values={}
    for item in a.params:
        key,sep,value=item.partition('=')
        if not sep:p.error('Arguments must be key=value')
        try:values[key]=json.loads(value)
        except json.JSONDecodeError:values[key]=value
    if a.op=='launch':
        exe=Path(values.get('exe',Path(__file__).resolve().parent/'SexyAppFramework.exe')).resolve()
        proc=subprocess.Popen([str(exe),'-control='+a.instance],cwd=exe.parent)
        result={'ok':True,'pid':proc.pid,'instance':a.instance}
    elif a.op=='screenshot':result=screenshot(a.instance,values.get('path',a.instance+'.bmp'))
    else:result=request(a.instance,a.op,**values)
    print(json.dumps(result,ensure_ascii=False,indent=2))
    return 0 if result['ok'] else 1

if __name__=='__main__':
    import sys
    sys.stdout.reconfigure(encoding='utf-8')
    raise SystemExit(main())
