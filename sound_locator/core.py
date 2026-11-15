"""Binaural direction evidence. No game process access; scores are not probabilities."""
from __future__ import annotations

from dataclasses import dataclass, asdict
from pathlib import Path
import json
import math
import time

import numpy as np
from scipy import signal
from scipy.io import wavfile

RATE = 48000
NFFT = 1024
HOP = 256
FREQ = np.fft.rfftfreq(NFFT, 1 / RATE)
BINS = (FREQ >= 250) & (FREQ <= 12000)


def stereo(samples, rate=RATE):
    x = np.asarray(samples)
    if x.ndim != 2 or x.shape[1] != 2:
        raise ValueError("需要恰好两个声道的立体声音频；不能用单声道或直接截取 7.1 声道。")
    if np.issubdtype(x.dtype, np.integer):
        if x.dtype == np.uint8:
            x = (x.astype(float) - 128) / 128
        else:
            x = x.astype(float) / (2 ** (np.iinfo(x.dtype).bits - 1))
    x = x.astype(np.float64)
    if not np.isfinite(x).all() or not 8000 <= rate <= 192000:
        raise ValueError("音频采样或采样率无效。")
    if rate != RATE:
        gcd = math.gcd(int(rate), RATE)
        x = signal.resample_poly(x, RATE // gcd, int(rate) // gcd, axis=0)
    return x


def read_wav(path):
    rate, samples = wavfile.read(path)
    return stereo(samples, rate)


def spectra(x):
    if len(x) < NFFT:
        raise ValueError("音频过短。")
    frames = np.lib.stride_tricks.sliding_window_view(x, NFFT, axis=0)[::HOP]
    z = np.fft.rfft(frames * np.hanning(NFFT), axis=-1)[:, :, BINS]
    return z


def features(x, noise_floor=None):
    z = spectra(x)
    left, right = z[:, 0], z[:, 1]
    a, b = np.abs(left), np.abs(right)
    energy = a * a + b * b
    ild = np.clip(20 * np.log10((b + 1e-10) / (a + 1e-10)), -36, 36)
    phase = right * np.conj(left) / (a * b + 1e-15)
    valid = energy > max(float(energy.max()) * 0.001, 1e-9)
    excess = energy if noise_floor is None else np.maximum(energy - 2*noise_floor, 0)
    weight = np.sqrt(excess) * valid
    return ild, phase, weight


def angular_distance(a, b):
    return np.abs((np.asarray(a) - b + 180) % 360 - 180)


@dataclass(frozen=True)
class Pose:
    x: float = 0.5
    y: float = 0.5
    heading_deg: float = 0.0
    timestamp: float | None = None

    def __post_init__(self):
        if not all(math.isfinite(float(v)) for v in (self.x, self.y, self.heading_deg)):
            raise ValueError("位置和朝向必须是有限数值。")
        if not 0 <= self.x <= 1 or not 0 <= self.y <= 1:
            raise ValueError("位置 x、y 使用地图归一化坐标 0～1。")
        if self.timestamp is not None and not math.isfinite(self.timestamp):
            raise ValueError("时间戳无效。")

    def ray(self, relative_deg, length=0.25):
        bearing = (self.heading_deg + relative_deg) % 360
        r = math.radians(bearing)
        return {"bearing_deg": bearing, "origin": [self.x, self.y],
                "unit_vector": [math.sin(r), -math.cos(r)],
                "display_end": [self.x + length * math.sin(r), self.y - length * math.cos(r)]}


def read_pose(path, now=None, max_age=1.0):
    data = json.loads(Path(path).read_text(encoding="utf-8-sig"))
    pose = Pose(float(data["x"]), float(data["y"]), float(data["heading_deg"]), float(data["timestamp"]))
    age = (time.time() if now is None else now) - pose.timestamp
    if age > max_age or age < -0.25:
        raise ValueError("位置数据过期或时钟不一致，暂停地图投影。")
    return pose


class Profile:
    def __init__(self, angles, ild, phase, support, metadata=None, noise_floor=None):
        self.angles = np.asarray(angles, dtype=float)
        self.ild = np.asarray(ild, dtype=float)
        self.phase = np.asarray(phase, dtype=complex)
        self.support = np.asarray(support, dtype=float)
        self.metadata = metadata or {}
        self.noise_floor = None if noise_floor is None else np.asarray(noise_floor, dtype=float)
        if self.noise_floor is not None and (self.noise_floor.shape != (int(BINS.sum()),)
                or not np.isfinite(self.noise_floor).all() or np.any(self.noise_floor<0)):
            raise ValueError("背景频谱维度或数值无效。")
        shape = (len(self.angles), int(BINS.sum()))
        if not len(self.angles) or any(a.shape != shape for a in (self.ild, self.phase, self.support)):
            raise ValueError("校准配置维度不正确。")
        if not all(np.isfinite(a).all() for a in (self.angles, self.ild, self.phase, self.support)):
            raise ValueError("校准配置包含无效数值。")
        if np.any(self.support < 0) or np.any(self.support > 1):
            raise ValueError("校准支持度无效。")
        if len(np.unique(self.angles % 360)) != len(self.angles):
            raise ValueError("校准角度重复。")
        self.aliases = []
        for k, angle in enumerate(self.angles):
            common = np.minimum(self.support, self.support[k])
            d = ((self.ild-self.ild[k])/5)**2 + 3*(1-np.real(self.phase*np.conj(self.phase[k])))
            distance = (d*common).sum(axis=1)/(common.sum(axis=1)+1e-12)
            self.aliases.append([float(self.angles[j] % 360) for j in range(len(self.angles))
                                 if angular_distance(self.angles[j], angle)>25 and distance[j]<.15
                                 and np.count_nonzero(common[j]>.3)>=16])

    @classmethod
    def train(cls, clips, metadata=None, background=None):
        noise_floor = None
        if background is not None:
            z = spectra(stereo(background))
            noise_floor = np.percentile((np.abs(z)**2).sum(axis=1), 90, axis=0)
        grouped = {}
        for angle, x in clips:
            angle = float(angle) % 360
            x = stereo(x)
            if len(x) < RATE // 4 or np.sqrt(np.mean(x*x)) < 10 ** (-55/20):
                raise ValueError("校准片段太短或太安静；请录制单个已知方向的声音。")
            ild, phase, w = features(x, noise_floor)
            # A phase-coherent transfer ratio cancels a single source's spectrum.
            grouped.setdefault(angle, []).append((ild, phase, w))
        angles, ilds, phases, supports = [], [], [], []
        for angle, rows in sorted(grouped.items()):
            ild = np.concatenate([r[0] for r in rows])
            phase = np.concatenate([r[1] for r in rows])
            w = np.concatenate([r[2] for r in rows])
            total = w.sum(axis=0)
            p = (phase*w).sum(axis=0) / (total + 1e-12)
            support = np.minimum(total / (np.max(total)*0.05 + 1e-12), 1) * np.abs(p)
            if np.count_nonzero(support > .3) < 16:
                raise ValueError(f"{angle:g}° 的频谱覆盖不足，不能用于方向校准。")
            angles.append(angle)
            ilds.append((ild*w).sum(axis=0) / (total+1e-12))
            phases.append(p / (np.abs(p)+1e-12))
            supports.append(support)
        return cls(angles, ilds, phases, supports, metadata, noise_floor)

    @property
    def full_circle(self):
        a = np.sort(self.angles % 360)
        return len(a) >= 8 and np.max(np.diff(np.r_[a, a[0]+360])) <= 60

    def save(self, path):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("wb") as f:
            np.savez_compressed(f, angles=self.angles, ild=self.ild, phase=self.phase,
                                support=self.support, metadata=json.dumps(self.metadata, ensure_ascii=False),
                                noise_floor=self.noise_floor if self.noise_floor is not None else np.array([]),
                                schema=1, sample_rate=RATE, nfft=NFFT)

    @classmethod
    def load(cls, path):
        with np.load(path, allow_pickle=False) as d:
            if int(d["schema"]) != 1 or int(d["sample_rate"]) != RATE or int(d["nfft"]) != NFFT:
                raise ValueError("校准配置的算法版本不兼容。")
            noise = d["noise_floor"] if "noise_floor" in d and d["noise_floor"].size else None
            return cls(d["angles"], d["ild"], d["phase"], d["support"], json.loads(str(d["metadata"])), noise)


class Locator:
    def __init__(self, profile=None, gate_db=-52.0, max_candidates=3):
        self.profile = profile
        self.gate_db = gate_db
        self.max_candidates = max_candidates

    def analyze(self, samples, rate=RATE, pose=None):
        x = stereo(samples, rate)
        rms = np.sqrt(np.mean(x*x, axis=0))
        db = float(20*np.log10(np.sqrt(np.mean(x*x))+1e-12))
        ild_total = float(20*np.log10((rms[1]+1e-12)/(rms[0]+1e-12)))
        result = {"level_dbfs": round(db, 2), "ild_db": round(ild_total, 2),
                  "status": "quiet", "candidates": [], "source_count": None,
                  "distance": None, "score_is_probability": False,
                  "side_hint": "右侧" if ild_total > 1.5 else "左侧" if ild_total < -1.5 else "左右接近",
                  "pose": asdict(pose) if pose else None}
        if db < self.gate_db:
            return result
        if len(x) < NFFT:
            result["status"] = "too_short"
            return result
        if np.max(np.abs(x[:, 0]-x[:, 1])) < 1e-7:
            result["status"] = "center_or_mono"
            return result
        if self.profile is None:
            result["status"] = "uncalibrated"
            return result
        p = self.profile
        if not p.full_circle:
            result["status"] = "incomplete_profile"
            return result
        ild, phase, w = features(x, p.noise_floor)
        if p.noise_floor is not None:
            result["background_excess_fraction"] = round(float((w*w).sum()/(np.sum(np.abs(spectra(x))**2)+1e-12)), 4)
            if result["background_excess_fraction"] < .08:
                result["status"] = "background"
                return result
        if w.sum() < 1e-8:
            result["status"] = "background"
            return result
        coherence = np.abs((phase*w).sum(axis=0)) / (w.sum(axis=0)+1e-12)
        reliable = coherence > .55
        result["coherent_energy_fraction"] = round(float((w*reliable).sum()/(w.sum()+1e-12)), 3)
        if result["coherent_energy_fraction"] < .20:
            result["status"] = "incoherent"
            return result
        w = w * reliable * coherence
        # Robust local matching: overlapping sources may dominate different TF cells.
        dist = ((ild[None]-p.ild[:, None])/5.0)**2
        dist += 3*(1-np.real(phase[None]*np.conj(p.phase[:, None])))
        dist += (1-p.support[:, None])*4
        best = np.argmin(dist, axis=0)
        error = np.min(dist, axis=0)
        accepted = error < 2.5
        fit = np.exp(-error/2) * accepted
        weighted = w*fit
        total = float(w.sum()+1e-12)
        scores = np.bincount(best.ravel(), weights=weighted.ravel(), minlength=len(p.angles))/total
        good = float((w*accepted).sum()/total)
        result["matched_energy_fraction"] = round(good, 3)
        if good < .25 or scores.max() < .08:
            result["status"] = "unmatched"
            return result
        remaining = scores.copy()
        for _ in range(self.max_candidates):
            k = int(remaining.argmax())
            if remaining[k] < max(.08, scores.max()*.30):
                break
            angle = float(p.angles[k] % 360)
            neighbor = angular_distance(p.angles, angle) <= 25
            support = float(scores[neighbor].sum())
            candidate = {"relative_deg": angle, "support": round(support, 3),
                         "ambiguous_with_deg": p.aliases[k],
                         "label": "direction_hypothesis", "ray": pose.ray(angle) if pose else None}
            result["candidates"].append(candidate)
            remaining[neighbor] = 0
        result["status"] = "candidates"
        return result
