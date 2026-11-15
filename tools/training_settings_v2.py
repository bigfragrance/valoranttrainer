"""Validated persistent configuration for practice and local free-for-all."""
from __future__ import annotations

import copy
import json
import math
import os
from pathlib import Path
import threading

from training_control import STYLES, ROUTE_STYLES
from training_heroes import DEFAULT_HERO_ID, hero_profile

def _unique_hero_limit():
    return _HERO_PROFILE['count']


# The selected local hero and each NPC consume one original hero for the match.
_HERO_PROFILE = hero_profile()
HERO_IDS = frozenset(row['id'] for row in _HERO_PROFILE['heroes'])
MAX_UNIQUE_PLAYERS = _unique_hero_limit()

DEFAULTS = {
    "schema_version": 2, "revision": 0, "mode": "practice", "map_id": "duel1",
    "local_hero_id": DEFAULT_HERO_ID,
    "total_players": 14, "invincible": False, "infinite_ammo": True, "paused": False,
    "respawn_seconds": 1.5, "streak_seconds": 5.0, "seed": 1,
    "practice": {"peeks_per_minute": 20.0, "style": "random", "return_fire": False,
                 "mode": "continuous", "minimum_interval": 1.0, "waves": {}, "restart_serial": 0},
    "bots": {"reaction_ms": 260.0, "reaction_spread_ms": 100.0,
             "aim_speed": 220.0, "aim_spread": 100.0,
             "accuracy": 0.65, "accuracy_spread": 0.22,
             "movement": 0.65, "movement_spread": 0.25},
    "weapon_id": None, "skins": {}, "effects": {}, "routes": {"duel1": [
        {"id": 1, "name": "默认左侧横移", "enabled": True, "weight": 1.0, "style": "global",
         "start": [-900.0, -300.0, 202.3], "expose": [-900.0, 0.0, 202.3], "retreat": [-900.0, -300.0, 202.3]},
        {"id": 2, "name": "默认右侧横移", "enabled": True, "weight": 1.0, "style": "global",
         "start": [-900.0, 300.0, 202.3], "expose": [-900.0, 0.0, 202.3], "retreat": [-900.0, 300.0, 202.3]},
    ]},
}


for _route in DEFAULTS['routes']['duel1']:
    _route.update(fire=False, delay_min=.1, delay_max=.4)


def _number(value, lo, hi, name, *, integer=False):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"{name} 必须是有效数字")
    if not lo <= value <= hi or integer and not isinstance(value, int):
        raise ValueError(f"{name} 范围为 {lo}–{hi}")
    return int(value) if integer else float(value)


def _keys(value, allowed, name):
    if not isinstance(value, dict) or set(value) - set(allowed):
        raise ValueError(f"{name} 包含未知字段")


