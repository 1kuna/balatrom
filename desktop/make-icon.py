"""Original geometric card monogram; no font or retail artwork input."""
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parent;sizes=[16,32,128,256,512];icons=root/'app.iconset';icons.mkdir(exist_ok=True)
bg=Image.new('RGBA',(1024,1024),(0,0,0,0));d=ImageDraw.Draw(bg);d.rounded_rectangle((12,12,1012,1012),radius=222,fill='#172b24');d.rounded_rectangle((34,34,990,990),radius=206,outline='#50664c',width=8)
card=Image.new('RGBA',(700,820));c=ImageDraw.Draw(card);c.rounded_rectangle((58,34,642,786),radius=54,fill='#efecdc');c.rectangle((178,164,244,540),fill='#1c362a');c.ellipse((206,164,474,362),fill='#1c362a');c.ellipse((206,326,498,540),fill='#1c362a');c.ellipse((244,215,397,313),fill='#efecdc');c.ellipse((244,380,420,484),fill='#efecdc');c.rectangle((206,215,272,313),fill='#efecdc');c.rectangle((206,380,272,484),fill='#efecdc');c.rectangle((178,164,244,540),fill='#1c362a');c.polygon([(350,598),(408,662),(350,726),(292,662)],fill='#cf7759')
card=card.rotate(8,resample=Image.Resampling.BICUBIC,expand=True);bg.alpha_composite(card,((1024-card.width)//2,(1024-card.height)//2))
for size in sizes:
 for scale in (1,2):bg.resize((size*scale,size*scale),Image.Resampling.LANCZOS).save(icons/f'icon_{size}x{size}{"@2x" if scale==2 else ""}.png')
bg.save(root/'app.icns',format='ICNS')
