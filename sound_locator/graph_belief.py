"""Full-support graph filtering of bearing evidence, without particle extinction.

Two hypotheses (stationary and moving) retain every original navigation node.
The result is a set of compatible map regions, not a range measurement. In
particular, rotating in place must never manufacture distance observability.
"""
from __future__ import annotations
import math
import weakref
import numpy as np
from scipy import sparse
from scipy.ndimage import gaussian_filter1d
from scipy.spatial import cKDTree
from scipy.sparse.csgraph import connected_components


_CACHE=weakref.WeakKeyDictionary()


class _Graph:
    def __init__(self,geometry):
        self.points=np.asarray(geometry.points_world,float)
        self.xy=self.points[:,:2]
        self.n=len(self.points)
        rows=[];cols=[]
        for i,neighbors in enumerate(geometry.adjacency):
            for j in neighbors:
                rows.append(i);cols.append(j)
        self.rows=np.asarray(rows,int);self.cols=np.asarray(cols,int)
        self.length=np.linalg.norm(self.points[self.rows]-self.points[self.cols],axis=1)
        self.edge_max=float(self.length.max(initial=1))
        # Correct the heavily nonuniform density of interpolated path nodes.
        # This is spatial sampling correction, never an acoustic range prior.
        density=cKDTree(self.points).query_ball_point(self.points,150,return_length=True)
        self.base=1/np.maximum(density,1)
        self.base/=self.base.sum()
        self.degree=np.bincount(self.rows,minlength=self.n)
        self.connections=sparse.csr_matrix((np.ones(len(rows)),(rows,cols)),shape=(self.n,self.n))
        self.transitions={}

    def transition(self,max_edge_cm):
        # Round down so each accepted edge respects this remaining motion budget.
        key=int(max_edge_cm*10)
        if key in self.transitions:return self.transitions[key]
        keep=(self.length<=key/10+1e-9)&(self.length>1e-7)
        a,b=self.rows[keep],self.cols[keep]
        degree=np.bincount(a,minlength=self.n)
        proposal=1/np.maximum(degree[a],1)
        accept=np.minimum(1,self.base[b]*degree[a]/(self.base[a]*np.maximum(degree[b],1)))
        values=.8*proposal*accept
        diagonal=1-np.bincount(a,weights=values,minlength=self.n)
        matrix=sparse.csr_matrix((values,(b,a)),shape=(self.n,self.n)) + sparse.diags(diagonal,dtype=float)
        # Column-stochastic: posterior = matrix @ previous.
        if len(self.transitions)>30:self.transitions.clear()
        self.transitions[key]=matrix.tocsr()
        return self.transitions[key]


