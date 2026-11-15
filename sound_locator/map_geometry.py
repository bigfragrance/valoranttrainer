"""Source-backed map transforms and bounded recorded navigation paths.

Coordinates are Unreal centimetres. Graph membership is limited to the recorded
paths in the manifest; the minimap image is never treated as collision geometry.
"""
from __future__ import annotations
import json
from pathlib import Path
import numpy as np


class MapGeometry:
    def __init__(self, document, path=None):
        self.document = document
        self.path = Path(path) if path is not None else None
        self.map_id = document["map_id"]
        self.points_world = np.asarray(document["nodes_world_cm"], dtype=float)
        self.adjacency = [list(map(int, row)) for row in document["adjacency"]]
        self.world_to_map_matrix = np.asarray(document["world_to_map_matrix"], dtype=float)
        self.map_to_world_matrix = np.asarray(document["map_to_world_matrix"], dtype=float)
        self.coverage = document["coverage"]
        if self.points_world.ndim != 2 or self.points_world.shape[1] != 3:
            raise ValueError("nodes_world_cm must be N x 3")
        if not np.isfinite(self.points_world).all() or len(self.adjacency) != len(self.points_world):
            raise ValueError("Invalid map coordinates or adjacency dimensions")
        for i, row in enumerate(self.adjacency):
            if any(j < 0 or j >= len(self.adjacency) or i not in self.adjacency[j] for j in row):
                raise ValueError("Adjacency must contain valid symmetric node indexes")
        for matrix in (self.world_to_map_matrix, self.map_to_world_matrix):
            if matrix.shape != (2, 3) or not np.isfinite(matrix).all():
                raise ValueError("Map transforms must be finite affine 2 x 3 matrices")
        self.points_map = self.world_to_map(self.points_world)

    @classmethod
    def load(cls, path=None):
        path = Path(path) if path else Path(__file__).parent / "maps" / "ascent.json"
        return cls(json.loads(path.read_text(encoding="utf-8-sig")), path)

    @staticmethod
    def _apply(points, matrix):
        p = np.asarray(points, dtype=float)
        if p.ndim == 0 or p.shape[-1] < 2 or not np.isfinite(p).all():
            raise ValueError("Expected finite XY/XYZ coordinates")
        return p[..., :2] @ matrix[:, :2].T + matrix[:, 2]

    def world_to_map(self, xy_or_xyz):
        """World XY/XYZ centimetres -> full uncropped normalized UV (right/down)."""
        return self._apply(xy_or_xyz, self.world_to_map_matrix)

    def map_to_world(self, uv, z=None):
        """UV -> XY cm; optional supplied z broadcasts to return XYZ cm."""
        xy = self._apply(uv, self.map_to_world_matrix)
        if z is None:
            return xy
        z = np.broadcast_to(np.asarray(z, dtype=float), xy.shape[:-1])
        return np.concatenate((xy, z[..., None]), axis=-1)

    def world_yaw_to_map_heading(self, yaw_deg):
        """Transform direction rather than assuming image rotation is identity."""
        angle = np.deg2rad(yaw_deg)
        d = np.stack((np.cos(angle), np.sin(angle)), axis=-1)
        uv_d = d @ self.world_to_map_matrix[:, :2].T
        return np.rad2deg(np.arctan2(uv_d[..., 0], -uv_d[..., 1])) % 360

    def nearest(self, xy_or_xyz):
        p = np.asarray(xy_or_xyz, dtype=float)
        # Third coordinate participates only when caller supplies it; no floor guessed.
        dims = min(p.shape[-1], 3)
        distance = np.linalg.norm(p[..., None, :dims] - self.points_world[:, :dims], axis=-1)
        indices = distance.argmin(axis=-1)
        return indices, np.take_along_axis(distance, np.asarray(indices)[..., None], axis=-1)[..., 0]

    def sample_neighbors(self, indices, rng=None, include_stay=True):
        """One graph edge per update. Speed/time bounds belong to the caller."""
        rng = rng or np.random.default_rng()
        a = np.asarray(indices, dtype=int)
        if np.any(a < 0) or np.any(a >= len(self.points_world)):
            raise ValueError("Invalid graph node")
        out = [rng.choice(([int(i)] if include_stay else []) + self.adjacency[int(i)])
               if self.adjacency[int(i)] or include_stay else int(i) for i in a.ravel()]
        return np.asarray(out, dtype=int).reshape(a.shape)

    def reachable_neighbors(self, node, max_distance_cm):
        """Dijkstra-radius neighbors, preserving every documented edge restriction."""
        import heapq
        node = int(node)
        if not 0 <= node < len(self.points_world) or not np.isfinite(max_distance_cm) or max_distance_cm < 0:
            raise ValueError("Invalid graph radius query")
        best, pending = {node: 0.0}, [(0.0, node)]
        while pending:
            distance, current = heapq.heappop(pending)
            if distance != best[current]:
                continue
            for neighbor in self.adjacency[current]:
                candidate = distance + float(np.linalg.norm(self.points_world[current]-self.points_world[neighbor]))
                if candidate <= max_distance_cm and candidate < best.get(neighbor, float("inf")):
                    best[neighbor] = candidate
                    heapq.heappush(pending, (candidate, neighbor))
        return best

    @property
    def image_path(self):
        return self.path.parent / self.document["image"] if self.path else None