def validate(config):
    _keys(config, DEFAULTS, "设置")
    if set(config) != set(DEFAULTS) or config["schema_version"] != 2:
        raise ValueError("设置格式不完整或版本不匹配")
    if config["mode"] not in ("practice", "deathmatch"):
        raise ValueError("未知训练模式")
    if not isinstance(config['local_hero_id'], str) or config['local_hero_id'] not in HERO_IDS:
        raise ValueError('所选英雄不在当前原资源目录中')
    if not isinstance(config["map_id"], str) or not config["map_id"].isalnum() or len(config["map_id"]) > 48:
        raise ValueError("无效地图")
    for key in ("invincible", "infinite_ammo", "paused"):
        if not isinstance(config[key], bool):
            raise ValueError(f"{key} 必须为开关")
    for key, lo, hi in (("revision", 0, 0x7fffffff), ("total_players", 2, MAX_UNIQUE_PLAYERS), ("seed", 1, 0xffffffff)):
        config[key] = _number(config[key], lo, hi, key, integer=True)
    for key, lo, hi in (("respawn_seconds", .2, 15), ("streak_seconds", .5, 30)):
        config[key] = _number(config[key], lo, hi, key)
    practice = config["practice"]
    _keys(practice, DEFAULTS["practice"], "架枪设置")
    for key in ("mode", "minimum_interval", "waves", "restart_serial"):
        practice.setdefault(key, copy.deepcopy(DEFAULTS['practice'][key]))
    _keys(practice, DEFAULTS["practice"], "架枪设置")
    if set(practice) != set(DEFAULTS["practice"]) or practice["style"] not in STYLES:
        raise ValueError("无效拉出方式")
    if not isinstance(practice["return_fire"], bool):
        raise ValueError("还击必须为开关")
    practice["peeks_per_minute"] = _number(practice["peeks_per_minute"], .5, 180, "拉出频率")
    if practice['mode'] not in ('continuous', 'waves'):
        raise ValueError('未知架点模式')
    practice['minimum_interval'] = _number(practice['minimum_interval'], 0, 120, '总体最小拉出间隔')
    bots = config["bots"]
    _keys(bots, DEFAULTS["bots"], "人机设置")
    if set(bots) != set(DEFAULTS["bots"]):
        raise ValueError("人机设置不完整")
    ranges = {"reaction_ms": (60, 2000), "reaction_spread_ms": (0, 1000),
              "aim_speed": (20, 1080), "aim_spread": (0, 800),
              "accuracy": (.02, 1), "accuracy_spread": (.01, .7),
              "movement": (.05, 1), "movement_spread": (.01, .7)}
    for key, (lo, hi) in ranges.items():
        bots[key] = _number(bots[key], lo, hi, key)
    if config["weapon_id"] is not None and not isinstance(config["weapon_id"], str):
        raise ValueError("无效武器")
    if not all(isinstance(config[key], dict) for key in ("skins", "effects", "routes")):
        raise ValueError("无效收藏或路线设置")
    if len(config["skins"]) > 64 or len(config["effects"]) > 128 or len(config["routes"]) > 48:
        raise ValueError("设置数量超出限制")
    for map_id, routes in config["routes"].items():
        if not isinstance(map_id, str) or not map_id.isalnum() or not isinstance(routes, list) or len(routes) > 64:
            raise ValueError("路线地图或数量无效")
        ids = set()
        for route in routes:
            if not isinstance(route, dict):
                raise ValueError("路线必须为对象")
            for key, value in dict(fire=practice['return_fire'], delay_min=.1, delay_max=.4).items():
                route.setdefault(key, value)
            _keys(route, ("id", "name", "start", "expose", "retreat", "style", "enabled", "weight", "fire", "delay_min", "delay_max"), "路线")
            if not {"id", "name", "start", "expose", "retreat", "style", "enabled", "weight"} <= set(route):
                raise ValueError("每条路线需要起点、露出点和撤回点")
            validate_enemy(route)
            route["id"] = _number(route["id"], 1, 0x7fffffff, "路线编号", integer=True)
            if route["id"] in ids:
                raise ValueError("路线编号重复")
            ids.add(route["id"])
            if not isinstance(route["name"], str) or not 1 <= len(route["name"].strip()) <= 60:
                raise ValueError("路线名称为 1–60 个字符")
            if route["style"] not in ROUTE_STYLES or not isinstance(route["enabled"], bool):
                raise ValueError("路线拉出方式无效")
            route["weight"] = _number(route["weight"], .1, 100, "路线权重")
            for point in ("start", "expose", "retreat"):
                coords = route[point]
                if not isinstance(coords, list) or len(coords) != 3:
                    raise ValueError("路线点需要三维坐标")
                route[point] = [_number(v, -1e7, 1e7, "坐标") for v in coords]
    practice['restart_serial'] = _number(practice['restart_serial'], 0, 0x7fffffff, '练习重开序号', integer=True)
    waves = practice['waves']
    if not isinstance(waves, dict) or len(waves) > 48:
        raise ValueError('波次地图无效')
    for map_id, sequence in waves.items():
        if map_id not in config['routes'] or not isinstance(sequence, list) or len(sequence) > 64:
            raise ValueError('波次地图或数量无效')
        route_ids = {r['id'] for r in config['routes'][map_id]}
        if sum(len(w) for w in sequence if isinstance(w, list)) > 64:
            raise ValueError('每张地图的波次敌人总数最多64')
        for wave in sequence:
            if not isinstance(wave, list) or not 1 <= len(wave) < config['total_players']:
                raise ValueError('每波敌人数需要小于总人数，且至少为1')
            for enemy in wave:
                _keys(enemy, ('route_id', 'style', 'fire', 'delay_min', 'delay_max'), '波次敌人')
                if set(enemy) != {'route_id', 'style', 'fire', 'delay_min', 'delay_max'} or type(enemy['route_id']) is not int or enemy['route_id'] not in route_ids:
                    raise ValueError('波次敌人需要有效拉出点')
                validate_enemy(enemy)
    for weapon, skin in config['skins'].items():
        from training_control import guid_words
        guid_words(weapon)
        _keys(skin, ('skin_id', 'level_id', 'chroma_id'), '外观选择')
        if set(skin) != {'skin_id', 'level_id', 'chroma_id'}:
            raise ValueError('外观选择需要皮肤、等级与炫彩')
        for value in skin.values():
            guid_words(value)
    for key, effect in config['effects'].items():
        if not isinstance(key, str) or not key.isascii() or not 0 < len(key) < 80 or not all(c.isalnum() or c == '_' for c in key):
            raise ValueError('无效特效编号')
        _keys(effect, ('mode', 'value'), '特效')
        if isinstance(effect.get('mode'), bool) or effect.get('mode') not in (0, 1, 2):
            raise ValueError('特效模式无效')
        effect['value'] = _number(effect.get('value', 0), -1e6, 1e6, '特效参数')
    return config


