"""Reproducibly copy source minimap and bounded, formerly validated original paths."""
from pathlib import Path
import hashlib
import json
import math
import shutil

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent


def main():
    catalog_path = ROOT / "reports/training-maps.json"
    source_path = ROOT / "profiles/humanlike-hold-positions.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    entry = next(r for r in catalog["maps"] if r["id"] == "ascent")
    holds = json.loads(source_path.read_text(encoding="utf-8"))
    image = ROOT / entry["minimap"]["relative_path"]
    OUT.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(image, OUT / "ascent.png")
    nodes, adjacency, routes = [], [], []
    for row in holds["positions"]:
        if row["id"] not in (1501, 1502):
            continue
        pre = row["source"]["preflight"]
        assert pre["ground_mask"] == 7 and pre["capsule_segments_mask"] == 7
        path = pre["original_path"]
        indexes = []
        for start, end in zip(path[:-1], path[1:]):
            steps = math.ceil(math.dist(start, end) / 25)
            for i in range(steps + 1):
                if i == 0 and indexes:
                    continue
                point = [a+(b-a)*i/steps for a, b in zip(start, end)]
                indexes.append(len(nodes)); nodes.append(point); adjacency.append([])
        for a, b in zip(indexes[:-1], indexes[1:]):
            adjacency[a].append(b); adjacency[b].append(a)
        routes.append({"id": str(row["id"]), "node_indices": indexes,
                       "source_original_path_cm": path, "ground_query_points_cm": pre["ground_query_points"],
                       "original_capsule_radius_cm": pre["capsule_radius_cm"],
                       "historical_source": row["source"]["files"],
                       "scope": "Original path plus local capsule/ground and same-life arrival evidence; current scene rerun required"})
    result = {
        "schema_version": 1, "map_id": "ascent", "name": "Ascent / 亚海悬城", "image": "ascent.png",
        "image_size": [1024, 1024], "image_sha256": hashlib.sha256(image.read_bytes()).hexdigest(),
        "world_to_map_matrix": entry["world_to_image"]["matrix"],
        "map_to_world_matrix": entry["image_to_world"]["matrix"],
        "world_coordinates": "Unreal centimetres: X forward at yaw 0, Y right at yaw 90, Z up",
        "map_coordinates": "Full original uncropped/unrotated image; normalized U right, V down",
        "nodes_world_cm": nodes, "adjacency": adjacency, "routes": routes,
        "coverage": {"status": "bounded_recorded_paths", "full_map_walkability": False,
                     "hard_world_wall_geometry": False, "components": len(routes),
                     "max_graph_spacing_cm": 25,
                     "meaning": "Graph edges only interpolate the two original recorded paths; no shortcut connections or inferred walls",
                     "unknown": "Every location not on a recorded path remains unknown, not classified as an obstacle",
                     "floor_scope": "Path Z retained; no global floor mesh or inferred vertical transitions",
                     "runtime_marker_alignment": "pending new-session minimap landmark validation"},
        "evidence": {"catalog": {"path": str(catalog_path.relative_to(ROOT)), "sha256": hashlib.sha256(catalog_path.read_bytes()).hexdigest()},
                     "paths": {"path": str(source_path.relative_to(ROOT)), "sha256": hashlib.sha256(source_path.read_bytes()).hexdigest()},
                     "projection": "work/training-map-minimap-static/projection.json",
                     "source_texture": entry["minimap"]["texture_evidence"],
                     "source_native_image_sha256": "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"}
    }
    (OUT / "ascent.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"nodes": len(nodes), "components": len(routes), "full_map_walkability": False}))


if __name__ == "__main__":
    main()
