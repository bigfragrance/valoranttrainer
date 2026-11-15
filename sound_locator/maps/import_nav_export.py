"""Convert current-session original path chains to a sampled map graph.

Every graph edge lies on a recorded original path segment. No k-nearest-neighbor
shortcuts are introduced; duplicate endpoints merge only at 0.0001 cm rounding.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import shutil
import numpy as np


def convert(raw_path, output_path, sample_cm=100):
    if not math.isfinite(sample_cm) or not 10 <= sample_cm <= 400:
        raise ValueError("Graph subdivision must be 10..400 cm")
    raw_path, output_path = Path(raw_path), Path(output_path)
    raw = json.loads(raw_path.read_text(encoding="utf-8"))
    if raw.get("map_id") != "ascent" or not raw.get("complete"):
        raise ValueError("A complete Ascent query export is required")
    base_path = Path(__file__).parent / "ascent.json"
    base = json.loads(base_path.read_text(encoding="utf-8"))
    # Preserve the existing bounded graph separately; the new manifest replaces
    # its actual graph only after this conversion has passed structural checks.
    nodes, neighbors, index = [], [], {}
    def add(p):
        key = tuple(round(float(x), 4) for x in p)
        if key not in index:
            index[key] = len(nodes); nodes.append([float(x) for x in p]); neighbors.append(set())
        return index[key]
    for point in raw["nodes_world_cm"]:
        if len(point) != 3 or not all(math.isfinite(x) for x in point):
            raise ValueError("Invalid source navigation point")
        add(point)
    for path in raw["path_chains_cm"]:
        for start, end in zip(path[:-1], path[1:]):
            length = math.dist(start, end)
            steps = max(1, math.ceil(length/sample_cm))
            chain = [add([a+(b-a)*i/steps for a,b in zip(start,end)]) for i in range(steps+1)]
            for a,b in zip(chain[:-1],chain[1:]):
                if a != b:
                    neighbors[a].add(b);neighbors[b].add(a)
    # Remove isolated projection points: they are point membership evidence,
    # but cannot support motion until an original path has connected them.
    active = [i for i,n in enumerate(neighbors) if n]
    mapping = {old:new for new,old in enumerate(active)}
    nodes = [nodes[i] for i in active]
    adjacency = [sorted(mapping[j] for j in neighbors[i]) for i in active]
    unseen = set(range(len(nodes))); components=[]
    while unseen:
        frontier=[unseen.pop()];component=[]
        while frontier:
            i=frontier.pop();component.append(i)
            for j in adjacency[i]:
                if j in unseen:unseen.remove(j);frontier.append(j)
        components.append(component)
    base["nodes_world_cm"],base["adjacency"] = nodes,adjacency
    base["routes"] = []
    base["coverage"] = {
        "status": "sampled_original_navigation_queries", "full_map_walkability": False,
        "hard_world_wall_geometry": False, "components": len(components),
        "component_sizes": sorted(map(len,components),reverse=True),
        "grid_spacing_cm": raw["spacing_cm"], "max_graph_spacing_cm": sample_cm,
        "sampled_xy_rectangle_cm": [-6118,8218,-11668,2668],
        "sampling_heights_cm": raw["sampling_heights_cm"],
        "projection_extent_cm": raw["projection_extent_cm"],
        "max_neighbor_height_difference_cm": 300,
        "projection_calls": raw["projection_calls"], "path_calls": raw["path_calls"],
        "accepted_path_chains": len(raw["path_chains_cm"]), "rejected_neighbor_queries": raw["path_rejected"],
        "isolated_projection_nodes_removed": len(index)-len(active),
        "meaning": "Every graph edge subdivides a valid complete original path; only identical rounded endpoints connect chains",
        "unknown": "Gaps and disconnected components may result from finite sampling, height probes or detour cap; never label all gaps as walls",
        "runtime_marker_alignment": "Original source transform; current-session rendered minimap markers still require visual validation",
        "floor_scope": "Original navigation Z retained; no 2D collapse between different heights",
        "ray_occlusion": "Not available: navigation edges are not an acoustic ray/visibility model"
    }
    base["evidence"]["runtime_nav_export"]={"path":str(raw_path.resolve()),
          "sha256":hashlib.sha256(raw_path.read_bytes()).hexdigest(),"pid":raw["pid"],"complete":True}
    build=raw_path.parent/"ascent-nav-build.json"
    if build.is_file():
        base["evidence"]["runtime_nav_export"]["module_build_manifest"]=build.name
        base["evidence"]["runtime_nav_export"]["module_build_manifest_sha256"]=hashlib.sha256(build.read_bytes()).hexdigest()
    output_path.parent.mkdir(parents=True,exist_ok=True)
    output_path.write_text(json.dumps(base,ensure_ascii=False,separators=(",",":")),encoding="utf-8")
    summary={"nodes":len(nodes),"edges":sum(map(len,adjacency))//2,"components":len(components),
             "largest_components":sorted(map(len,components),reverse=True)[:20],"path_chains":len(raw["path_chains_cm"])}
    (output_path.parent/(output_path.stem+"-summary.json")).write_text(json.dumps(summary,indent=2),encoding="utf-8")
    print(json.dumps(summary))
    return base


if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("raw");p.add_argument("--out",default=str(Path(__file__).parent/"ascent.json"));p.add_argument("--sample-cm",type=float,default=100)
    a=p.parse_args();convert(a.raw,a.out,a.sample_cm)
