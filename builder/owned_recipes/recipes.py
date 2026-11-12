"""Deterministic asset conversion from a supported user-owned archive."""
from __future__ import annotations
from collections import Counter
from io import BytesIO
from pathlib import Path
from typing import Any
import itertools,json,re,time
from PIL import Image
from builder.conversion import tile_bytes,native_records,text_table
from builder.lua_literal import parse,parse_assignment
from .static_catalog import catalog
from .palette_effects import effect_word

def card_data(a: Any, centers: dict[str,Any]) -> dict[str,list[int]]:
 cards=parse_assignment(a.read('game.lua'),['self','P_CARDS'])
 deck_member='resources/textures/1x/8BitDeck.png';body_member='resources/textures/1x/Enhancers.png'
 def image(member):
  im=Image.open(BytesIO(a.read(member)));im.load();return im.convert('RGBA')
 deck=image(deck_member);body=image(body_member)
 def crop(im,pos):return im.crop((pos['x']*71,pos['y']*95,(pos['x']+1)*71,(pos['y']+1)*95))
 base=crop(body,centers['c_base']['pos']);faces=[];rows=[]
 # Retained face index is native suit index*13 + ace-first rank. Public
 # game suits0..3 route via{0,3,1,2}, hence S,C,D,H order here.
 for suit in ('S','C','D','H'):
  for rank in ('A','2','3','4','5','6','7','8','9','T','J','Q','K'):
   key=suit+'_'+rank;front=crop(deck,cards[key]['pos']);composed=Image.alpha_composite(base,front)
   flat=Image.new('RGBA',(71,95),(16,16,16,255));flat.alpha_composite(composed)
   resized=flat.resize((24,32),Image.Resampling.NEAREST).convert('RGB')
   pixels=[(r>>3)|((g>>3)<<5)|((b>>3)<<10) for r,g,b in resized.get_flattened_data()];faces.append(pixels)
   rows.append({'face':len(faces)-1,'key':key,'front_pos':cards[key]['pos'],'base_pos':centers['c_base']['pos'],'member_dependencies':[deck_member,body_member,'game.lua']})
 for key in ('b_red','m_stone','c_base'):
  flat=Image.new('RGBA',(71,95),(16,16,16,255));flat.alpha_composite(crop(body,centers[key]['pos']))
  faces.append([(r>>3)|((g>>3)<<5)|((b>>3)<<10) for r,g,b in flat.resize((24,32),Image.Resampling.NEAREST).convert('RGB').get_flattened_data()]);rows.append({'face':len(faces)-1,'key':key,'pos':centers[key]['pos'],'member_dependencies':[body_member,'game.lua']})
 assert len(faces)==55
 palettes=[];dark=(16>>3)|((16>>3)<<5)|((16>>3)<<10)
 for family in range(4):
  counts=Counter(c for pixels in faces[family*13:(family+1)*13] for c in pixels)
  colours=[c for c,_ in sorted(counts.items(),key=lambda p:(-p[1],p[0])) if c!=dark][:3]
  assert len(colours)==3;palettes.append([dark,*colours])
 def distance(a,b):return sum((((a>>shift)&31)-((b>>shift)&31))**2 for shift in (0,5,10))
 def flip_tile(tile,h,v):
  rows=[tile[i:i+2] for i in range(0,16,2)]
  if v:rows.reverse()
  if h:rows=[bytes(int(format(b,'08b')[::-1],2) for b in row) for row in rows]
  return b''.join(rows)
 # Palette index relabeling preserves the exact chosen RGB555 colour of
 # every pixel. Search only the six permutations of nonbackground entries.
 original_palettes=[p[:] for p in palettes];permutations=list(itertools.permutations((1,2,3)))
 options=[];search_start=time.monotonic()
 for family in range(4):
  source_faces=faces[family*13:(family+1)*13]+(faces[52:] if family==0 else [])
  pal=original_palettes[family];baseline=[]
  for pixels in source_faces:
   lookup={c:min(range(4),key=lambda i:(distance(c,pal[i]),i)) for c in set(pixels)}
   baseline.append([lookup[c] for c in pixels])
  family_options=[]
  for perm in permutations:
   order=(0,*perm);renumber={old:order.index(old) for old in range(4)};unique=set()
   for pixels in baseline:
    for tile in tile_bytes([renumber[c] for c in pixels],24,32):unique.add(min(flip_tile(tile,h,v) for h,v in ((0,0),(1,0),(0,1),(1,1))))
   family_options.append(unique)
  options.append(family_options)
 best=None
 for choice in itertools.product(range(6),repeat=4):
  assert time.monotonic()-search_start<60,'bounded permutation search'
  count=len(set.union(*(options[f][choice[f]] for f in range(4))))
  candidate=(count,choice)
  if best is None or candidate<best:best=candidate
 palettes=[[original_palettes[f][i] for i in (0,*permutations[best[1][f]])] for f in range(4)]
 tiles=[];maps=[];corners=[];losses=[]
 for face,pixels in enumerate(faces):
  family=face//13 if face<52 else 0;pal=palettes[family];oldpal=original_palettes[family];lookup={c:pal.index(oldpal[min(range(4),key=lambda i:(distance(c,oldpal[i]),i))]) for c in set(pixels)}
  losses.append({'face':face,'source_rgb555_colours':len(set(pixels)),'changed_pixels':sum(pal[lookup[c]]!=c for c in pixels),'sum_squared_rgb555_error':sum(distance(c,pal[lookup[c]]) for c in pixels)})
  ids=[]
  for t in tile_bytes([lookup[c] for c in pixels],24,32):
   canon,h,v=min((flip_tile(t,h,v),h,v) for h,v in ((0,0),(1,0),(0,1),(1,1)))
   assert flip_tile(canon,h,v)==t # inverse mirror reproduces every plane byte
   if canon not in tiles:tiles.append(canon)
   ids.append((tiles.index(canon),(h*32)|(v*64)))
  maps.append([(i if i<130 else i+15 if i<147 else i+17 if i<175 else i+59 if i<177 else i+69,8+family+flags) for i,flags in ids])
  corners.extend(maps[-1][i] for i in (0,2,9,11))
 while len(tiles)<180:tiles.append(bytes(16))
 return {'a1_card_tiles':list(b''.join(tiles)), 'a1_card_maps':[b for row in maps for pair in row for b in pair], 'a1_card_ink_corners':[b for pair in corners for b in pair], 'native_scene_card_palettes':sum(palettes,[])}

