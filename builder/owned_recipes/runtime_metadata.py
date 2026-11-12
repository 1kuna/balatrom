"""Derive functional rule scalars/associations locally; never execute game code.
No rule values or copied original implementation are bundled here.
"""
from builder.archive import InputError
from builder.lua_literal import parse
from .static_catalog import tokens,fields,initializer


def literal(source, sequence):
    return parse(('return {v='+source[sequence[0][1]:sequence[-1][2]]+'}').encode())['v']


def named_table(data, field):
    source, sequence=tokens(data); matches=[]
    for i in range(len(sequence)-2):
        if [t[0] for t in sequence[i:i+3]]==[field,'=','{']:
            depth=0
            for end in range(i+2,len(sequence)):
                depth+=(sequence[end][0]=='{')-(sequence[end][0]=='}')
                if depth==0:
                    matches.append(sequence[i+2:end+1]);break
    if len(matches)!=1:raise InputError('Required functional table must be unique')
    return source,matches[0]


def string_keyed_tables(sequence):
    result={};i=1
    while i<len(sequence)-1:
        if sequence[i][0] in (',',';'):i+=1;continue
        if [sequence[i][0],sequence[i+2][0],sequence[i+3][0],sequence[i+4][0]]!=['[',']','=','{']:
            raise InputError('Expected string-keyed rule table')
        key=parse(('return {v='+sequence[i+1][0]+'}').encode())['v'];start=i+4;depth=0
        for end in range(start,len(sequence)):
            depth+=(sequence[end][0]=='{')-(sequence[end][0]=='}')
            if depth==0:break
        else:raise InputError('Unclosed rule table')
        if key in result:raise InputError('Duplicate rule key')
        result[key]=sequence[start:end+1];i=end+1
    return result


def derive(data):
    source,table=named_table(data,'hands');hands=string_keyed_tables(table)
    ordered=sorted(hands.items(),key=lambda pair:literal(source,fields(source,pair[1])['order']),reverse=True)
    if len(ordered)!=12:raise InputError('Expected twelve supported hand categories')
    arrays={name:[] for name in ['bc','bm','lc','lm']};names=[]
    for name,body in ordered:
        selected=fields(source,body);names.append(name)
        for target,field in [('bc','chips'),('bm','mult'),('lc','l_chips'),('lm','l_mult')]:
            value=literal(source,selected[field])
            if type(value)!=int or not 0<=value<=255:raise InputError('Rule scalar outside native range')
            arrays[target].append(value)
    source,table=initializer(data,['self','P_CENTERS']);centers=fields(source,table);jokers=[];planets=[]
    for key,body in centers.items():
        selected=fields(source,body)
        kind=literal(source,selected['set']) if 'set' in selected else None
        if kind=='Joker':
            compat=literal(source,selected['blueprint_compat']);order=literal(source,selected['order'])
            if type(compat)!=bool or type(order)!=int:raise InputError('Invalid compatibility metadata')
            jokers.append((order,int(compat)))
        elif kind=='Planet':
            config=fields(source,selected['config']);planets.append((literal(source,selected['order']),literal(source,config['hand_type'])))
    if len(jokers)!=150 or len(planets)!=12:raise InputError('Unsupported functional catalog dimensions')
    mapping={name:index for index,(_,name) in enumerate(sorted(planets))}
    if len(mapping)!=12 or set(mapping)!=set(names):raise InputError('Ambiguous hand-to-planet association')
    arrays['hand_planet']=[mapping[name] for name in names]
    arrays['blueprint_compat']=[compat for _,compat in sorted(jokers)]
    return arrays
