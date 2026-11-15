"""Candidate history must follow navigation connectivity, never visual shortcuts."""
from pathlib import Path
import sys
from types import SimpleNamespace
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from gui_paths import GraphPathCache


def point(x,y,t):return {"world_xy_cm":[x,y],"time_s":t}


def test_wall_detour_follows_graph_and_respects_speed_even_after_caching():
    geometry=SimpleNamespace(points_world=np.array([[0,0,0],[0,200,0],[200,200,0],[200,0,0]],float),
                             adjacency=[[1],[0,2],[1,3],[2]])
    paths=GraphPathCache(geometry)
    path=paths.between(point(0,0,0),point(200,0,1))
    assert np.array_equal(path,geometry.points_world)
    assert np.linalg.norm(np.diff(path,axis=0),axis=1).sum()==600
    # Straight distance 200 cm fits this bound, but the real 600 cm detour does not.
    assert paths.between(point(0,0,0),point(200,0,.2)) is None
    assert paths.between(point(0,0,1),point(200,0,1)) is None


def test_unknown_disconnected_and_stacked_floor_endpoints_are_not_drawn():
    geometry=SimpleNamespace(points_world=np.array([[0,0,0],[200,0,0],[1000,0,0],[1200,0,0]],float),
                             adjacency=[[1],[0],[3],[2]])
    paths=GraphPathCache(geometry)
    assert paths.between(point(0,0,0),point(1000,0,10)) is None
    assert paths.between(point(0,500,0),point(200,0,10)) is None
    stacked=SimpleNamespace(points_world=np.array([[0,0,0],[200,0,0],[0,0,300],[200,0,300]],float),
                            adjacency=[[1],[0],[3],[2]])
    assert GraphPathCache(stacked).between(point(0,0,0),point(200,0,1)) is None


def test_pair_cache_is_bounded_and_never_changes_navigation_vertices():
    geometry=SimpleNamespace(points_world=np.array([[i*100,0,0] for i in range(5)],float),
                             adjacency=[[1],[0,2],[1,3],[2,4],[3]])
    paths=GraphPathCache(geometry,cache_size=2)
    for i in range(1,5):
        path=paths.between(point(0,0,0),point(i*100,0,2))
        assert np.array_equal(path,geometry.points_world[:i+1])
    assert len(paths.cache)==2
