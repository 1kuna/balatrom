"""Routing from declared diagnostic functional scenes and native cache ABI.

The nine-state corpus uses synthetic geometric patterns. Routing arrays
are built from scene keys; no retail art or text is read.
"""
import re
from builder.scenes import scene_key, SPANS
from builder.native_cache import compile_pair, emit_group, MISS
from .atlas_schema import values as palette_values


def fixture(index):
    alias=3 if index==6 else index
    blank=alias==0
    label,level,chips,mult=('FLUSH','LV9','4096','9999') if alias==5 else ('PAIR','LV2','27','5')
    lw=len(label)*6-1; vw=len(level)*4-1; x=94-(lw+16+vw)//2
    glyphs=[] if blank else [dict(binding='label',x=x,y=48,font=1,colour=19),dict(binding='level',x=x+lw+16,y=49,font=0,colour=12),dict(binding='chips',x=46+(45-6*len(chips))//2,y=67,font=1,colour=19),dict(binding='mult',x=104+(45-6*len(mult))//2,y=67,font=1,colour=19)]
    cards=[] if blank else [dict(x=40,y=92 if alias==2 else 100,kind=2 if alias==3 else 3 if alias==4 else 1,rank=14 if index==6 else 2,suit=3 if index==6 else 0,cues=1 if alias==2 else 0),dict(x=54,y=100,kind=1,rank=3,suit=1,cues=0)]
    jokers=[] if blank else [dict(x=0,kind=2 if alias==3 else 1,id=99 if index==6 else 3)]
    return dict(target='target',jokers=jokers,glyphs=glyphs,cards=cards),dict(target='123456' if alias in (3,4) else '1234',label=label,level=level,chips=chips,mult=mult)


def pattern(which):
    return [[0,(x+y)&3,4+(x+y)%3,(x+3*y)%7,(x+3*y)%10][which] for y in range(16) for x in range(8)]


def registry(atlas_source, descriptor):
    section=atlas_source[atlas_source.index('uint8_t pb_atlas_preclassified('):atlas_source.index('uint8_t pb_atlas(const')]
    primary,supplement=section.split('supplement:',1)
    def routes(text):
        return {int(pair):[int(g) for g in re.findall(r'pb_direct_group_(\d+)\(',line)] for pair,line in re.findall(r'case (\d+):([^\n]+)',text)}
    main,supp=routes(primary),routes(supplement)
    assert set(main)==set(range(180))
    groups=[{} for _ in range(56)]; palettes=palette_values()
    for profile in range(11):
        row=palettes[profile*56:(profile+1)*56];bg=[row[i:i+4] for i in range(0,32,4)];obj=[row[i:i+3] for i in range(32,56,3)]
        for index in range(9):
            frame,texts=fixture(index if index<7 else 0)
            if index>=7:texts['target']='1234' if index==7 else '123456'
            for pair in range(180):
                key=scene_key(frame,pair,profile,texts)
                group=(supp if profile==10 and pair in supp else main)[pair][0]
                if len(key)==5 and group==5:group=19
                record=compile_pair(pattern((sum(key)+17*len(key))%5),bg,obj)
                if key in groups[group]:assert groups[group][key]==record
                groups[group][key]=record
    banks={r['name']:r['bank'] for r in descriptor['objects']};handles={};stats=[]
    for group,records in enumerate(groups):
        if not records:continue
        _,mapped,stat=emit_group(records,group,banks['pb_direct_group_'+str(group)])
        for key,handle in mapped.items():
            assert key not in handles or handles[key]==handle
            handles[key]=handle
        stats.append(stat)
    return handles,stats


def emit(name, values, width):
    assert width in (8,16) and all(type(x) is int and 0<=x<(1<<width) for x in values)
    return ('static const uint%d_t %s[%d]={%s};\n'%(width,name,len(values),','.join(map(str,values)))).encode()


def card_identity(frame,pair):
    first,end=SPANS[pair%20]&15,SPANS[pair%20]>>4
    left=right=511;row=pair//20
    for i in range(first,min(end,len(frame['cards']))):
        card=frame['cards'][i]
        code=(card['rank']-2)*4+card['suit'] if card['kind']==1 else 52 if card['kind']==2 else 53
        if card['cues']&1:code|=64
        if card['cues']&2:code|=128
        if card['cues']&8:code|=256
        y=92 if code&64 else 100
        if row*16>=y+31 or (row+1)*16<=y-5:continue
        if i==first:left=code
        else:right=code
    return left,right


def derive(atlas_source,descriptor):
    handles,stats=registry(atlas_source,descriptor)
    lookup=lambda frame,pair,profile,texts:handles.get(scene_key(frame,pair,profile,texts),MISS)
    files={};static=[]
    for profile in range(11):
        for target in ('1234','123456'):
            frame,texts=fixture(0);texts['target']=target
            for pair in range(180):static.extend(lookup(frame,pair,profile,texts).to_bytes(2,'little'))
    files['generated/pb_direct_static_static_records.inc']=emit('static_records',list(static),8)
    for group,(profile,start,end) in enumerate([(0,105,129),(0,129,154),(0,154,180),(10,105,180)]):
        records=[];starts=[0]
        for pair in range(start,end):
            candidates={}
            if pair%20>=5:
                for index in range(1,7):
                    frame,texts=fixture(index);left,right=card_identity(frame,pair)
                    key=bytes([left&255,((left>>8)&1)|((right&127)<<1),(right>>7)&3])
                    handle=lookup(frame,pair,profile,texts)
                    if handle==MISS:continue
                    assert key not in candidates or candidates[key]==handle
                    candidates[key]=handle
            for key,handle in sorted(candidates.items()):records.extend(key+handle.to_bytes(2,'little'))
            starts.append(len(records)//5)
        prefix='generated/pb_selection_cards_%03d_'%group
        files[prefix+'records.inc']=emit('records',records,8);files[prefix+'starts.inc']=emit('starts',starts,16)
    # A sparse functional corpus. Unknown identities deliberately MISS and
    # retain generic rendering. Vector ordinals follow ascending identity.
    def line_table(profile,role,start,end,pairs):
        known={}
        for index in (1,5):
            frame,texts=fixture(index)
            identity=(102 if index==1 else 509) if role=='label' else int(texts[role])
            if start<=identity<end:known[identity]=[lookup(frame,pair,profile,texts) for pair in pairs]
        vectors=[tuple([MISS]*len(pairs))];ids=[]
        for identity in range(start,end):
            vector=tuple(known.get(identity,vectors[0]))
            if vector not in vectors:vectors.append(vector)
            ids.append(vectors.index(vector))
        return ids,[x for row in vectors for x in row]
    for profile,group in [(0,0),(10,11)]:
        ids,vectors=line_table(profile,'label',0,1301,range(64,80));prefix='generated/pb_selection_lines_label_%03d_'%group
        files[prefix+'ids.inc']=emit('ids',ids,8);files[prefix+'vectors.inc']=emit('vectors',vectors,16)
    for group,profile,role,start,end,pairs in [(1,0,'chips',0,2048,range(85,92)),(3,0,'chips',4096,6144,range(85,92)),(6,0,'mult',0,2048,range(93,100)),(10,0,'mult',8192,10001,range(93,100)),(12,10,'chips',0,2048,range(85,92)),(14,10,'chips',4096,6144,range(85,92)),(17,10,'mult',0,2048,range(93,100)),(21,10,'mult',8192,10001,range(93,100))]:
        ids,vectors=line_table(profile,role,start,end,pairs);prefix='generated/pb_selection_lines_number_%03d_'%group
        files[prefix+'ids.inc']=emit('ids',ids,8);files[prefix+'vectors.inc']=emit('vectors',vectors,16)
    assert len(files)==29
    return files,stats
