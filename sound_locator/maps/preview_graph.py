"""Static scientific verification plot of sampled paths on the original map."""
from pathlib import Path
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from map_geometry import MapGeometry


def main():
    m=MapGeometry.load()
    fig,ax=plt.subplots(figsize=(10,10),dpi=140)
    ax.set_facecolor("#102231")
    ax.imshow(plt.imread(m.image_path),extent=(0,1,1,0))
    lines=[m.points_map[[i,j]] for i,row in enumerate(m.adjacency) for j in row if i<j]
    ax.add_collection(LineCollection(lines,colors="#12b5cc",linewidths=.4,alpha=.65))
    ax.set(xlim=(0,1),ylim=(1,0),xlabel="Original minimap U",ylabel="Original minimap V")
    ax.set_title(f"Ascent: {len(m.points_world):,} sampled nodes, {len(lines):,} original-path segments\n{m.coverage['status']}; gaps are unknown, not inferred walls",fontsize=10)
    fig.tight_layout()
    fig.savefig(Path(__file__).parent/"ascent-graph-preview.png")
    plt.close(fig)


if __name__=="__main__":main()