def distinct(p: list[int]) -> list[int]:
 mapping={};used=set();adjusted=[]
 for src,dst in zip(p[0],p[1]):
  if src in mapping:adjusted.append(mapping[src]);continue
  if dst in used:
   rgb=[(dst>>(5*i))&31 for i in range(3)];candidates=[]
   for shift in itertools.product(range(-2,3),repeat=3):
    col=[c+d for c,d in zip(rgb,shift)]
    if all(0<=c<=31 for c in col):
     value=sum(c<<(5*i) for i,c in enumerate(col))
     if value not in used:candidates.append((sum(abs(d) for d in shift),sum(d*d for d in shift),value))
   dst=min(candidates)[2]
  used.add(dst);mapping[src]=dst;adjusted.append(dst)
 return adjusted

def flatten(x: Any) -> list[int]:
 return [n for y in x for n in flatten(y)] if isinstance(x,list) else [x]

def numeric_arrays(s: str) -> dict[str,list[int]]:
 return {name:[int(n,0) for n in re.findall(r'0x[0-9a-fA-F]+|\d+',body)] for name,body in re.findall(r'\b(?:static )?const\s+\w+\s+(\w+)(?:\[[^]]*\])+\s*=\s*\{(.*?)\};',s,re.S)}

def derive(a: Any, descriptor: dict[str,Any]) -> tuple[dict[str,bytes],dict[str,list[int]],dict[str,str]]:
 game=a.read('game.lua');centers=catalog(game);loc=parse(a.read('localization/en-us.lua'));files={};arrays={};members={name:a.read(name) for name in ['game.lua','localization/en-us.lua','resources/textures/1x/Jokers.png','resources/textures/1x/Tarots.png','resources/textures/1x/8BitDeck.png','resources/textures/1x/Enhancers.png']};banks={x['name']:x['bank'] for x in descriptor['objects']};selected={}
 for kind,n in [('Joker',150),('Tarot',22),('Planet',12),('Spectral',18)]:
  selected[kind]=sorted([(k,v) for k,v in centers.items() if v.get('set')==kind],key=lambda x:x[1]['order'])
  if len(selected[kind])!=n:raise ValueError('Unsupported catalog shape: '+kind)
 icons={}
 specs=[('Joker','sidebar_jokers_0',selected['Joker'][:75],'sidebar_joker_tiles_0','sidebar_joker_palettes_0'),('Joker','sidebar_jokers_1',selected['Joker'][75:],'sidebar_joker_tiles_1','sidebar_joker_palettes_1'),('Tarot','sidebar_tarot',selected['Tarot'],'sidebar_tarot_tiles','sidebar_tarot_palettes'),('Planet','sidebar_planet',selected['Planet'],'sidebar_planet_tiles','sidebar_planet_palettes'),('Spectral','sidebar_spectral',selected['Spectral'],'sidebar_spectral_tiles','sidebar_spectral_palettes')]
 for kind,unit,rows,ts,ps in specs:
  member='resources/textures/1x/Jokers.png' if kind=='Joker' else 'resources/textures/1x/Tarots.png';raw=members[member]
  with Image.open(BytesIO(raw)) as im:dimensions=list(im.size)
  recipe={'symbol':'retail_'+unit,'tile_symbol':'retail_'+ts,'palette_symbol':'retail_'+ps,'bank':banks[unit],'dimensions':dimensions,'crops':[[v['pos']['x']*71,v['pos']['y']*95,71,95] for k,v in rows],'background':[0,0,0],'resample':'nearest'}
  code,_=native_records(raw,recipe);body=code['retail_'+unit+'.c'].decode().replace('retail_','');files['src/'+unit+'.c']=body.encode();icons[unit]=numeric_arrays(body)
 for kind,symbol in [('Joker','joker_names'),('Tarot','tarot_labels'),('Planet','planet_labels'),('Spectral','spectral_labels')]:
  code,_=text_table(loc,{'symbol':symbol,'selectors':[['descriptions',kind,k,'name'] for k,v in selected[kind]],'max_bytes':32});files['generated/retail_text_'+symbol+'.inc']=code.decode().replace('const char *const ','static const char *const ').encode()
 for part in [0,1]:
  arrays['table_1998_jokers_'+str(part)]=icons['sidebar_jokers_'+str(part)]['sidebar_joker_tiles_'+str(part)]
  arrays['native_record_palettes_'+str(part)]=icons['sidebar_jokers_'+str(part)]['sidebar_joker_palettes_'+str(part)]
 arrays['felt_item_tiles']=sum([icons['sidebar_'+kind]['sidebar_'+kind+'_tiles'] for kind in ['planet','tarot','spectral']],[])
 arrays['native_item_palettes']=sum([icons['sidebar_'+kind]['sidebar_'+kind+'_palettes'] for kind in ['planet','tarot','spectral']],[])
 arrays['native_negative_item_palettes']=[effect_word(w,4,0) for w in arrays['native_item_palettes']]
 cards=card_data(a,centers);arrays.update(cards)
 # Relocate palette attributes to the renderer palette slots; image planes unchanged.
 for key in ['a1_card_maps','a1_card_ink_corners']:arrays[key]=[x if not i&1 else (x&~7)+((x&7)+3) for i,x in enumerate(arrays[key])]
 palettes=arrays['native_record_palettes_0']+arrays['native_record_palettes_1'];palettes=[palettes[i:i+4] for i in range(0,len(palettes),4)];fixkeys=[];fixwords=[];variants=[]
 for i,pal in enumerate(palettes):
  row=[pal]
  for ed in range(1,5):
   for ph in range(1 if ed==4 else 4):
    raw=[effect_word(w,ed,ph) for w in pal];q=distinct([pal,raw]);row.append(q)
    if q!=raw:fixkeys.extend([i,(ed-1)*4+ph]);fixwords.extend(q)
  variants.append(row)
 arrays['de_fix_keys']=fixkeys;arrays['de_fix_words']=fixwords
 arrays['np_palette_0']=flatten(variants[:75]);arrays['np_palette_1']=flatten(variants[75:])
 sources=sorted(set(flatten(palettes)+cards['native_scene_card_palettes']));arrays['de_sources']=sources;arrays['de_words']=[effect_word(w,e,p) for w in sources for e,p in [(e,p) for e in [1,2,3] for p in range(4)]+[(4,0)]]
 matrix=[]
 for ed in range(5):
  for suit in range(4):
   for ph in range(4):
    pal=cards['native_scene_card_palettes'][suit*4:suit*4+4];q=[effect_word(w,ed,ph) if ed else w for w in pal];used=set()
    for i,dst in enumerate(q):
     if dst in used:
      rgb=[(dst>>(5*k))&31 for k in range(3)];opts=[]
      for ds in itertools.product(range(-2,3),repeat=3):
       xyz=[x+y for x,y in zip(rgb,ds)]
       if all(0<=x<=31 for x in xyz):
        n=sum(x<<(5*k) for k,x in enumerate(xyz))
        if n not in used:opts.append((sum(abs(x) for x in ds),sum(x*x for x in ds),n))
      q[i]=min(opts)[2]
     used.add(q[i])
    matrix.extend(q)
 arrays['dh_words']=matrix
 import hashlib
 return files,arrays,{name:hashlib.sha256(raw).hexdigest() for name,raw in members.items()}
