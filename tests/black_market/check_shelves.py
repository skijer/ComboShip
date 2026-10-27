#!/usr/bin/env python3
"""Check production shelf geometry against reserved offers and all keeper poses."""
from pathlib import Path
import argparse,json,subprocess,sys,tempfile,zipfile
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
from presentation_preview import shelf_positions,triangle_box_intersects
from native_preview import NativeKeeper
from keeper_lean import room_faces

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('pack',type=Path);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    assert (ROOT/'combo/BlackMarketShelves.h').exists(),'Physical shop shelves are missing; floating item anchors alone do not create shelves'
    with tempfile.TemporaryDirectory(prefix='market-shelves-') as temporary:
        binary=Path(temporary)/'shelves'
        subprocess.run(['c++','-std=c++20','-I'+str(ROOT),str(ROOT/'tools/black_market/shelves_preview.cpp'),'-o',str(binary)],check=True)
        fixture=json.loads(subprocess.check_output([str(binary)],text=True))
    shelves=np.array(shelf_positions(),float)
    relative=np.array([[-17.368482142857143,-15.162767857142857,-16.96848214285714],
                       [17.368482142857143,27.962767857142858,16.96848214285714]])
    reserved=np.array([relative+position for position in shelves])
    with zipfile.ZipFile(a.pack) as archive:
        keeper=NativeKeeper(archive)
        triangles=np.concatenate([keeper.position_triangles(i) for i in range(keeper.frame_count)])
        room=np.array(room_faces(archive),float)
    corners=np.array(fixture['corners'],float)
    # Use the actual rounded native vertices, rather than treating the source
    # axis-aligned boxes as room coordinates after the bank rotation.
    faces=((4,5,6,7),(1,0,3,2),(5,1,2,6),(0,4,7,3),(7,6,2,3),(0,1,5,4))
    board_candidates=[]
    for index,points in enumerate(corners):
        low,high=points.min(0),points.max(0)
        surface=np.array([points[[face[n] for n in indices]] for face in faces for indices in ((0,1,2),(0,2,3))])
        for slot,offer in enumerate(reserved):
            assert not any(triangle_box_intersects(face,*offer) for face in surface), (index,slot,'shelf intersects selected item/seal envelope')
        axes=np.column_stack((points[1]-points[0],points[3]-points[0],points[4]-points[0]))
        inverse=np.linalg.inv(axes)
        local=lambda values,origin=points[0].copy(),matrix=inverse.copy():(np.asarray(values)-origin)@matrix.T
        bounds=local(points)
        lo,hi=bounds.min(0),bounds.max(0)
        # Exclude contact within 0.01 world units; retain all positive overlap.
        epsilon=.01*np.linalg.norm(inverse,axis=1)
        def contacts(values):
            values=local(values)
            candidate=np.all(values.max(1)>=lo+epsilon,axis=1)&np.all(values.min(1)<=hi-epsilon,axis=1)
            return any(triangle_box_intersects(face,lo+epsilon,hi-epsilon) for face in values[candidate])
        assert not contacts(triangles),(index,'shelf intersects keeper pose')
        if index%6!=3:
            assert not contacts(room),(index,'shelf intersects retained room props')
        board_candidates.append((points,local,lo,hi))
    for slot,position in enumerate(shelves):
        boards=[]
        for points,local,lo,hi in board_candidates:
            p=local(position)
            if points[:,1].max()<reserved[slot,0,1] and lo[0]<p[0]<hi[0] and lo[2]<p[2]<hi[2]:
                boards.append(points[:,1].max())
        assert boards,(slot,'item has no shelf beneath it')
        assert reserved[slot,0,1]-max(boards)<5,(slot,'shelf too far below the complete offer envelope')
    a.output.write_text(json.dumps(dict(local_boxes=fixture['boxes'],room_corners=fixture['corners'],shelves=shelves.tolist(),
        reserved_slot_envelopes=reserved.tolist(),keeper_frames=keeper.frame_count,
        physical_shelves_clear_items_and_keeper=True,native_runtime_proven=False),indent=2)+'\n')
    print('PASS physical shelves: every offer has a board; full selected item/seal envelopes and all keeper poses clear shared shelf geometry')

if __name__=='__main__':main()
