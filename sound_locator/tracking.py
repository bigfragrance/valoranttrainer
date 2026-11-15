"""Map-constrained, bearing-only multi-hypothesis tracks.

Listener rotations are exact inputs. Range stays uncertain unless evidence and map
geometry constrain it. Dynamic track lists have no fixed source-count ceiling.
"""
from __future__ import annotations
from dataclasses import dataclass
import math
import numpy as np
from scipy.optimize import linear_sum_assignment
from direction_field import DirectionObservation


@dataclass(frozen=True)
class ListenerPose:
    x_cm: float
    y_cm: float
    yaw_deg: float
    time_s: float
    z_cm: float=0.


class GraphParticleTrack:
    def __init__(self, identifier, geometry, observation, pose, rng, particles=1600):
        self.id=identifier;self.map=geometry;self.rng=rng;self.n=particles
        self.points=np.asarray(geometry.points_world,dtype=float)
        self.xy=self.points[:,:2]
        self.edges=geometry.adjacency
        self.signature=observation.signature.copy()
        self.created=self.last_seen=self.last_time=pose.time_s
        self.hits=1;self.history=[]
        self.heading_span=[]
        self.previous_direction=np.zeros((particles,2))
        delta=self.xy-np.array([pose.x_cm,pose.y_cm])
        angles=np.degrees(np.arctan2(delta[:,1],delta[:,0]))-pose.yaw_deg
        prior=observation.likelihood(angles)
        # A broad distance prior is not a range measurement. Keep map support explicit.
        distance=np.linalg.norm(delta,axis=1)
        prior*=np.exp(-distance/5000.)
        prior/=prior.sum()
        self.src=rng.choice(len(self.xy),particles,p=prior)
        self.dst=self.src.copy();self.progress=np.zeros(particles)
        self.speed=rng.choice([0.,150.,300.,450.,600.],particles)
        self.weight=np.full(particles,1/particles)

    def positions(self):
        return self.xy[self.src]+(self.xy[self.dst]-self.xy[self.src])*self.progress[:,None]

    def predict(self, timestamp):
        dt=max(0,min(timestamp-self.last_time,1.0));self.last_time=timestamp
        if not dt:return
        change=self.rng.random(self.n)<dt*.8
        self.speed[change]=self.rng.choice([0.,100.,200.,300.,400.,500.,600.,700.],change.sum())
        remaining=self.speed*dt
        for i in range(self.n):
            # Traverse actual graph edges; no unconstrained XY jumps through walls.
            while remaining[i]>1e-7:
                if self.src[i]==self.dst[i]:
                    neighbors=self.edges[int(self.src[i])]
                    if not neighbors:break
                    dirs=self.xy[neighbors]-self.xy[self.src[i]]
                    lengths=np.linalg.norm(dirs,axis=1)
                    directions=dirs/(lengths[:,None]+1e-9)
                    probs=np.exp(2*(directions@self.previous_direction[i]))
                    probs/=probs.sum()
                    self.dst[i]=self.rng.choice(neighbors,p=probs)
                segment=self.xy[self.dst[i]]-self.xy[self.src[i]]
                length=float(np.linalg.norm(segment))
                if length<1e-6:break
                available=length*(1-self.progress[i])
                move=min(available,remaining[i])
                self.progress[i]+=move/length;remaining[i]-=move
                if self.progress[i]>=1-1e-8:
                    self.previous_direction[i]=segment/length
                    self.src[i]=self.dst[i];self.progress[i]=0

    def evidence(self, obs, pose):
        delta=self.positions()-np.array([pose.x_cm,pose.y_cm])
        angle=np.degrees(np.arctan2(delta[:,1],delta[:,0]))-pose.yaw_deg
        return obs.likelihood(angle)

    def association_cost(self,obs,pose):
        measured=float(np.dot(self.weight,self.evidence(obs,pose)))*len(obs.angles)
        spectral=float(np.clip(np.dot(self.signature,obs.signature),0,1))
        return -math.log(max(measured,1e-8))+.6*(1-spectral)

    def update(self,obs,pose):
        likelihood=self.evidence(obs,pose)
        self.weight*=np.sqrt(likelihood*len(obs.angles)+.03)
        self.weight/=self.weight.sum()
        self.last_seen=pose.time_s;self.hits+=1
        self.heading_span.append(pose.yaw_deg)
        self.signature=.9*self.signature+.1*obs.signature
        self.signature/=np.linalg.norm(self.signature)+1e-12
        state=self.summary(pose.time_s)
        # Save the estimate and its uncertainty together; a line alone would overstate range.
        self.history.append({"time_s":pose.time_s,"world_xy_cm":state["world_xy_cm"],
                             "radius90_cm":state["radius90_cm"]})
        self.history=self.history[-300:]
        if 1/np.sum(self.weight*self.weight)<self.n*.45:
            u=(self.rng.random()+np.arange(self.n))/self.n
            idx=np.searchsorted(np.cumsum(self.weight),u)
            self.src=self.src[idx].copy();self.dst=self.dst[idx].copy()
            self.progress=self.progress[idx].copy();self.speed=self.speed[idx].copy()
            self.previous_direction=self.previous_direction[idx].copy()
            self.weight.fill(1/self.n)

    def summary(self,timestamp):
        positions=self.positions()
        mean=np.sum(positions*self.weight[:,None],axis=0)
        # Select a supported point, not an average that may land inside a wall.
        nearest=int(np.argmin(np.sum((positions-mean)**2,axis=1)))
        center=positions[nearest]
        radius=np.linalg.norm(positions-center,axis=1)
        order=np.argsort(radius)
        r90=float(radius[order[min(np.searchsorted(np.cumsum(self.weight[order]),.9),self.n-1)]])
        confirmed=self.hits>=3
        return {"track_id":self.id,"status":"track_hypothesis" if confirmed else "tentative",
                "world_xy_cm":center.tolist(),"map_uv":np.asarray(self.map.world_to_map(center)).tolist(),
                "radius90_cm":r90,"position_concentrated":r90<300 and self.hits>=6,
                "distance_directly_measured":False,
                "hits":self.hits,"seconds_since_evidence":timestamp-self.last_seen,
                "uncertainty_is_calibrated":False,"history":list(self.history),
                "particles_world_xy_cm":positions[::max(1,self.n//160)].tolist()}


class MultiSourceTracker:
    def __init__(self,geometry,particles_per_track=1600,seed=3485,method="graph"):
        if not len(geometry.points_world):raise ValueError("地图没有可用导航节点。")
        self.map=geometry;self.particles=particles_per_track;self.rng=np.random.default_rng(seed)
        if method=="graph":
            from graph_belief import GraphBeliefTrack
            self.track_factory=GraphBeliefTrack
        elif method=="particle":self.track_factory=GraphParticleTrack
        else:raise ValueError("未知地图跟踪方法。")
        self.tracks=[];self.next_id=1;self.last_time=None

    def update(self,observations:list[DirectionObservation],pose:ListenerPose):
        if self.last_time is not None and pose.time_s<self.last_time:
            raise ValueError("姿态/音频时间必须单调递增。")
        self.last_time=pose.time_s
        for track in self.tracks:track.predict(pose.time_s)
        matched=set()
        if self.tracks and observations:
            costs=np.array([[t.association_cost(o,pose) for o in observations] for t in self.tracks])
            rr,cc=linear_sum_assignment(costs)
            for i,j in zip(rr,cc):
                if costs[i,j]<2.0:
                    self.tracks[i].update(observations[j],pose);matched.add(int(j))
        for j,obs in enumerate(observations):
            if j not in matched:
                self.tracks.append(self.track_factory(self.next_id,self.map,obs,pose,self.rng,self.particles))
                self.next_id+=1
        self.tracks=[t for t in self.tracks if pose.time_s-t.last_seen<=4.0]
        return {"time_s":pose.time_s,"track_count_hypotheses":len(self.tracks),
                "confirmed_source_count":None,"fixed_source_limit":None,
                "map_coverage":self.map.coverage,
                "tracks":[t.summary(pose.time_s) for t in self.tracks]}
