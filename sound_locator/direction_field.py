"""V2: continuous circular evidence and adaptive source hypotheses, no top-K cap.

This is a sparse time-frequency mixture approximation, not arbitrary source recovery.
Keep core.py unchanged so the first experiment remains reproducible.
"""
from __future__ import annotations
from dataclasses import dataclass
import numpy as np
from scipy.ndimage import gaussian_filter1d
from core import Profile, RATE, FREQ, BINS, spectra, features, stereo, angular_distance


@dataclass
class DirectionObservation:
    angles: np.ndarray
    scores: np.ndarray
    signature: np.ndarray
    strength: float
    peak_deg: float
    alternatives_deg: list[float]
    range_hint: dict | None = None

    def likelihood(self, relative_angles):
        a=np.r_[self.angles,360.]
        v=np.r_[self.scores,self.scores[0]]
        return np.interp(np.asarray(relative_angles)%360,a,v)


class DirectionField:
    def __init__(self, profile: Profile, resolution=3):
        if not profile.full_circle:raise ValueError("需要完整一圈的真实校准。")
        self.profile=profile
        self.angles=np.arange(0,360,resolution,dtype=float)
        order=np.argsort(profile.angles%360)
        original=profile.angles[order]%360
        a=np.r_[original[-1]-360,original,original[0]+360]
        def interpolate(values):
            v=np.concatenate([values[order[-1:]],values[order],values[order[:1]]])
            return np.array([np.interp(self.angles,a,v[:,j]) for j in range(v.shape[1])]).T
        self.ild=interpolate(profile.ild)
        # Interpolate complex interaural phase, then project back onto the unit circle.
        ph=interpolate(profile.phase.real)+1j*interpolate(profile.phase.imag)
        self.phase=ph/(np.abs(ph)+1e-12)
        self.support=interpolate(profile.support)
        self.bands=np.array_split(np.arange(BINS.sum()),16)

    def analyze(self, samples, rate=RATE, frame_yaws_deg=None):
        x=stereo(samples,rate)
        output={"status":"quiet","observations":[],"angles":self.angles,
                "scores":np.zeros_like(self.angles),"source_count":None,
                "number_of_sources_is_not_fixed":True}
        if len(x)<1024 or np.sqrt(np.mean(x*x))<10**(-52/20):return output
        if np.max(np.abs(x[:,0]-x[:,1]))<1e-7:
            output["status"]="center_or_mono";return output
        ild,phase,w=features(x,self.profile.noise_floor)
        if self.profile.noise_floor is not None:
            ratio=float((w*w).sum()/(np.sum(np.abs(spectra(x))**2)+1e-12))
            output["background_excess_fraction"]=ratio
            if ratio<.08:output["status"]="background";return output
        coh=np.abs((phase*w).sum(axis=0))/(w.sum(axis=0)+1e-12)
        reliable=coh>.45
        if (w*reliable).sum()<.15*w.sum():
            output["status"]="incoherent";return output
        w*=reliable*coh
        dist=((ild[None]-self.ild[:,None])/5)**2
        dist+=3*(1-np.real(phase[None]*np.conj(self.phase[:,None])))
        dist+=(1-self.support[:,None])*4
        best=np.min(dist,axis=0)
        accepted=best<2.5
        weights=w*np.exp(-best/2)*accepted
        total=float(w.sum()+1e-12)
        if weights.sum()<.15*total:
            output["status"]="unmatched";return output
        # Soft evidence preserves adjacent angles and competing front/back explanations.
        soft=np.exp(-np.minimum(dist-best[None],40)/.65)
        soft/=soft.sum(axis=0,keepdims=True)+1e-12
        compatibility=np.exp(-np.minimum(dist-best[None],40)/2)
        if frame_yaws_deg is not None:
            yaw=np.asarray(frame_yaws_deg,dtype=float)
            if yaw.shape!=(soft.shape[1],) or not np.isfinite(yaw).all():
                raise ValueError("每个STFT帧必须有已知、有限的视角角度。")
            yaw=np.rad2deg(np.unwrap(np.deg2rad(yaw)))
            reference=yaw[len(yaw)//2]
            # Align each short-time frame to the window-center camera orientation.
            # Rotation does not triangulate range; it improves direction evidence.
            for t,delta in enumerate(yaw-reference):
                shift=delta/(self.angles[1]-self.angles[0])
                integer=int(np.floor(shift));fraction=shift-integer
                soft[:,t]=(1-fraction)*np.roll(soft[:,t],integer,axis=0)+fraction*np.roll(soft[:,t],integer+1,axis=0)
                compatibility[:,t]=(1-fraction)*np.roll(compatibility[:,t],integer,axis=0)+fraction*np.roll(compatibility[:,t],integer+1,axis=0)
            output["window_reference_yaw_deg"]=float(reference%360)
            output["known_rotation_span_deg"]=float(np.ptp(yaw))
        votes=np.sum(soft*weights[None],axis=(1,2))/total
        output["single_direction_explained_fraction"]=np.sum(compatibility*weights[None],axis=(1,2))/(weights.sum()+1e-12)
        curve=gaussian_filter1d(votes,1.0,mode="wrap")
        output["scores"]=curve/(curve.sum()+1e-12)
        # Local maxima with evidence thresholds; no arbitrary maximum source count.
        peaks=np.where((curve>=np.roll(curve,1))&(curve>np.roll(curve,-1)))[0]
        peaks=[int(k) for k in peaks if curve[k]>max(.0015,1.5*np.median(curve))]
        used=set();observations=[]
        for k in sorted(peaks,key=lambda i:curve[i],reverse=True):
            if k in used:continue
            near=angular_distance(self.angles,self.angles[k])<=12
            if curve[near].sum()<.025:continue
            original=int(np.argmin(angular_distance(self.profile.angles,self.angles[k])))
            aliases=self.profile.aliases[original]
            group=near.copy()
            for alias in aliases:group|=angular_distance(self.angles,alias)<=12
            for j in peaks:
                if group[j]:used.add(j)
            mask=np.sum(soft[group],axis=0)
            pw=np.sum(weights*mask,axis=0)
            signature=np.array([pw[b].sum() for b in self.bands])
            signature/=np.linalg.norm(signature)+1e-12
            # A wide floor deliberately prevents unjustified single-frame certainty.
            likelihood=gaussian_filter1d(curve*group,2.0,mode="wrap")
            likelihood+=max(likelihood.max()*.015,1e-10)
            likelihood/=likelihood.sum()
            observations.append(DirectionObservation(self.angles,likelihood,signature,
                float(curve[group].sum()),float(self.angles[k]),aliases))
        output["observations"]=observations
        output["status"]="evidence" if observations else "unmatched"
        return output
