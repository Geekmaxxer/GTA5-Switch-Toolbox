"""Execute the C navigation/UTF-8 helpers against boundary and timing cases."""
import argparse,ctypes,json,os,subprocess
from pathlib import Path

class Repeat(ctypes.Structure):_fields_=[('held',ctypes.c_uint),('next',ctypes.c_uint64)]

def test(output):
    root=Path(__file__).resolve().parents[1];output.mkdir(parents=True,exist_ok=True)
    locator=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    install=subprocess.check_output([str(locator),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    script=output/'compile.cmd';dll=output/'ui_core.dll'
    script.write_text(f'@call "{install}/VC/Auxiliary/Build/vcvars64.bat" >nul\n@cl /nologo /LD /O2 /MD /std:c11 "{root}/source/gc_ui_core.c" /link /OUT:"{dll}" /EXPORT:ui_repeat /EXPORT:ui_viewport /EXPORT:ui_grid_move /EXPORT:ui_utf8_next\n')
    run=subprocess.run(['cmd','/c',str(script)],cwd=output,capture_output=True,text=True);(output/'compile.log').write_text(run.stdout+run.stderr)
    assert run.returncode==0,run.stdout+run.stderr
    lib=ctypes.CDLL(str(dll));lib.ui_repeat.argtypes=[ctypes.POINTER(Repeat),ctypes.c_uint,ctypes.c_uint64]
    lib.ui_viewport.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.POINTER(ctypes.c_int),ctypes.c_int]
    lib.ui_utf8_next.argtypes=[ctypes.POINTER(ctypes.c_char_p)];lib.ui_utf8_next.restype=ctypes.c_uint32
    checks=0
    def check(okay):
        nonlocal checks
        assert okay,checks;checks+=1
    for count in [0,1,5,6,7,128,256]:
        for selected in [-3,0,count-1,count+5]:
            for visible in [1,5,6,8,20]:
                for initial in [-3,0,count-1,count+20]:
                    scroll=ctypes.c_int(initial);result=lib.ui_viewport(count,selected,ctypes.byref(scroll),visible)
                    check(result==0 and scroll.value==0 if not count else
                          0<=result<count and 0<=scroll.value<=max(0,count-visible) and scroll.value<=result<scroll.value+visible)
    state=Repeat();check(lib.ui_repeat(ctypes.byref(state),1,100)==1)
    check(lib.ui_repeat(ctypes.byref(state),1,200)==0);check(lib.ui_repeat(ctypes.byref(state),1,419)==0)
    check(lib.ui_repeat(ctypes.byref(state),1,420)==1);check(lib.ui_repeat(ctypes.byref(state),1,421)==0)
    check(lib.ui_repeat(ctypes.byref(state),2,430)==2);check(lib.ui_repeat(ctypes.byref(state),0,431)==0)
    check(lib.ui_repeat(ctypes.byref(state),2,432)==2)
    state=Repeat();check(lib.ui_repeat(ctypes.byref(state),8,1<<34)==8);check(lib.ui_repeat(ctypes.byref(state),8,(1<<34)+319)==0)
    for selected in range(10):
        for direction in [1,2,4,8]:check(0<=lib.ui_grid_move(selected,10,4,direction)<10)
    check(lib.ui_grid_move(3,10,4,8)==3);check(lib.ui_grid_move(0,10,4,4)==0)
    check(lib.ui_grid_move(7,10,4,2)==9);check(lib.ui_grid_move(9,10,4,8)==9)
    for data,expected in [(b'ASCII',[65,83,67,73,73]),('中文'.encode(),[0x4e2d,0x6587]),('🎵'.encode(),[0x1f3b5]),
                          (b'\xc2',[0xfffd]),(b'\xe4\xb8',[0xfffd,0xfffd]),(b'\xffA',[0xfffd,65]),
                          (b'\xc0\x80',[0xfffd,0xfffd]),(b'\xed\xa0\x80',[0xfffd]*3),(b'\xf4\x90\x80\x80',[0xfffd]*4)]:
        buffer=ctypes.create_string_buffer(data);cursor=ctypes.c_char_p(ctypes.addressof(buffer));actual=[]
        for _ in range(len(data)+1):
            code=lib.ui_utf8_next(ctypes.byref(cursor))
            if not code:break
            actual.append(code)
        check(actual==expected)
    report={'checks':checks,'console_verified':False};(output/'ui-core-tests.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);test(parser.parse_args().output.resolve())
