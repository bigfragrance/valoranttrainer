import sys
from pathlib import Path
import numpy as np
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tracking import MultiSourceTracker,ListenerPose
from direction_field import DirectionObservation,DirectionField
from core import angular_distance,Profile
from test_core import synthetic


class GridMap:
    def __init__(self):
        axis=np.arange(-2000,2001,200)
        self.points_world=np.array([[x,y,0] for x in axis for y in axis],float)
        n=len(axis)
        self.adjacency=[]
        for k in range(n*n):
            i,j=divmod(k,n)
            self.adjacency.append([a*n+b for a,b in ((i-1,j),(i+1,j),(i,j-1),(i,j+1)) if 0<=a<n and 0<=b<n])
        self.coverage={"status":"synthetic_test_map"}
    def world_to_map(self,xy):return (np.asarray(xy)+2000)/4000


def observation(bearing,heading=0,ambiguity=False):
    angles=np.arange(0,360,3.)
    relative=(bearing-heading)%360
    scores=np.exp(-.5*(angular_distance(angles,relative)/5)**2)
    if ambiguity:scores+=np.exp(-.5*(angular_distance(angles,180-relative)/5)**2)
    scores+=1e-5;scores/=scores.sum()
    return DirectionObservation(angles,scores,np.ones(16)/4,1.,relative,[])


def test_known_rotation_reduces_front_back_ambiguity():
    tracker=MultiSourceTracker(GridMap(),particles_per_track=3500)
    truth=30.
    tracker.update([observation(truth,0,True)],ListenerPose(0,0,0,0))
    t=tracker.tracks[0]
    initial=np.mean(angular_distance(np.degrees(np.arctan2(t.positions()[:,1],t.positions()[:,0])),truth)<15)
    for i,yaw in enumerate([20,40,60,80,100,120],1):
        tracker.update([observation(truth,yaw,True)],ListenerPose(0,0,yaw,.08*i))
    t=tracker.tracks[0]
    angles=np.degrees(np.arctan2(t.positions()[:,1],t.positions()[:,0]))
    final=np.sum(t.weight[angular_distance(angles,truth)<15])
    assert final>initial+.15,(initial,final)
    assert t.summary(.48)["radius90_cm"]>300  # Turning alone is not a range measurement.


def test_more_than_three_sources_no_fixed_ceiling():
    tracker=MultiSourceTracker(GridMap(),particles_per_track=300)
    obs=[observation(a) for a in range(0,360,45)]
    result=tracker.update(obs,ListenerPose(0,0,0,0))
    assert len(result["tracks"])==8
    assert result["fixed_source_limit"] is None
    result=tracker.update(obs,ListenerPose(0,0,0,.1))
    assert len(result["tracks"])==8


def test_tracks_expire_without_evidence_and_time_order_is_checked():
    t=MultiSourceTracker(GridMap(),particles_per_track=100)
    t.update([observation(45)],ListenerPose(0,0,0,0))
    assert not t.update([],ListenerPose(0,0,0,5))["tracks"]
    with pytest.raises(ValueError):t.update([],ListenerPose(0,0,0,4))


def test_graph_prediction_stays_on_edges():
    m=GridMap();t=MultiSourceTracker(m,particles_per_track=200,method="particle")
    t.update([observation(45)],ListenerPose(0,0,0,0))
    for k in range(1,20):
        for track in t.tracks:
            track.predict(k*.08)
            assert all(a==b or b in m.adjacency[a] for a,b in zip(track.src,track.dst))
            assert np.all((track.progress>=0)&(track.progress<=1))


def test_continuous_field_does_not_quantize_to_training_grid():
    p=Profile.train([(a,synthetic(a,a+1,1)) for a in range(0,360,30)])
    f=DirectionField(p)
    r=f.analyze(synthetic(45,773)[:7680])
    assert r["observations"]
    peak=r["angles"][r["scores"].argmax()]
    assert angular_distance(peak,45)<=12
    assert r["source_count"] is None


def test_eight_spectrally_disjoint_sources_are_not_truncated_to_three():
    p=Profile.train([(a,synthetic(a,a+1,1)) for a in range(0,360,15)])
    parts=[]
    for i,a in enumerate(range(0,360,45)):
        low=300+i*1400;high=low+1300
        parts.append(synthetic(a,600+i,mask=lambda f,lo=low,hi=high:(f>=lo)&(f<hi)))
    result=DirectionField(p).analyze(sum(parts)[:7680])
    assert len(result["observations"])>3
    # Only synthetic TF-disjoint coverage; no claim about arbitrary same-band mixtures.
    assert result["number_of_sources_is_not_fixed"]


def test_short_frame_known_rotation_is_compensated():
    p=Profile.train([(a,synthetic(a,a+1,1)) for a in range(0,360,15)])
    headings=[0.,30.,60.]
    pieces=[synthetic(90-h,990+i,.06) for i,h in enumerate(headings)]
    x=np.concatenate(pieces)
    frame_centers=np.arange(0,len(x)-1024+1,256)+512
    yaws=np.array(headings)[np.minimum((frame_centers/(48000*.06)).astype(int),2)]
    result=DirectionField(p).analyze(x,frame_yaws_deg=yaws)
    peak=result["angles"][result["scores"].argmax()]
    assert angular_distance(peak,60)<=12
    assert result["known_rotation_span_deg"]==pytest.approx(60)
