"""Drive command20 only in an explicitly owned, ready stage9 Ascent session."""
from pathlib import Path
import argparse
import json
import shutil
import struct
import sys
import time
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from training_runtime_client import OwnedSession
from training_control import COMMANDS
COMMANDS["sound_locator_nav_export"]=20


def main():
    p=argparse.ArgumentParser();p.add_argument("active");p.add_argument("--spacing",type=float,default=200)
    p.add_argument("--out",type=Path,default=ROOT/"sound_locator/maps/ascent-nav-runtime.json")
    p.add_argument("--timeout",type=float,default=600);a=p.parse_args()
    raw=ROOT/".local/native-module/ascent-nav-export.json"
    started=time.monotonic();last_print=0
    with OwnedSession(a.active) as session:
        session.wait(lambda s:s.get("ready"))
        if session.active.get("module_stage")!=9:raise RuntimeError("Explicit stage9 session required")
        print(session.command("sound_locator_nav_export",struct.pack("<IId",0,128,a.spacing)),flush=True)
        while time.monotonic()-started<a.timeout:
            result=session.command("sound_locator_nav_export",struct.pack("<IId",1,128,a.spacing),timeout=8)
            if time.monotonic()-last_print>5:
                print(result["message"],flush=True);last_print=time.monotonic()
            if "phase_2_done_1" in result["message"]:
                data=json.loads(raw.read_text(encoding="utf-8"))
                if not data.get("complete") or data.get("pid")!=session.active["pid"]:raise RuntimeError("Foreign/incomplete navigation export")
                a.out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(raw,a.out)
                shutil.copyfile(ROOT/".local/native-module/build.json",a.out.parent/"ascent-nav-build.json")
                print(json.dumps({"out":str(a.out),"seconds":time.monotonic()-started,"nodes":len(data["nodes_world_cm"]),"paths":len(data["path_chains_cm"])}),flush=True)
                return
            time.sleep(.01)
        session.command("sound_locator_nav_export",struct.pack("<IId",2,128,a.spacing))
        raise TimeoutError("Navigation export incomplete; partial receipt retained")


if __name__=="__main__":main()
