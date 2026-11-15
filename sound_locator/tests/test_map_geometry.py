import json
import sys
from pathlib import Path
import numpy as np
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from map_geometry import MapGeometry


def test_original_ascent_transform_roundtrip():
    m=MapGeometry.load()
    points=np.array([[0,0],[-6118,-11668],[8218,2668],[4720.668457,-4450.544434]])
    assert np.max(np.abs(m.map_to_world(m.world_to_map(points))-points))<1e-8
    assert np.allclose(m.world_to_map(points[1]),[0,1])
    assert np.allclose(m.world_to_map(points[2]),[1,0])
    assert np.allclose(m.world_yaw_to_map_heading([0,90,180,270]),[0,90,180,270])
    assert np.allclose(m.map_to_world(m.world_to_map(points),z=400)[:,2],400)


def test_every_graph_edge_is_finite_symmetric_and_bounded():
    m=MapGeometry.load()
    assert m.image_path.is_file()
    limit=m.coverage["max_graph_spacing_cm"]
    for i,row in enumerate(m.adjacency):
        assert all(i in m.adjacency[j] for j in row)
        assert all(np.linalg.norm(m.points_world[i]-m.points_world[j])<=limit+1e-6 for j in row)
    points=np.arange(len(m.points_world))
    sampled=m.sample_neighbors(points,np.random.default_rng(7))
    assert all(j==i or j in m.adjacency[i] for i,j in zip(points,sampled))


def test_radius_query_and_nearest():
    m=MapGeometry.load()
    indexes,distance=m.nearest(m.points_world[:3])
    assert np.allclose(distance,0)
    assert np.array_equal(indexes,np.arange(3))
    for i,length in m.reachable_neighbors(0,100).items():
        assert length<=100
        assert np.linalg.norm(m.points_world[0]-m.points_world[i])<=100+1e-6
    with pytest.raises(ValueError):m.reachable_neighbors(-1,10)
    with pytest.raises(ValueError):m.world_to_map([float("nan"),0])


def test_unknown_map_space_is_not_claimed_as_wall():
    m=MapGeometry.load()
    assert m.coverage["full_map_walkability"] is False
    assert m.coverage["hard_world_wall_geometry"] is False
    assert "unknown" in m.coverage
