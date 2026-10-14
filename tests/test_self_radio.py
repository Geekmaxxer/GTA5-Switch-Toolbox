"""Optional real-archive integration check; works only on disposable copies.

The independent RPF reader is supplied by the maintainer, not shipped in the NRO.
"""
import argparse,ctypes,hashlib,importlib.util,json,shutil
from pathlib import Path
from test_scripts import compile_library
from test_packages import Pack

def main(a):
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    lib=compile_library(Path(__file__).resolve().parents[1],out,a.zlib_source.resolve())
    game=out/'fixture-game';home=out/'fixture-packages'
    for path in (game,home):
        if path.exists():
            assert out in path.resolve().parents;shutil.rmtree(path)
        path.mkdir()
    package=home/'self-radio';shutil.copytree(a.package,package)
    archive=game/'romfs/update/update.rpf';archive.parent.mkdir(parents=True)
    shutil.copy2(a.archive,archive)
    spec=importlib.util.spec_from_file_location('independent_rpf',a.rpf_reader)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    original=module.Rpf(a.archive);oldsha=module.sha(a.archive)
    pack=Pack();err=ctypes.create_string_buffer(512)
    assert lib.gc_package_read(str(package).encode(),ctypes.byref(pack),err,512)==0,err.value
    assert pack.count==18
    assert lib.gc_package_install(ctypes.byref(pack),None,None,err,512)==0,err.value
    assert lib.gc_package_state(ctypes.byref(pack))==1
    current=module.Rpf(archive);changes=[]
    for i in range(pack.count):
        item=pack.files[i]
        if item.mode==b'RPF':
            found=next(e for e in current.entries if e['path']==item.entry.decode())
            assert hashlib.sha256(current.extract(found)).hexdigest()==item.hash.decode()
            assert found['stored_size']==0,'Nested RPF must remain directly seekable'
            changes.append((found['record_offset'],found['record_offset']+16))
        else:
            assert module.sha(game/item.target.decode())==item.hash.decode()
    # Every pre-existing byte outside the 16 selected TOC records is preserved.
    with a.archive.open('rb') as left,archive.open('rb') as right:
        position=0
        while block:=left.read(4*1024*1024):
            other=bytearray(right.read(len(block)));expected=bytearray(block)
            for start,end in changes:
                low=max(start,position)-position;high=min(end,position+len(block))-position
                if high>low:other[low:high]=expected[low:high]
            assert other==expected,('Unexpected archive mutation',position)
            position+=len(block)
    assert lib.gc_package_install(ctypes.byref(pack),None,None,err,512)==0,err.value
    assert lib.gc_package_restore(ctypes.byref(pack),err,512)==0,err.value
    assert module.sha(archive)==oldsha
    assert not (game/'exefs/subsdk4').exists() and not (game/'exefs/main.npdm').exists()
    assert module.sha(a.archive)==oldsha,'Input was changed'
    report={'passed':True,'archive_bytes':a.archive.stat().st_size,'archive_sha256':oldsha,
            'patched_entries':len(changes),'unrelated_bytes_preserved':True,
            'byte_exact_restore':True,'repeat_install_safe':True,'console_verified':False}
    (out/'real-self-radio-tests.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':
    p=argparse.ArgumentParser()
    for key in ('output','zlib-source','archive','package','rpf-reader'):p.add_argument('--'+key,type=Path,required=True)
    main(p.parse_args())
