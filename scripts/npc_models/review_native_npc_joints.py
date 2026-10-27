"""Focused Saria hand and bend comparisons from actual O2R resources."""
import argparse
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from native_npc_models import VARIANTS
from preview_native_npc_models import preview


def panels():
    hands = []
    groups = []
    for variant, label in [('saria-anime','Anime'),('saria-render','Render')]:
        for mode, text, opts in [
            ('pose1','Ocarina / front',dict(crop='hands',hand_span=1100)),
            ('pose1','Ocarina / raised angle',dict(crop='hands',hand_span=1000,yaw=-35,pitch=20)),
            ('pose0','Relaxed left fingers',dict(crop='hands',hand_side='left',hand_span=760)),
            ('pose0','Relaxed right fingers',dict(crop='hands',hand_side='right',hand_span=760)),
            ('pose3','Waiting palms / back',dict(crop='hands',hand_span=1100,yaw=180)),
            ('pose2','Seated hands / side',dict(crop='hands',hand_span=1100,yaw=55))]:
            hands.append((variant,mode,label+' — '+text,opts,False))
        joints=[]
        for mode, text, opts, before in [
            ('pose3','Previous shoulders / waiting',dict(crop='shoulders'),True),
            ('pose3','Blended shoulders / waiting',dict(crop='shoulders'),False),
            ('pose1','Blended shoulders / playing',dict(crop='shoulders',yaw=35),False),
            ('pose3','Previous elbows / back',dict(crop='elbows',yaw=180,joint_span=1500),True),
            ('pose3','Blended elbows / back',dict(crop='elbows',yaw=180,joint_span=1500),False),
            ('pose2','Blended elbows / seated',dict(crop='elbows',yaw=55,joint_span=1500),False),
            ('pose2','Previous knees / side',dict(crop='knees',yaw=65,joint_span=1800),True),
            ('pose2','Blended knees / side',dict(crop='knees',yaw=65,joint_span=1800),False),
            ('pose2','Blended knees / front',dict(crop='knees',joint_span=1800),False),
            ('pose3','Arm transition / beginning',dict(crop='shoulders',animation='gSariaTransitionHandsSideToBackAnim',frame=0),False),
            ('pose3','Arm transition / middle',dict(crop='shoulders',animation='gSariaTransitionHandsSideToBackAnim',frame=7),False),
            ('pose3','Arm transition / end',dict(crop='shoulders',animation='gSariaTransitionHandsSideToBackAnim',frame=13),False)]:
            joints.append((variant,mode,text,opts,before))
        groups.append(('Saria_'+label+'_Joint_Bends_POC4',joints,3))
    faces=[]
    for mode,label,opts,before in [
        ('face0','Previous forehead / eyelashes',dict(crop='head'),True),
        ('face0','Revised forehead / eyelashes',dict(crop='head'),False),
        ('face0','Revised face / three-quarter',dict(crop='head',yaw=35),False),
        ('face1','Half blink / retained artwork',dict(crop='head'),False),
        ('face2','Closed blink / retained artwork',dict(crop='head'),False),
        ('face0','Revised surface / clay control',dict(crop='head',clay=True),False)]:
        faces.append(('twins',mode,label,opts,before))
    poses=[]
    for mode,label,opts,before in [
        ('pose1','Native sway / side / clay',dict(yaw=90,native_control=True),False),
        ('pose1','Previous fitted sway / side',dict(yaw=90),True),
        ('pose1','Exact native sway / replacement',dict(yaw=90),False),
        ('pose2','Native crossed pose / clay',dict(native_control=True),False),
        ('pose2','Revised crossed pose',dict(),False),
        ('pose2','Revised crossed pose / side',dict(yaw=55),False),
        ('pose2','Previous shoulder attachment',dict(crop='shoulders'),True),
        ('pose2','Blended shoulder attachment',dict(crop='shoulders'),False),
        ('pose1','Sway shoulder attachment / back',dict(crop='shoulders',yaw=160),False),
        ('pose2','Previous crossed hands',dict(crop='hands',hand_span=1300),True),
        ('pose2','Curled hands / forearm contact',dict(crop='hands',hand_span=1300),False),
        ('pose2','Curled hands / side',dict(crop='hands',hand_span=1300,yaw=50),False)]:
        poses.append(('twins',mode,label,opts,before))
    return [('Saria_Hands_POC4',hands,3),*groups,('Kokiri_Face_POC4',faces,3),('Kokiri_Poses_POC4',poses,3)]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('native');p.add_argument('candidate',type=Path);p.add_argument('previous',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--sheets',nargs='+')
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    jobs=[]
    for title, items, columns in panels():
        if args.sheets and title not in args.sheets:continue
        for i,(variant,mode,label,opts,before) in enumerate(items):
            opts=dict(opts,name=title+f'-{i}',size=480)
            if before: opts['archive']=VARIANTS[variant]['archive'].replace('POC4','POC3')
            jobs.append((args.native,str(args.previous if before else args.candidate),variant,mode,opts))
    with ProcessPoolExecutor(max_workers=2) as pool:
        for name in pool.map(preview,jobs):print('Rendered',name,flush=True)
    compose(args.candidate,args.previous,args.output,args.sheets)


def compose(candidate,previous,output,names=None):
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
    small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',15)
    for title,items,columns in panels():
        if names and title not in names:continue
        rows=(len(items)+columns-1)//columns
        sheet=Image.new('RGB',(columns*480,80+rows*515),(28,33,40));d=ImageDraw.Draw(sheet)
        d.text((16,12),title.replace('_',' '),font=font,fill='white')
        d.text((16,42),'Decoded O2R geometry + live weighted matrices | Offline review; game runtime untested',font=small,fill='#b9c4ce')
        for i,(variant,mode,label,opts,before) in enumerate(items):
            x=(i%columns)*480;y=80+(i//columns)*515
            directory=previous if before else candidate
            sheet.paste(Image.open(directory/'previews'/f'{title}-{i}.png'),(x,y))
            d.text((x+10,y+485),label,font=small,fill='white')
        target=output/(title+'.png');sheet.save(target);print('SHEET',target,flush=True)


if __name__=='__main__':main()
