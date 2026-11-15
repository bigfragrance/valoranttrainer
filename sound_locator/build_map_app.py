"""Build the standalone map UI without touching the first SoundLocator package."""
from pathlib import Path
import subprocess
import sys


def main():
    source=Path(__file__).resolve().parent
    root=source.parent
    data=[(source/"maps/ascent.json","maps"),(source/"maps/ascent.png","maps"),
          (source/"validation/ascent-footstep-baseline/profile.npz","models"),
          (source/"validation/learned-ascent-onset/learned-model.npz","models")]
    # Stage explicit names because two independently versioned model formats are used.
    import shutil
    staging=root/".local/sound-map-assets"
    (staging/"models").mkdir(parents=True,exist_ok=True)
    shutil.copy2(data[2][0],staging/"models/ascent-profile.npz")
    shutil.copy2(data[3][0],staging/"models/ascent-neural.npz")
    command=[sys.executable,"-m","PyInstaller","--noconfirm","--windowed","--onedir",
             "--name","SoundMapLocator","--distpath",str(root/"outputs"),
             "--workpath",str(root/".local/sound-map-build"),"--specpath",str(source),
             "--exclude-module","torch","--exclude-module","matplotlib","--exclude-module","pandas",
             "--exclude-module","sklearn","--exclude-module","pytest"]
    assets=data[:2]+[(staging/"models/ascent-profile.npz","models"),(staging/"models/ascent-neural.npz","models")]
    optional_range=source/"validation/range-ascent/range-model.npz"
    if optional_range.is_file():
        shutil.copy2(optional_range,staging/"models/ascent-range.npz")
        assets.append((staging/"models/ascent-range.npz","models"))
    for path,dest in assets:
        if not path.is_file():raise FileNotFoundError(path)
        command.extend(["--add-data",f"{path};{dest}"])
    command.append(str(source/"map_app.py"))
    return subprocess.call(command,cwd=root)


if __name__=="__main__":raise SystemExit(main())
