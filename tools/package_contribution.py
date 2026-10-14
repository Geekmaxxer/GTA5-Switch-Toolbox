"""Create a source ZIP and checked patch against a preserved source snapshot.

No remote repository, installed game or existing build is modified. The work
directory holds disposable baseline/patch-check copies for the reviewer audit.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile
from pathlib import Path


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def sources(root):
    selected = [root / name for name in
                ('.gitignore', 'Makefile', 'icon.jpg', 'README.md', 'README.en.md')]
    for folder in ('source', 'romfs', 'docs', 'tools', 'tests'):
        for path in sorted((root / folder).rglob('*')):
            if not path.is_file() or '__pycache__' in path.parts:
                continue
            if folder == 'romfs' or path.suffix.lower() in ('.c', '.h', '.inc', '.py', '.ps1', '.md', '.json'):
                selected.append(path)
    return {path.relative_to(root).as_posix(): path for path in selected}


def git(folder, *args):
    return subprocess.run(['git', '-c', 'core.autocrlf=false', '-c', 'core.safecrlf=false',
                           '-c', 'core.hooksPath=NUL', '-C', str(folder), *args],
                          check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout


def write_zip(destination, files):
    with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, path in sorted(files.items()):
            archive.write(path, name)
    with zipfile.ZipFile(destination) as archive:
        if archive.testzip() is not None or set(archive.namelist()) != set(files):
            raise ValueError('Archive verification failed')
        for name, path in files.items():
            if hashlib.sha256(archive.read(name)).hexdigest() != sha(path):
                raise ValueError('Archived source differs: ' + name)


def main(args):
    root = args.source.resolve()
    baseline = json.loads(args.baseline.read_text(encoding='utf-8-sig'))
    current = sources(root)
    # Validate all old files before creating output. A missing baseline asset is
    # accepted from current source only when its exact original hash still matches.
    originals = {}
    for name, record in baseline.items():
        if Path(name).is_absolute() or '..' in Path(name).parts or ':' in name:
            raise ValueError('Unsafe baseline path')
        original = args.baseline.parent / 'source-before' / name
        if not original.is_file():
            original = root / name
        if sha(original) != record['sha256']:
            raise ValueError('Missing or changed original snapshot: ' + name)
        if name not in current:
            raise ValueError('Original source missing from contribution: ' + name)
        originals[name] = original
    if args.output.exists() or args.work.exists():
        raise FileExistsError('Use new output and work directories')
    args.output.mkdir(parents=True)
    args.work.mkdir(parents=True)
    repository = args.work / 'patch-build'
    check = args.work / 'patch-check'
    for folder in (repository, check):
        folder.mkdir()
        for name, original in originals.items():
            destination = folder / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(original, destination)
        git(folder, 'init', '--quiet')
    git(repository, 'add', '--all')
    git(repository, '-c', 'user.name=Contribution packager',
        '-c', 'user.email=packager@localhost', 'commit', '--quiet', '-m', 'Original source snapshot')
    for name, path in current.items():
        destination = repository / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, destination)
    git(repository, 'add', '--all')
    patch = args.output / 'contribution.patch'
    patch.write_bytes(git(repository, 'diff', '--cached', '--binary', '--no-ext-diff', 'HEAD'))
    git(check, 'apply', '--check', '--binary', str(patch.resolve()))
    git(check, 'apply', '--binary', str(patch.resolve()))
    for name, path in current.items():
        if sha(check / name) != sha(path):
            raise ValueError('Applied patch differs: ' + name)
    source_zip = args.output / 'GTA5-Switch-Toolbox-Source.zip'
    write_zip(source_zip, {'GTA5-Switch-Toolbox/' + name: path for name, path in current.items()})
    changed = sorted(name for name, path in current.items()
                     if name in baseline and sha(path) != baseline[name]['sha256'])
    added = sorted(set(current) - set(baseline))
    report = {'baseline_files': len(baseline), 'source_files': len(current),
              'changed_files': changed, 'added_files': added,
              'patch_apply_check': True, 'patched_source_byte_exact': True,
              'source_sha256': {name: sha(path) for name, path in sorted(current.items())},
              'source_zip_sha256': sha(source_zip), 'patch_sha256': sha(patch),
              'remote_published': False}
    verification = args.output / 'source-verification.json'
    verification.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    baseline_copy = args.output / 'baseline-manifest.json'
    shutil.copy2(args.baseline, baseline_copy)
    bundle = {'contribution.patch': patch, source_zip.name: source_zip,
              verification.name: verification, baseline_copy.name: baseline_copy}
    for name in ('CONTRIBUTION.en.md', 'CONTRIBUTION.zh.md', 'RELEASE-VALIDATION.json'):
        path = root / 'docs' / name
        if not path.is_file():
            raise FileNotFoundError(path)
        bundle[name] = path
    contribution_zip = args.output / 'GTA5-Switch-Toolbox-Contribution.zip'
    write_zip(contribution_zip, bundle)
    report['contribution_zip_sha256'] = sha(contribution_zip)
    # The archive's record intentionally omits its own hash; the external record includes it.
    (args.output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'source_files': len(current), 'changed': len(changed), 'added': len(added),
                      'patch_apply_check': True, 'contribution_zip': str(contribution_zip)}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    main(parser.parse_args())
