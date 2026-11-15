"""Build and verify a self-contained Duel 1 package from original resource ranges."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import uuid
import zipfile

from client_layout import CLIENT_IMAGE, is_portable
from export_playtest import ROOT, copy_file, runtime_tools, stage_export
from resource_bundle import MANIFEST


def hash_file(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def prepare_resources(destination: Path, plan_path: Path, profile: str) -> dict:
    destination = destination.resolve()
    config = json.loads((destination / 'project.json').read_text(encoding='utf-8'))
    if not is_portable(config):
        raise RuntimeError('Resources may be staged only into a portable package')
    plan = json.loads(plan_path.read_text(encoding='utf-8'))
    original_config = json.loads((ROOT / 'project.json').read_text(encoding='utf-8'))
    source = Path(original_config['game_executable']).resolve().parents[3]
    if source != Path(plan['source_root']).resolve() or source == destination or source in destination.parents:
        raise RuntimeError('Resource plan/source/destination identity mismatch')
    if hash_file(Path(original_config['game_executable'])) != config['expected_snapshot_sha256']:
        raise RuntimeError('Source client image changed')
    selected = plan['profiles'][profile]['offset_preserving_sparse_candidate']
    # Explicit runtime directories; no Saved trees, installed launchers, account
    # settings, developer manifests, crash dumps or unrelated source tools.
    directories = ('ShooterGame/Binaries', 'Engine/Binaries', 'Engine/Content')
    loose_bytes = 0
    for directory in directories:
        for item in sorted((source / directory).rglob('*')):
            if not item.is_file() or any(part.casefold() in ('saved', 'logs', 'crashes', '__pycache__')
                                         for part in item.relative_to(source).parts):
                continue
            if item.suffix.casefold() in ('.dmp', '.log', '.pdb'):
                continue
            if item.name.casefold().startswith('crashreportclient'):
                continue  # Local diagnostics are retained; no upload executable.
            copy_file(item, destination / 'client' / item.relative_to(source))
            loose_bytes += item.stat().st_size
    paks = source / 'ShooterGame/Content/Paks'
    paks_out = destination / 'client/ShooterGame/Content/Paks'
    paks_out.mkdir(parents=True, exist_ok=True)
    for item in sorted(paks.iterdir()):
        if item.is_file() and item.suffix.casefold() in ('.utoc', '.pak', '.sig'):
            copy_file(item, paks_out / item.name)
            loose_bytes += item.stat().st_size
    if hash_file(destination / CLIENT_IMAGE) != config['expected_snapshot_sha256']:
        raise RuntimeError('Copied client image verification failed')
    print(json.dumps({'loose_files_copied_bytes': loose_bytes}), flush=True)
    sys.path.insert(0, str(ROOT / 'work/extract'))
    from iostore import IoStore
    manifest = {'schema_version': 1, 'profile': profile, 'files': [],
                'original_tocs_and_signatures_unchanged': True,
                'original_block_sha1_verified': True,
                'toc_public_key_signature_authentication_performed_by_exporter': False,
                'source_plan_sha256': hash_file(plan_path)}
    for item in selected['manifest']:
        io = IoStore(paks / item['toc'])
        if (hashlib.sha256(io.data).hexdigest() != item['toc_sha256'] or io.partition_count != 1
                or not io.block_hashes or io.path.with_suffix('.ucas').stat().st_size != item['original_logical_file_bytes']):
            raise RuntimeError('Original signed resource layout differs from its plan')
        block_ids = [index for first, last in item['block_index_intervals_inclusive'] for index in range(first, last + 1)]
        blocks = sorted((io.blocks[index][0], (io.blocks[index][1] + 15) // 16 * 16, index) for index in block_ids)
        block_index = 0
        payload_path = destination / 'resources/blocks' / (item['file'] + '.bin')
        payload_path.parent.mkdir(parents=True, exist_ok=True)
        if payload_path.resolve() != payload_path or (payload_path.exists() and payload_path.stat().st_nlink != 1):
            raise RuntimeError('Resource payload destination is not package-owned')
        temporary_payload = payload_path.with_name(payload_path.name + '.' + uuid.uuid4().hex + '.tmp')
        record = {'file': item['file'], 'toc': item['toc'], 'toc_sha256': item['toc_sha256'],
                  'logical_bytes': item['original_logical_file_bytes'], 'ranges': [],
                  'payload_bytes': item['original_stored_payload_bytes'],
                  'original_block_count': len(blocks)}
        with io.path.with_suffix('.ucas').open('rb') as incoming, temporary_payload.open('xb') as outgoing:
            for offset, size in item['physical_ranges_offset_length']:
                incoming.seek(offset)
                end = offset + size
                cursor = offset
                digest = hashlib.sha256()
                while cursor < end:
                    if block_index >= len(blocks):
                        raise RuntimeError('Incomplete selected-block coverage')
                    physical, length, index = blocks[block_index]
                    if cursor != physical or cursor + length > end:
                        raise RuntimeError('Physical range is not an exact original-block union')
                    stored = incoming.read(length)
                    if len(stored) != length or hashlib.sha1(stored).digest() != io.block_hashes[index]:
                        raise RuntimeError('Original compressed block SHA1 differs from unchanged TOC')
                    outgoing.write(stored)
                    digest.update(stored)
                    cursor += length
                    block_index += 1
                record['ranges'].append([offset, size, digest.hexdigest()])
        if block_index != len(blocks) or temporary_payload.stat().st_size != record['payload_bytes']:
            raise RuntimeError('Selected payload size/block coverage mismatch')
        os.replace(temporary_payload, payload_path)
        manifest['files'].append(record)
        print(json.dumps({'file': record['file'], 'selected_bytes': record['payload_bytes'],
                          'blocks_sha1_verified': len(blocks)}), flush=True)
    manifest_path = destination / MANIFEST
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    config['resources_manifest_sha256'] = hash_file(manifest_path)
    (destination / 'project.json').write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding='utf-8')
    for item in runtime_tools():
        copy_file(item, destination / 'tools' / item.name)
    version_path = destination / 'playtest-version.json'
    version = json.loads(version_path.read_text(encoding='utf-8'))
    version.update({'game_resources_bundled': True, 'resource_profile': profile,
                    'resource_payload_bytes': sum(item['payload_bytes'] for item in manifest['files']),
                    'loose_runtime_files_bytes': loose_bytes, 'ntfs_sparse_files_required': True,
                    'standalone_resource_validation_complete': False})
    version_path.write_text(json.dumps(version, indent=2), encoding='utf-8')
    readme = destination / '测试说明.txt'
    text = readme.read_text(encoding='utf-8-sig')
    text = text.split('\n资源说明\n', 1)[0].rstrip() + '\n'
    text = text.replace('首次加载需等待约 10 秒。', '请解压到 NTFS 磁盘；首次启动会准备本地资源，需等待数分钟。')
    text = text.replace('需等待约 1–2 分钟。', '需等待数分钟。')
    text += ('\n资源说明\n只带当前训练场功能的依赖数据，保留原资源索引和校验。\n'
             '首次启动会生成稀疏资源文件：文件的逻辑大小较大，以“占用空间”查看实际磁盘使用量。\n'
             '请始终保留 resources 文件夹，方便资源校验和恢复；转发时请发送原 ZIP。\n')
    readme.write_text(text, encoding='utf-8-sig')
    return {'resource_payload_bytes': version['resource_payload_bytes'], 'loose_bytes': loose_bytes,
            'manifest_sha256': config['resources_manifest_sha256'], 'profile': profile}


def package_files(destination: Path):
    """Keep fresh shipping data; omit QA sessions, generated sparse files and caches."""
    shipped_tools = {path.name for path in runtime_tools()}
    top_files = {'project.json', 'playtest-version.json', '测试说明.txt', '斗牛1训练器.exe',
                 '诊断启动.cmd', '检查环境.cmd', '导出测试日志.cmd', '退出训练器.cmd'}
    reports = {'native-config-validation.json', 'playtest-native-check.json', 'portable-validation.json'}
    startup_sources = {'native/offline_gi_instance.cpp', 'native/build_offline_gi_instance.cmd'}
    for path in sorted(destination.rglob('*')):
        if not path.is_file():
            continue
        relative = path.relative_to(destination)
        name = relative.as_posix()
        if len(relative.parts) == 1 and path.name not in top_files:
            continue
        if relative.parts[0] not in top_files | {'tools', 'runtime', 'reports', 'resources', 'client', '.local', 'native'}:
            continue
        if relative.parts[0] == 'native' and name not in startup_sources:
            continue
        if '__pycache__' in relative.parts or path.suffix == '.pyc':
            continue
        if relative.parts[0] == 'tools' and path.name not in shipped_tools:
            continue
        if relative.parts[0] == 'resources' and name != MANIFEST and not (
                relative.parent.as_posix() == 'resources/blocks' and path.name.endswith('.ucas.bin')):
            continue
        if relative.parts[0] == 'reports' and (len(relative.parts) != 2 or path.name not in reports):
            continue
        if relative.parts[0] == 'client' and (any(part.casefold() in ('saved', 'logs', 'crashes') for part in relative.parts)
                                              or path.suffix.casefold() in ('.dmp', '.log', '.pdb')):
            continue
        if name.startswith('client/Engine/Programs/') or path.name.casefold().startswith('crashreportclient'):
            continue
        if name.startswith('client/ShooterGame/Content/Paks/') and path.suffix == '.ucas':
            continue
        if relative.parts[0] == '.local' and not (
                name.startswith('.local/native-module/source-snapshot/') or
                name in ('.local/native-module/trainer_bridge.dll', '.local/native-module/build.json',
                         '.local/native-module/trainer_bridge.map',
                         '.local/offline-gi-instance/build.json', '.local/offline-gi-instance/offline_gi_instance.dll',
                         '.local/native-baseline/portable-core/core-config.json')):
            continue
        if relative.parts[0] == 'logs' or name == 'package-sha256.json':
            continue
        yield path


def archive_package(destination: Path, archive: Path) -> dict:
    version = json.loads((destination / 'playtest-version.json').read_text(encoding='utf-8'))
    if not version.get('standalone_resource_validation_complete'):
        raise RuntimeError('Standalone resource validation must pass before creating a shareable archive')
    config_path = destination / 'project.json'
    config = json.loads(config_path.read_text(encoding='utf-8'))
    proof = json.loads((destination / 'reports/portable-validation.json').read_text(encoding='utf-8'))
    if (proof.get('passed') is not True or proof.get('resources_manifest_sha256') != config.get('resources_manifest_sha256')
            or proof.get('module_sha256') != version['module_sha256']):
        raise RuntimeError('Native validation belongs to another resource manifest or module')
    manifest_path = destination / MANIFEST
    if hash_file(manifest_path) != config['resources_manifest_sha256']:
        raise RuntimeError('Portable resource manifest changed after validation')
    for item in json.loads(manifest_path.read_text(encoding='utf-8'))['files']:
        packed = destination / 'resources/blocks' / (item['file'] + '.bin')
        if not packed.is_file() or packed.stat().st_size != item['payload_bytes']:
            raise RuntimeError('All resource payloads must be present before archive creation')
    config['discard_resource_payload_after_prepare'] = True
    config_path.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding='utf-8')
    readme = destination / '测试说明.txt'
    text = readme.read_text(encoding='utf-8-sig')
    text = text.replace('请始终保留 resources 文件夹，方便资源校验和恢复；转发时请发送原 ZIP。',
                        '首次准备成功后会自动清理提取用的中间数据，减少磁盘占用；转发时请发送原 ZIP。\n'
                        '如资源损坏或缺失，请从原 ZIP 重新解压；请保留 resources 中的资源清单。')
    readme.write_text(text, encoding='utf-8-sig')
    files = list(package_files(destination))
    hashes = {path.relative_to(destination).as_posix(): hash_file(path) for path in files}
    inventory = destination / 'package-sha256.json'
    inventory.write_text(json.dumps(hashes, indent=2), encoding='utf-8')
    files.append(inventory)
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED, compresslevel=1, allowZip64=True) as output:
        for path in files:
            relative = path.relative_to(destination)
            # These bytes are already compressed by the original container.
            method = zipfile.ZIP_STORED if path.suffix == '.bin' or path.suffix == '.pak' else zipfile.ZIP_DEFLATED
            output.write(path, archive.stem + '/' + relative.as_posix(), compress_type=method)
    return {'archive': str(archive), 'archive_bytes': archive.stat().st_size,
            'files': len(files), 'package_payload_bytes': sum(p.stat().st_size for p in files),
            'archive_sha256': hash_file(archive)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--plan', type=Path, default=ROOT / 'reports/portable-resource-plan.json')
    parser.add_argument('--profile', default='catalog_guarded_hard_closure')
    parser.add_argument('--resources-only', action='store_true')
    parser.add_argument('--archive-only', type=Path)
    args = parser.parse_args()
    if args.archive_only:
        result = archive_package(args.output.resolve(), args.archive_only.resolve())
    else:
        if not args.resources_only:
            stage_export(args.output.resolve(), portable=True)
        result = prepare_resources(args.output.resolve(), args.plan.resolve(), args.profile)
    print(json.dumps(result, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