def validate_enemy(enemy):
    if enemy.get('style') not in ROUTE_STYLES or type(enemy.get('fire')) is not bool:
        raise ValueError('敌人拉出方式或开枪设置无效')
    for key in ('delay_min', 'delay_max'):
        enemy[key] = _number(enemy.get(key), 0, 120, '拉出时间')
    if enemy['delay_min'] > enemy['delay_max']:
        raise ValueError('拉出时间下限不能大于上限')


def merge(current, patch):
    _keys(patch, set(DEFAULTS) - {"schema_version", "revision"}, "修改")
    result = copy.deepcopy(current)
    for key, value in patch.items():
        if key in ("practice", "bots"):
            _keys(value, DEFAULTS[key], key)
            result[key].update(value)
        else:
            result[key] = copy.deepcopy(value)
    result["revision"] += 1
    return validate(result)


class Settings:
    def __init__(self, path: Path):
        self.path = Path(path)
        self.lock = threading.RLock()
        try:
            saved = json.loads(self.path.read_text(encoding="utf-8"))
            # Compatible in-memory migration; merely opening an old preference
            # store does not rewrite it. The next explicit save persists Jett.
            if isinstance(saved, dict) and saved.get('schema_version') == 2 and 'local_hero_id' not in saved:
                saved['local_hero_id'] = DEFAULT_HERO_ID
            population = saved.get('total_players')
            if type(population) is int and MAX_UNIQUE_PLAYERS < population <= 32:
                saved['total_players'] = MAX_UNIQUE_PLAYERS
            self.value = validate(saved)
        except FileNotFoundError:
            self.value = copy.deepcopy(DEFAULTS)

    def read(self):
        with self.lock:
            return copy.deepcopy(self.value)

    def update(self, patch, expected_revision):
        with self.lock:
            if isinstance(expected_revision, bool) or expected_revision != self.value["revision"]:
                raise RevisionConflict("设置已在另一页面更新，请刷新后再试")
            value = merge(self.value, patch)
            self.path.parent.mkdir(parents=True, exist_ok=True)
            temporary = self.path.with_suffix(".json.tmp")
            with temporary.open("w", encoding="utf-8", newline="\n") as stream:
                json.dump(value, stream, ensure_ascii=False, indent=2)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.path)
            self.value = value
            return copy.deepcopy(value)


class RevisionConflict(ValueError):
    pass


def world_to_uv(transform: dict, point):
    """The shipped MapUIData axes are intentionally crossed."""
    if transform.get('type') == 'affine_2x3':
        a = transform['matrix']
        return tuple(row[0] * point[0] + row[1] * point[1] + row[2] for row in a)
    return (point[1] * transform["x_multiplier"] + transform["x_scalar_to_add"],
            point[0] * transform["y_multiplier"] + transform["y_scalar_to_add"])


def uv_to_world(transform: dict, uv, z):
    if transform.get('type') == 'affine_2x3':
        a = transform['matrix']
        p, q = uv[0] - a[0][2], uv[1] - a[1][2]
        determinant = a[0][0] * a[1][1] - a[0][1] * a[1][0]
        if not determinant or not all(math.isfinite(float(v)) for row in a for v in row):
            raise ValueError('地图坐标矩阵无效')
        return [(p * a[1][1] - a[0][1] * q) / determinant,
                (a[0][0] * q - p * a[1][0]) / determinant, _number(z, -1e7, 1e7, '地面高度')]
    xm, ym = float(transform["x_multiplier"]), float(transform["y_multiplier"])
    if not xm or not ym or not all(math.isfinite(float(v)) for v in (*uv, z, xm, ym)):
        raise ValueError("地图没有有效坐标变换")
    return [(uv[1] - transform["y_scalar_to_add"]) / ym,
            (uv[0] - transform["x_scalar_to_add"]) / xm, float(z)]
