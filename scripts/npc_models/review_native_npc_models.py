"""Close pose/detail review sheets read from candidate O2Rs and a held baseline."""
import argparse
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from preview_native_npc_models import preview


def reviews():
    def panel(variant, pose, label, **options):
        return dict(variant=variant, mode='pose'+str(pose), label=label, options=options)
    p=panel
    saria=[
        p('saria-anime',1,'Anime / playing, eyes closed'),
        p('saria-anime',2,'Anime / seated (old pose 2)'),
        p('saria-anime',3,'Anime / waiting, front',frame=0),
        p('saria-anime',3,'Anime / waiting, back',yaw=180,frame=12),
        p('saria-anime',1,'Ocarina grip / three-quarter',crop='hands',yaw=35),
        p('saria-anime',2,'Seated hand clearance / side',crop='hands',yaw=55),
        p('saria-anime',3,'Waiting hands / back',crop='hands',yaw=180,frame=12),
        p('saria-anime',3,'Waiting feet / side',yaw=70,frame=23,bounds=[-1000,-250,1000,1750]),
        p('saria-render',1,'Render / playing, eyes closed'),
        p('saria-render',2,'Render / seated'),
        p('saria-render',3,'Render / waiting',frame=12),
        p('saria-anime',1,'Placed Saria / dialogue eyes',talking=True,crop='head'),
        p('saria-anime',0,'Anime / arms-at-side wait'),
        p('saria-render',0,'Render / arms-at-side wait'),
        p('saria-anime',4,'Anime / bridge wait'),
        p('saria-anime',4,'Bridge-wait hand / three-quarter',crop='hands',yaw=35),
    ]
    shoulder=[
        p('twins',1,'Previous / sway shoulders',baseline=True,crop='shoulders'),
        p('twins',1,'Revised / sway shoulders',crop='shoulders'),
        p('twins',1,'Revised sway / three-quarter',yaw=35),
        p('twins',2,'Previous / folded shoulders',baseline=True,crop='shoulders'),
        p('twins',2,'Revised / folded shoulders',crop='shoulders'),
        p('twins',2,'Revised folded arms / three-quarter',yaw=35),
    ]
    child=[
        p('young-malon',0,'Idle / arm placement'),
        p('young-malon',2,'Singing / hands over chest'),
        p('young-malon',2,'Native singing animation / clay',native_control=True),
        p('young-malon',2,'Singing hands / front',crop='hands'),
        p('young-malon',2,'Singing hands / three-quarter',crop='hands',yaw=25),
        p('young-malon',0,'Idle arms / three-quarter',yaw=35,bounds=[-900,1800,900,3600]),
        p('young-malon',0,'Previous face and crown',baseline=True,crop='head'),
        p('young-malon',0,'Revised mouth and crown',crop='head'),
        p('young-malon',0,'Rebuilt Bowser clasp',bounds=[-190,2150,190,2530]),
    ]
    adult=[
        p('malon',0,'Idle / relaxed fingers'),
        p('malon',1,'Basket / both hands'),
        p('malon',2,'Singing / hands over chest'),
        p('malon',0,'Idle hands / close',crop='hands',hand_span=1700),
        p('malon',1,'Basket grip / three-quarter',crop='hands',yaw=35),
        p('malon',2,'Singing hands / three-quarter',crop='hands',yaw=25),
        p('malon',2,'Singing hands / front',crop='hands'),
        p('malon',0,'Slightly stronger closed smile',crop='head'),
        p('malon',2,'Native singing hands / clay',native_control=True,crop='hands',yaw=25),
    ]
    return [('Saria_POC3_Review',4,saria),('Kokiri_Shoulders_POC3_Review',3,shoulder),
            ('Young_Malon_POC3_Review',3,child),('Adult_Malon_POC3_Review',3,adult)]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native');parser.add_argument('candidates',type=Path)
    parser.add_argument('baseline',type=Path);parser.add_argument('output',type=Path)
    parser.add_argument('--sheets',nargs='+',help='Render only these sheet basenames')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    tasks=[];groups=reviews()
    if args.sheets:groups=[g for g in groups if g[0] in args.sheets]
    for title,columns,panels in groups:
        for i,panel in enumerate(panels):
            options=panel['options'].copy();old=options.pop('baseline',False)
            options['name']=title+'-'+str(i)
            directory=args.baseline if old else args.candidates
            if old:
                from native_npc_models import VARIANTS
                options['archive']=VARIANTS[panel['variant']]['archive'].replace('POC4','POC2').replace('POC3','POC2')
            tasks.append((args.native,str(directory),panel['variant'],panel['mode'],options))
    with ProcessPoolExecutor(max_workers=2) as pool:
        for name in pool.map(preview,tasks):print('Rendered',name,flush=True)
    try:
        font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',17)
        heading=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',23)
    except OSError:font=heading=ImageFont.load_default()
    for title,columns,panels in groups:
        rows=(len(panels)+columns-1)//columns
        sheet=Image.new('RGB',(columns*480,rows*520+80),(28,33,40));draw=ImageDraw.Draw(sheet)
        draw.text((18,12),title.replace('_',' '),font=heading,fill='white')
        draw.text((18,46),'Actual exported O2R geometry + native/fitted clips | Offline review; runtime untested',font=font,fill=(185,195,207))
        for i,panel in enumerate(panels):
            x,y=(i%columns)*480,(i//columns)*520+80
            directory=args.baseline if panel['options'].get('baseline') else args.candidates
            sheet.paste(Image.open(directory/'previews'/f'{title}-{i}.png'),(x,y))
            draw.text((x+12,y+486),panel['label'],font=font,fill='white')
        dest=args.output/(title+'.png');sheet.save(dest);print('SHEET',dest,flush=True)

if __name__=='__main__':main()
