"""Stage the movable Duel1/Lotus Premium+Exclusive edition and native runtime."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil

from export_local_server_patch import ROOT, copy, digest, json_file, command


def stage(destination,bundle):
    record=json.loads(bundle.with_name(bundle.name+'.json').read_text(encoding='utf-8'))
    manifest_path=bundle.with_name(bundle.name+'.manifest.json')
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    if bundle.stat().st_size!=record['bytes'] or digest(manifest_path)!=record['manifest_sha256']:
        raise RuntimeError('Dependency bundle/manifest differs from its export record')
    scope=manifest['scope']
    donor=ROOT/'exports/ValorantTrainer-Patch-20261108-r1'
    version=json.loads((donor/'patch-version.json').read_text(encoding='utf-8'))
    if destination.exists():
        raise RuntimeError('Standalone output already exists')
    destination.mkdir(parents=True)
    inventory=json.loads((donor/'package-sha256.json').read_text(encoding='utf-8'))
    for name,expected in inventory.items():
        path=donor/name
        if digest(path)!=expected:
            raise RuntimeError('Verified patch changed: '+name)
        if name.startswith('reports/training-map-images/'):
            continue
        copy(path,destination/name)
    for name in ('trainer_patch.py','trainer_dependencies.py','resource_bundle.py'):
        copy(ROOT/'tools'/name,destination/'tools'/name)
    from offline_gi_instance import verified_build as startup_build
    startup=startup_build()
    for name in startup['sha256']:
        copy(ROOT/name,destination/name)
    copy(ROOT/'.local/offline-gi-instance/build.json',destination/'.local/offline-gi-instance/build.json')
    copy(manifest_path,destination/'profiles/standalone-dependencies.json')
    skins=json.loads((ROOT/'reports/all-skins.json').read_text(encoding='utf-8'))
    rows=list(skins['skins'].values()) if isinstance(skins['skins'],dict) else skins['skins']
    selected=set(scope['skin_ids'])
    skins['skins']=[row for row in rows if row['guid'].replace('-','') in selected]
    wanted=set(selected)
    for row in skins['skins']:
        wanted.update(item['guid'].replace('-','') for item in (*row['levels'],*row['chromas']) if isinstance(item.get('guid'),str))
    skins['assets']=[row for row in skins['assets'] if row['guid'].replace('-','') in wanted]
    skins['edition_filter']={'content_tiers':['Premium','Exclusive'],'base_weapons':True,
                             'skin_count':len(skins['skins']),'tier_source':scope['tier_source']}
    json_file(destination/'reports/all-skins.json',skins)
    maps=json.loads((ROOT/'reports/training-maps.json').read_text(encoding='utf-8'))
    maps['maps']=[row for row in maps['maps'] if row['id'] in scope['maps']]
    for row in maps['maps']:
        name=row.get('minimap',{}).get('relative_path')
        if name:
            copy(ROOT/name,destination/name)
    json_file(destination/'reports/training-maps.json',maps)
    settings=json.loads((donor/'profiles/patch-default-settings.json').read_text(encoding='utf-8'))
    settings['map_id']='jam'
    settings['routes']={key:rows for key,rows in settings['routes'].items() if key in scope['maps']}
    settings['skins']={weapon:value for weapon,value in settings['skins'].items() if value['skin_id'] in selected}
    json_file(destination/'profiles/patch-default-settings.json',settings)
    project=json.loads((donor/'profiles/patch-project.json').read_text(encoding='utf-8'))
    project.update(name='ValorantTrainer-Standalone-Duel1-Lotus-Premium-Exclusive',
                   resource_mode='standalone-original-ranges-v1')
    json_file(destination/'profiles/patch-project.json',project)
    loose=json.loads((donor/'profiles/patch-installation-manifest.json').read_text(encoding='utf-8'))['loose_files']
    for item in loose:
        source=ROOT/'client'/item['file']
        if digest(source)!=item['sha256']:
            raise RuntimeError('Pinned runtime file changed: '+item['file'])
        copy(source,destination/'client'/item['file'])
    version.update(kind='local-server-standalone-lite-v1',created_utc=datetime.now(timezone.utc).isoformat(),
                   startup_module_sha256=startup['sha256']['.local/offline-gi-instance/offline_gi_instance.dll'],
                   dependency_file=bundle.name,dependency_sha256=record['sha256'],dependency_bytes=record['bytes'],
                   dependency_manifest_sha256=record['manifest_sha256'],game_resources_bundled=True,
                   installed_game_required=False,maps=scope['maps'],skin_tier_counts=scope['skin_tier_counts'],
                   skin_count=scope['skin_count'],hero_count=scope['hero_count'],
                   standalone_native_tested=False,separate_device_tested=False)
    version.pop('reused_resource_bytes',None)
    version.pop('relocated_native_active_seconds',None)
    version.pop('validation_report',None)
    version['relocated_package_tested']=False
    json_file(destination/'patch-version.json',version)
    command(destination/'启动训练器.cmd')
    command(destination/'检查环境.cmd','--check',pause=True)
    command(destination/'导出反馈日志.cmd','--logs',pause=True)
    (destination/'使用说明.txt').write_text(
        '无畏契约离线训练器：免安装精简资源版\n\n'
        '文件准备\n'
        '1. 完整解压启动包到空间充足的 NTFS 磁盘，建议使用非系统盘。\n'
        '2. 将 '+bundle.name+' 放进解压后的训练器文件夹，与“启动训练器.cmd”并列。\n'
        '3. 双击“启动训练器.cmd”。首次会校验并准备资源，需要几分钟；随后打开 WebUI。\n'
        '4. 在 http://127.0.0.1:8792 点击开始。结束时先在 WebUI 停止游戏，再关闭启动窗口。\n\n'
        '本版内容\n'
        '斗牛一与莲花古城；全部基础武器；本地资源中所有紫色 Premium、橙色 Exclusive 品质皮肤，\n'
        '包含这些皮肤的等级与炫彩，以及 30 个可选英雄的默认人物资源。\n'
        '范围按原 DataAsset.content_tier 字段确定：477 款橙色、163 款紫色，另含 21 款基础外观。\n'
        '未包含的地图和皮肤不在本版 WebUI 选择列表中。默认使用莲花古城的人机乱斗。\n\n'
        '无需国际服安装、账号登录、系统 Python 或编译器。设置、缓存和日志保存在本包 .local。\n'
        '原资源索引、签名和已选压缩块均保留原字节；运行时仍执行原校验。\n'
        '首次准备按原偏移写入 NTFS 稀疏文件，Windows 显示的“大小”不等于实际“占用空间”。\n'
        '建议首次配置预留 30 GB 可用空间，另加下载资源文件本身占用的空间。\n'
        '资源准备完成后可将 .vtr 文件移到其他磁盘保存；正常运行不再依赖它，修复资源时需要。\n\n'
        '测试版说明\n'
        '这是当前开发版，英雄技能、人机行为及部分外观表现仍有未完善之处。\n'
        '原品质定义参考：https://support.riotgames.com/en-us/valorant/store/price-tiers-for-skins-in-valorant\n'
        '发生问题时运行“检查环境.cmd”或“导出反馈日志.cmd”。\n',encoding='utf-8-sig')
    hashes={p.relative_to(destination).as_posix():digest(p) for p in sorted(destination.rglob('*'))
            if p.is_file() and p.name!='package-sha256.json'}
    json_file(destination/'package-sha256.json',hashes)
    print(json.dumps({'directory':str(destination),'files':len(hashes),'resource_bundle':bundle.name,
                      'scope':{key:scope[key] for key in ('maps','skin_count','skin_tier_counts','hero_count')}},ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--dependencies',type=Path,required=True)
    args=parser.parse_args()
    stage(args.output.resolve(),args.dependencies.resolve(strict=True))
