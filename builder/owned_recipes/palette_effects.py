"""D reduction on quantized colors; an approximation of the desktop shaders."""
import colorsys
def effect_word(word,edition,phase):
 rgb=[(word>>(5*i)&31)/31 for i in range(3)];h,l,s=colorsys.rgb_to_hls(*rgb)
 if edition==4:rgb=colorsys.hls_to_rgb((.2-h)%1,1-l,s);rgb=[min(1,v+.8*c/255) for v,c in zip(rgb,[79,99,103])]
 elif edition==1:
  delta=min(max(rgb),max(.5,1-min(rgb)));m=.65+.2*phase;rgb=[max(0,min(1,rgb[0]-delta+delta*m*.3)),max(0,min(1,rgb[1]-delta+delta*m*.3)),max(0,min(1,rgb[2]+delta*m*1.9))]
 elif edition==2:rgb=colorsys.hls_to_rgb((h+.08*phase)%1,min(1,l*.6+.4),min(1,s*1.3))
 elif edition==3:rgb=colorsys.hls_to_rgb((h+.25*phase)%1,l,min(.6,s+.5))
 else:return word
 return sum(round(v*31)<<(5*i) for i,v in enumerate(rgb))
