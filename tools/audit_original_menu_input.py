"""Read-only audit of owned menu-open/close snapshots; never launches/input/UI."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path


def read(path: str):
    p=Path(path)
    data=p.read_bytes()
    return json.loads(data),{'path':str(p.resolve()),'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)}


def audit(args):
    opened,p1=read(args.open_ui);closed,p2=read(args.closed_ui)
    checks={
      'same_local_controller':bool(opened.get('controller')) and opened.get('controller')==closed.get('controller'),
      'original_menu_observed_open':any(opened.get(k) for k in ['options_open','shop_open','map_open','external_observed_open']),
      'open_original_game_and_ui_dispatch':opened.get('viewport_mode')=='game_and_ui' and opened.get('game_and_ui_applied') is True,
      'open_actual_cursor_visible':opened.get('cursor_visible') is True,
      'open_gameplay_blocked':opened.get('block_gameplay') is True,
      'close_request_completed':closed.get('close_requests',0)>0 and closed.get('close_observed') is True,
      'actual_menus_closed':not any(closed.get(k) for k in ['options_open','shop_open','map_open','external_observed_open']),
      'closed_original_game_only_dispatch':closed.get('viewport_mode')=='game_only' and closed.get('game_and_ui_applied') is False,
      'closed_actual_cursor_hidden':closed.get('cursor_visible') is False,
      'closed_gameplay_restored':closed.get('block_gameplay') is False,
      'module_errors_zero':opened.get('error')==closed.get('error')==0,
      'no_module_exception':not opened.get('restart_required') and not closed.get('restart_required')}
    focus=bool(opened.get('widget_has_focus') or opened.get('widget_has_focused_descendants'))
    if args.require_focus:checks['actual_open_widget_or_child_keyboard_focus']=focus
    proof=[p1,p2];notes=[]
    if not focus:notes.append('No focus claimed from dispatch alone. Use a snapshot after actual original widget focus/click, or investigate this negative readback.')
    if args.shop_before or args.shop_after:
        if not(args.shop_before and args.shop_after):raise ValueError('Both shop snapshots are required')
        before,p3=read(args.shop_before);after,p4=read(args.shop_after);proof.extend([p3,p4])
        checks['same_shop_controller']=before.get('graph',{}).get('controller')==after.get('graph',{}).get('controller')
        checks['original_selection_event_increased']=after.get('selections',0)>before.get('selections',0)
        checks['local_equip_request_accepted']=after.get('requests_accepted',0)>before.get('requests_accepted',0)
        notes.append('Selection/request acceptance does not prove completed equip; inspect the original current weapon GUID/lifecycle separately.')
    if args.view_before or args.view_after:
        if not(args.view_before and args.view_after):raise ValueError('Both original view snapshots are required')
        a,p3=read(args.view_before);b,p4=read(args.view_after);proof.extend([p3,p4])
        ra,rb=a.get('rotation'),b.get('rotation')
        valid=isinstance(ra,list) and isinstance(rb,list) and len(ra)==len(rb)==3 and all(isinstance(v,(int,float)) and math.isfinite(v) for v in ra+rb)
        checks['actual_view_changed_after_close']=valid and max(abs((float(x)-float(y)+180)%360-180) for x,y in zip(ra,rb))>0.1
    return {'schema_version':1,'passed':all(checks.values()),'checks':checks,
            'keyboard_focus_observed':focus,'proof_files':proof,'notes':notes,
            'runtime_launched':False,'physical_or_UI_input_sent':False,
            'coverage':'Archived native state readbacks only; no mouse hit-test or rendered-pixel success is inferred.'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--open-ui',required=True)
    parser.add_argument('--closed-ui',required=True)
    parser.add_argument('--require-focus',action='store_true')
    parser.add_argument('--shop-before');parser.add_argument('--shop-after')
    parser.add_argument('--view-before');parser.add_argument('--view-after')
    parser.add_argument('--output')
    args=parser.parse_args();result=audit(args)
    text=json.dumps(result,ensure_ascii=False,indent=2)+'\n'
    if args.output:Path(args.output).write_text(text,encoding='utf-8')
    print(text,end='')
    raise SystemExit(0 if result['passed'] else 1)


if __name__=='__main__':main()