class GraphBeliefTrack:
    def __init__(self,identifier,geometry,observation,pose,rng=None,particles=None):
        if geometry not in _CACHE:_CACHE[geometry]=_Graph(geometry)
        self.graph=_CACHE[geometry];self.map=geometry
        self.points=self.graph.points;self.xy=self.graph.xy;self.edges=geometry.adjacency
        self.id=identifier;self.rng=rng;self.n=len(self.points)
        self.created=self.last_seen=self.last_time=pose.time_s
        self.motion_remainder_cm=0.
        self.hits=1;self.history=[];self.pose_history=[]
        self.bearing_history=[float(observation.peak_deg+pose.yaw_deg)]
        self.signature=np.asarray(observation.signature,float).copy()
        self.mode_probability=np.array([.5,.5])
        likelihood=self.evidence(observation,pose)
        initial=self.graph.base*np.sqrt(likelihood)
        initial/=initial.sum()
        self.stationary=initial.copy();self.moving=initial.copy()
        self.weight=initial.copy()
        self.last_likelihood=likelihood
        self.last_range_hint=getattr(observation,"range_hint",None)
        self._observe_pose(pose)

    def _observe_pose(self,pose):
        self.pose_history.append([pose.x_cm,pose.y_cm])
        self.pose_history=self.pose_history[-200:]
        self.latest_pose=pose

    def positions(self):
        return self.xy

    def predict(self,timestamp):
        dt=max(0.,float(timestamp)-self.last_time);self.last_time=float(timestamp)
        if dt==0:return
        # Switching source motion modes preserves the same physical node.
        exchange=min(.12,dt*.06)
        old=self.stationary.copy()
        self.stationary=(1-exchange)*old+exchange*self.moving
        self.moving=(1-exchange)*self.moving+exchange*old
        remaining=700*min(dt,4.)+self.motion_remainder_cm
        steps=int(remaining/self.graph.edge_max)
        for _ in range(steps):
            self.moving=self.graph.transition(self.graph.edge_max+1e-6)@self.moving
        self.motion_remainder_cm=remaining-steps*self.graph.edge_max
        self.weight=self.mode_probability[0]*self.stationary+self.mode_probability[1]*self.moving
        self.weight/=self.weight.sum()

    def evidence(self,obs,pose):
        delta=self.xy-np.array([pose.x_cm,pose.y_cm])
        angle=(np.degrees(np.arctan2(delta[:,1],delta[:,0]))-pose.yaw_deg)%360
        # Finite model/systematic angular error survives repeated observations.
        spacing=float(np.median(np.diff(obs.angles)))
        curve=gaussian_filter1d(np.asarray(obs.scores,float),max(1,5/max(spacing,1e-6)),mode="wrap")
        curve=np.maximum(curve,curve.max()*.005+1e-12)
        curve/=curve.sum()
        likelihood=np.interp(angle,np.r_[obs.angles,360.],np.r_[curve,curve[0]])
        hint=getattr(obs,"range_hint",None)
        if hint and hint.get("log_distance_grid") and hint.get("likelihood"):
            grid=np.asarray(hint["log_distance_grid"],float)
            values=np.asarray(hint["likelihood"],float)
            if grid.ndim==1 and len(grid)==len(values) and len(grid)>1 and np.isfinite(grid).all() and np.isfinite(values).all():
                distance_m=np.maximum(np.linalg.norm(delta,axis=1)/100,.1)
                measured=np.interp(np.log(distance_m),grid,values,left=float(values.min()),right=float(values.min()))
                # The range module already supplies a conservative flat likelihood floor.
                likelihood*=measured/(values.max()+1e-12)
        return likelihood

    def association_cost(self,obs,pose):
        measured=float(self.weight@self.evidence(obs,pose))*len(obs.angles)
        spectral=float(np.clip(self.signature@obs.signature,0,1))
        return -math.log(max(measured,1e-12))+.6*(1-spectral)

    def update(self,obs,pose):
        likelihood=self.evidence(obs,pose)
        self.last_likelihood=likelihood
        self.last_range_hint=getattr(obs,"range_hint",None)
        log_measure=.5*np.log(likelihood+1e-300)
        evidences=[]
        for name in ("stationary","moving"):
            distribution=getattr(self,name)
            # Discount stale/correlated evidence toward the original spatial
            # base measure. This alters epistemic confidence, not graph motion.
            log_prior=.93*np.log(distribution+1e-300)+.07*np.log(self.graph.base)
            log_posterior=log_prior+log_measure
            maximum=log_posterior.max()
            unnormalized=np.exp(log_posterior-maximum)
            evidences.append(maximum+math.log(unnormalized.sum()))
            normalized=unnormalized/unnormalized.sum()
            normalized=np.maximum(normalized,np.finfo(float).tiny)
            normalized/=normalized.sum()
            setattr(self,name,normalized)
        logs=np.log(self.mode_probability+1e-12)+np.asarray(evidences)
        self.mode_probability=np.exp(logs-logs.max());self.mode_probability/=self.mode_probability.sum()
        self.mode_probability=.98*self.mode_probability+.01
        self.weight=self.mode_probability[0]*self.stationary+self.mode_probability[1]*self.moving
        self.weight/=self.weight.sum()
        self.last_seen=pose.time_s;self.hits+=1;self._observe_pose(pose)
        self.bearing_history.append(float(obs.peak_deg+pose.yaw_deg));self.bearing_history=self.bearing_history[-200:]
        self.signature=.9*self.signature+.1*obs.signature
        self.signature/=np.linalg.norm(self.signature)+1e-12
        summary=self.summary(pose.time_s)
        self.history.append({"time_s":pose.time_s,"world_xy_cm":summary["world_xy_cm"],
                             "radius90_cm":summary["radius90_cm"],
                             "position_concentrated":summary["position_concentrated"]})
        self.history=self.history[-300:]

    def _regions(self,mask,density):
        ids=np.flatnonzero(mask)
        if not len(ids):ids=np.array([int(density.argmax())])
        graph=self.graph.connections[ids][:,ids]
        count,labels=connected_components(graph,directed=False)
        regions=[]
        for label in range(count):
            nodes=ids[labels==label];xy=self.xy[nodes]
            representative=nodes[np.argmax(density[nodes])]
            regions.append({"representative_node":int(representative),
                "representative_world_xy_cm":self.xy[representative].tolist(),
                "representative_is_source_location":False,
                "node_count":int(len(nodes)),"posterior_mass":float(self.weight[nodes].sum()),
                "bounds_world_xy_cm":[xy.min(axis=0).tolist(),xy.max(axis=0).tolist()],
                "node_indices":nodes.tolist()})
        return sorted(regions,key=lambda r:r["posterior_mass"],reverse=True),ids

    def summary(self,timestamp):
        density=self.weight/self.graph.base
        representative=int(density.argmax())
        center=self.xy[representative]
        pose_positions=np.asarray(self.pose_history)
        baseline=float(np.max(np.linalg.norm(pose_positions-pose_positions[0],axis=1)))
        # A posterior-mass region alone may discard nearby points because a
        # distant angular sector contains more map area. Keep explicit bearing
        # compatibility wherever listener translation has not observed range.
        bearing_span=float(np.ptp(np.rad2deg(np.unwrap(np.deg2rad(self.bearing_history)))))
        range_observable=baseline>=150 and bearing_span>=10 and self.hits>=6
        compatible=self.last_likelihood>=self.last_likelihood.max()*.05
        order=np.argsort(density)[::-1]
        stop=min(int(np.searchsorted(np.cumsum(self.weight[order]),.9)),self.n-1)
        high_density=np.zeros(self.n,bool);high_density[order[:stop+1]]=True
        mask=high_density|(density>=density.max()*.05)
        if not range_observable:mask|=compatible
        regions,ids=self._regions(mask,density)
        # Envelope of the supported regions is deliberately more conservative
        # than a 90% posterior radius. It is not a calibrated confidence bound.
        radius=float(np.max(np.linalg.norm(self.xy[ids]-center,axis=1)))
        concentrated=bool(range_observable and radius<300)
        return {"track_id":self.id,"status":"track_hypothesis" if self.hits>=3 else "tentative",
                "world_xy_cm":center.tolist(),"map_uv":np.asarray(self.map.world_to_map(center)).tolist(),
                "representative_is_source_location":concentrated,
                "radius90_cm":radius,"radius_kind":"compatible-region envelope containing >=90% posterior mass",
                "position_concentrated":concentrated,"distance_directly_measured":False,
                "range_observability":"listener_translation_with_bearing_change_motion_model_dependent" if range_observable else "unresolved_bearing_only",
                "observed_world_bearing_span_deg":bearing_span,
                "listener_baseline_cm":baseline,"stationary_mode_probability":float(self.mode_probability[0]),
                "weak_audio_range":self.last_range_hint,
                "hits":self.hits,"seconds_since_evidence":timestamp-self.last_seen,
                "uncertainty_is_calibrated":False,"history":list(self.history),
                "candidate_regions":regions,"full_support_nodes":self.n,
                "retained_positive_probability_nodes":int(np.count_nonzero(self.weight>0)),
                "particles_world_xy_cm":self.xy[ids[::max(1,len(ids)//250)]].tolist()}
