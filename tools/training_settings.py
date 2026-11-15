"""Persistent controls for the local game and its HUD; no account settings."""
from pathlib import Path
import json
import math

ROOT = Path(__file__).resolve().parents[1]
SETTINGS_PATH = ROOT / '.local/trainer-settings.json'
DEFAULTS = {'sensitivity': .35, 'fov': 103., 'enemy_outline': True,
            'hud_scale': 1., 'crosshair_size': 6., 'crosshair_gap': 5.,
            'minimap': True, 'skill_hints': True}
RANGES = {'sensitivity': (.05, 2.), 'fov': (80., 120.), 'hud_scale': (.7, 1.4),
          'crosshair_size': (1., 16.), 'crosshair_gap': (0., 16.)}

def normalize(value):
    result = dict(DEFAULTS)
    if not isinstance(value, dict):
        return result
    for name, default in DEFAULTS.items():
        item = value.get(name, default)
        if isinstance(default, bool):
            if isinstance(item, bool): result[name] = item
        elif isinstance(item, (int, float)) and not isinstance(item, bool) and math.isfinite(item):
            low, high = RANGES[name]
            result[name] = max(low, min(high, float(item)))
    return result

def load(path=SETTINGS_PATH):
    try:
        return normalize(json.loads(path.read_text(encoding='utf-8')))
    except (OSError, ValueError):
        return dict(DEFAULTS)

def save(value, path=SETTINGS_PATH):
    settings = normalize(value)
    path.parent.mkdir(parents=True, exist_ok=True)
    pending = path.with_suffix('.pending')
    pending.write_text(json.dumps(settings, ensure_ascii=False, indent=2), encoding='utf-8')
    pending.replace(path)
    return settings
