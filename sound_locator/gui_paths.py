"""Optional candidate-history paths on the verified navigation graph only.

These paths visualize one possible explanation of successive estimates. They do
not establish a source identity or a measured trajectory.
"""
from collections import OrderedDict
import numpy as np
from scipy.sparse import csr_matrix
from scipy.sparse.csgraph import connected_components,dijkstra
from scipy.spatial import cKDTree


class GraphPathCache:
    def __init__(self,geometry,cache_size=512,snap_limit_cm=75.):
        self.geometry=geometry
        self.points=np.asarray(geometry.points_world,dtype=float)
        self.tree=cKDTree(self.points[:,:2])
        self.cache_size=cache_size;self.snap_limit_cm=snap_limit_cm
        self.cache=OrderedDict()
        rows=[];cols=[];weights=[]
        for i,neighbors in enumerate(geometry.adjacency):
            for j in set(neighbors):
                weight=float(np.linalg.norm(self.points[j]-self.points[i]))
                if weight>0:
                    rows.append(i);cols.append(j);weights.append(weight)
        self.graph=csr_matrix((weights,(rows,cols)),shape=(len(self.points),len(self.points)))
        _,self.components=connected_components(self.graph,directed=False)

    def _node(self,xy):
        xy=np.asarray(xy,dtype=float)
        if xy.shape!=(2,) or not np.isfinite(xy).all():return None
        distance,index=self.tree.query(xy)
        if distance>self.snap_limit_cm:return None
        # A 2D history point cannot choose between stacked floors. Do not guess.
        nearby=self.tree.query_ball_point(xy,max(25.,float(distance)+.01))
        if len(nearby)>1 and np.ptp(self.points[nearby,2])>60:return None
        return int(index),float(distance)

    def between(self,a,b):
        dt=float(b["time_s"])-float(a["time_s"])
        if not np.isfinite(dt) or dt<=0:return None
        left,right=self._node(a["world_xy_cm"]),self._node(b["world_xy_cm"])
        if left is None or right is None:return None
        start,offset_a=left;end,offset_b=right
        if start==end or self.components[start]!=self.components[end]:return None
        limit=750.*dt+150.-offset_a-offset_b
        if limit<=0:return None
        key=(start,end)
        if key in self.cache:
            distance,indices=self.cache[key];self.cache.move_to_end(key)
            if distance>limit:return None
            return self.points[indices].copy()
        distances,previous=dijkstra(self.graph,directed=False,indices=start,limit=limit,return_predecessors=True)
        if not np.isfinite(distances[end]) or distances[end]>limit:return None
        indices=[end]
        while indices[-1]!=start:
            predecessor=int(previous[indices[-1]])
            if predecessor<0:return None
            indices.append(predecessor)
        indices=np.array(indices[::-1],dtype=int)
        self.cache[key]=(float(distances[end]),indices)
        if len(self.cache)>self.cache_size:self.cache.popitem(last=False)
        return self.points[indices].copy()
