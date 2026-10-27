"""Focused Saria review sheets rendered from exact candidate archive versions."""
import argparse
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from native_npc_models import VARIANTS
from preview_native_npc_models import preview


def panels():
    # variant, animation mode, label, render options, previous archive
    anime=[
        ('pose3','Previous waiting tunic',dict(crop='shoulders',yaw=15),True),
        ('pose3','Repaired waiting tunic',dict(crop='shoulders',yaw=15),False),
        ('pose0','Standing shoulder attachment',dict(crop='shoulders'),False),
        ('pose1','Playing / collar and shoulder join',dict(crop='shoulders',yaw=-25,pitch=15),False),
        ('pose3','Previous elbow joins',dict(crop='elbows',yaw=180,joint_span=1500),True),
        ('pose3','Blended elbow joins',dict(crop='elbows',yaw=180,joint_span=1500),False),
        ('pose0','Previous rear calf shading',dict(crop='knees',yaw=180,joint_span=2000),True),
        ('pose0','Repaired rear calf shading',dict(crop='knees',yaw=180,joint_span=2000),False),
    ]
    traditional=[
        ('face0','Previous neutral / lower lashes',dict(),True),
        ('face0','Centered gaze / original lash edge',dict(),False),
        ('pose1','Previous closed eye / angled',dict(crop='head',yaw=35),True),
        ('pose1','Closed eye / retained face contours',dict(crop='head',yaw=35),False),
        ('pose3','Previous waiting tunic and arms',dict(crop='shoulders',yaw=15),True),
        ('pose3','Repaired tunic and sleeve joins',dict(crop='shoulders',yaw=15),False),
        ('pose0','Standing shoulders and cloth',dict(crop='shoulders'),False),
        ('pose1','Playing / corrected hands and ears',dict(crop='shoulders',yaw=-25,pitch=15),False),
    ]
    hands=[]
    for variant,label in [('saria-anime','Anime'),('saria-render','Traditional')]:
        for old in (True,False):
            hands.append((variant,'pose1',label+(' / previous grip' if old else ' / revised finger contact'),
                          dict(crop='hands',hand_span=1100,yaw=-25,pitch=15),old))
    poses=[(variant,mode,label,dict(yaw=yaw),False) for variant,name in
           [('saria-anime','Anime'),('saria-render','Traditional')] for mode,label,yaw in
           [('pose3',name+' / waiting pose',0),('pose3',name+' / waiting hands and feet',160)]]
    return [
        ('Saria_Anime_POC5_Review',[('saria-anime',*row) for row in anime]),
        ('Saria_Traditional_POC5_Review',[('saria-render',*row) for row in traditional]),
        ('Saria_Ocarina_POC5_Review',hands),
        ('Saria_Waiting_POC5_Review',poses),
    ]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('native');p.add_argument('candidate',type=Path)
    p.add_argument('previous',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--sheets',nargs='+')
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    selected=[(title,rows) for title,rows in panels() if not args.sheets or title in args.sheets]
    jobs=[]
    for title,rows in selected:
        for i,(variant,mode,label,opts,old) in enumerate(rows):
            archive=VARIANTS[variant]['archive'].replace('POC4','POC4' if old else 'POC5')
            jobs.append((args.native,str(args.previous if old else args.candidate),variant,mode,
                         dict(opts,archive=archive,name=title+f'-{i}',size=650)))
    with ProcessPoolExecutor(max_workers=2) as pool:
        for name in pool.map(preview,jobs):print('Rendered',name,flush=True)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
    for title,rows in selected:
        sheet=Image.new('RGB',(1300,85+((len(rows)+1)//2)*690),(28,33,40));draw=ImageDraw.Draw(sheet)
        draw.text((16,12),title.replace('_',' '),font=font,fill='white')
        draw.text((16,45),'Actual exported O2R + native animation | Offline review; game runtime untested',font=small,fill='#c3ccd4')
        for i,(_,_,label,_,old) in enumerate(rows):
            x=(i%2)*650;y=85+(i//2)*690
            directory=args.previous if old else args.candidate
            sheet.paste(Image.open(directory/'previews'/f'{title}-{i}.png'),(x,y))
            draw.text((x+12,y+658),label,font=small,fill='white')
        dest=args.output/(title+'.png');sheet.save(dest);print('SHEET',dest,flush=True)


if __name__=='__main__':main()
