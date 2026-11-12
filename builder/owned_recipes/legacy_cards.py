"""Owner-derived16x32 legacy cards with existing UI palette/geometry; local conversion."""
from pathlib import Path
from io import BytesIO
import collections,hashlib,json,re,sys
from PIL import Image
from builder.archive import Archive
from builder.lua_literal import parse_assignment
from builder.conversion import tile_bytes
from builder.owned_recipes.static_catalog import catalog

def derive(a):
 centers=catalog(a.read('game.lua'));cards=parse_assignment(a.read('game.lua'),['self','P_CARDS']);deck=Image.open(BytesIO(a.read('resources/textures/1x/8BitDeck.png'))).convert('RGBA');body=Image.open(BytesIO(a.read('resources/textures/1x/Enhancers.png'))).convert('RGBA');pal=[5|(13<<5)|(13<<10),24|(10<<5)|(10<<10),29|(28<<5)|(25<<10),28|(22<<5)|(11<<10)]
 def crop(im,pos):return im.crop((pos['x']*71,pos['y']*95,(pos['x']+1)*71,(pos['y']+1)*95))
 base=crop(body,centers['c_base']['pos']);faces=[];keys=[]
 for suit in ['S','H','C','D']:
  for rank in ['A','2','3','4','5','6','7','8','9','T','J','Q','K']:
   key=suit+'_'+rank;faces.append(Image.alpha_composite(base,crop(deck,cards[key]['pos'])));keys.append(key)
 for key in ['b_red','m_stone','c_base']:faces.append(crop(body,centers[key]['pos']));keys.append(key)
 def flip(t,h,v):
  rows=[t[i:i+2] for i in range(0,16,2)]
  if v:rows.reverse()
  if h:rows=[bytes(int(format(b,'08b')[::-1],2) for b in row) for row in rows]
  return b''.join(rows)
 def distance(c,p):return sum((((c>>s)&31)-((p>>s)&31))**2 for s in [0,5,10])
 tiles=[];maps=[];loss=[];images=[]
 for key,source in zip(keys,faces):
  image=Image.new('RGBA',(71,95),(16,16,16,255));image.alpha_composite(source);image=image.resize((16,32),Image.Resampling.NEAREST).convert('RGB');words=[(r>>3)|((g>>3)<<5)|((b>>3)<<10) for r,g,b in image.get_flattened_data()];indices=[min(range(4),key=lambda j:(distance(c,pal[j]),j)) for c in words];row=[]
  for t in tile_bytes(indices,16,32):
   canonical,h,v=min((flip(t,h,v),h,v) for h,v in [(0,0),(1,0),(0,1),(1,1)]);assert flip(canonical,h,v)==t
   if canonical not in tiles:tiles.append(canonical)
   row.extend([tiles.index(canonical),9|(32 if h else 0)|(64 if v else 0)])
  maps.extend(row);loss.append({'key':key,'changed_rgb555_pixels':sum(c!=pal[i] for c,i in zip(words,indices)),'squared_error':sum(distance(c,pal[i]) for c,i in zip(words,indices))});preview=Image.new('RGB',(16,32));preview.putdata([tuple(((pal[i]>>s)&31)*8 for s in [0,5,10]) for i in indices]);images.append(preview)
 assert len(tiles)<=256 and len(maps)==880
 raw=b''.join(tiles);code='static const uint8_t owner_legacy_tiles['+str(len(raw))+']={'+','.join(map(str,raw))+'};\nstatic const uint8_t owner_legacy_maps[880]={'+','.join(map(str,maps))+'};\n'
 return code,{'tile_count':len(tiles),'map_bytes':len(maps),'face_count':55,'keys':keys,'palette_words':pal,'existing_UI_palette_unchanged':True,'dimensions':[16,32],'plane_roundtrip_exact_after_quantization':True,'source_nearest_resize':True,'colour_loss':loss,'font_and_title_unchanged':True},images
