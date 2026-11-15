# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['F:/valoranttrainer/sound_locator/map_app.py'],
    pathex=[],
    binaries=[],
    datas=[('F:/valoranttrainer/sound_locator/maps/ascent.json', 'maps'), ('F:/valoranttrainer/sound_locator/maps/ascent.png', 'maps'), ('F:/valoranttrainer/.local/sound-map-assets/models/ascent-profile.npz', 'models'), ('F:/valoranttrainer/.local/sound-map-assets/models/ascent-neural.npz', 'models'), ('F:/valoranttrainer/.local/sound-map-assets/models/ascent-range.npz', 'models')],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=['torch', 'matplotlib', 'pandas', 'sklearn', 'pytest'],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='SoundMapLocator',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='SoundMapLocator',
)
