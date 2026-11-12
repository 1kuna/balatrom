"""Fresh bounded owner catalog scalars; never execute retail Lua."""
from pathlib import Path
import hashlib,json,re,sys
from builder.archive import Archive,InputError
from builder.lua_literal import parse
from builder.owned_recipes.static_catalog import initializer,fields,catalog

def derive(data):
 s,t=initializer(data,('self','P_CENTERS'));centers=catalog(data);f=fields(s,t);ordered=sorted(((k,c) for k,c in centers.items() if c.get('set')=='Joker'),key=lambda x:x[1]['order']);assert len(ordered)==150
 out={'joker_cost':[],'joker_rarity':[]};keys=[]
 for key,c in ordered:
  selected=fields(s,f[key]);keys.append(key)
  for native,retail in [('joker_cost','cost'),('joker_rarity','rarity')]:
   if retail not in selected:raise InputError('Missing required catalog scalar')
   value=selected[retail];n=parse(('return {v='+s[value[0][1]:value[-1][2]]+'}').encode())['v']
   if type(n)!=int or not 0<=n<=255:raise InputError('Catalog scalar outside native uint8 range')
   out[native].append(n)
 return out,keys
