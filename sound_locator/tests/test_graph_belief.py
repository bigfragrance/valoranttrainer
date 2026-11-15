import sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from direction_field import DirectionObservation
from graph_belief import GraphBeliefTrack
from map_geometry import MapGeometry
from tracking import ListenerPose


def observation(angle):
    angles=np.arange(360.)
    error=(angles-angle+180)%360-180
    score=np.exp(-.5*(error/3)**2)+1e-6
    score/=score.sum()
    return DirectionObservation(angles,score,np.ones(16)/4,1.,angle,[])


def test_graph_support_survives_repeated_bearings_and_rotation_has_no_range():
    m=MapGeometry.load()
    p=ListenerPose(4482,-4268,0,0)
    t=GraphBeliefTrack(1,m,observation(270),p,np.random.default_rng(1))
    for i in range(1,30):
        p=ListenerPose(4482,-4268,i*5,i*.1)
        t.predict(p.time_s);t.update(observation((270-p.yaw_deg)%360),p)
    assert np.all(t.weight>0)
    assert np.isclose(t.weight.sum(),1)
    s=t.summary(3)
    assert not s["position_concentrated"]
    assert s["range_observability"]=="unresolved_bearing_only"
    assert s["candidate_regions"]
    assert not s["representative_is_source_location"]


def test_sparse_motion_has_no_unlisted_edges_and_preserves_mass():
    m=MapGeometry.load();t=GraphBeliefTrack(1,m,observation(270),ListenerPose(4482,-4268,0,0))
    mat=t.graph.transition(t.graph.edge_max+1e-6)
    assert np.allclose(np.asarray(mat.sum(axis=0)).ravel(),1)
    assert np.allclose(mat@t.graph.base,t.graph.base)
    rows,cols=mat.nonzero()
    assert all(i==j or i in m.adjacency[j] for i,j in zip(rows,cols))


def test_noncollinear_exact_bearings_preserve_near_source():
    m=MapGeometry.load()
    source=m.points_world[int(m.nearest([4448.9,-4893.9,410])[0]),:2]
    positions=[[4482,-4268],[4516,-4339],[4550,-4410],[4570,-4440],[4590,-4450],[4687.333333,-4456]]
    track=None
    for i in range(36):
        listener=np.array(positions[min(i//6,5)])
        delta=source-listener
        angle=np.degrees(np.arctan2(delta[1],delta[0]))%360
        pose=ListenerPose(*listener,0,i*.4)
        obs=observation(angle)
        if track is None:track=GraphBeliefTrack(1,m,obs,pose)
        else:track.predict(pose.time_s);track.update(obs,pose)
    assert np.linalg.norm(np.asarray(track.summary(15)["world_xy_cm"])-source)<150
    assert np.all(track.weight>0)
