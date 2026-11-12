"""Compare save/RNG source contracts only; never boot a ROM or touch SRAM."""
import argparse
import hashlib
import json
from pathlib import Path
import re

NAMES=('game_save','game_load','pg_save','pg_load','native_rng_valid','native_rng_import','native_rng_export')

def hash_text(text): return hashlib.sha256(text.encode()).hexdigest()

def functions(view, fallback=None):
    result={}
    candidates={p.name:p for p in (fallback/'src').glob('*.c')} if fallback else {}
    candidates.update({p.name:p for p in (view/'src').glob('*.c')})
    for path in sorted(candidates.values()):
        text=path.read_text()
        # Preserve character positions while masking comments and string braces.
        masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m[0]),text,flags=re.S)
        for name in NAMES:
            matches=list(re.finditer(r'^[\w* \t]+\b'+name+r'\s*\([^;{}]*\)[^;{}]*\{',masked,re.M))
            for match in matches:
                if name in result: raise ValueError('Multiple definitions: '+name)
                start=match.start(); cursor=match.end(); depth=1
                while depth and cursor<len(masked):
                    depth+=(masked[cursor]=='{')-(masked[cursor]=='}');cursor+=1
                if depth: raise ValueError('Unclosed function: '+name)
                result[name]={'sha256':hash_text(text[start:cursor]),'file':'src/'+path.name,'inherited_base':not path.is_relative_to(view)}
    if set(result)!=set(NAMES): raise ValueError('Missing definitions: '+str(set(NAMES)-set(result)))
    for header in ('game.h','native_run_rng.h'):
        path=view/'include'/header
        if not path.exists() and fallback:
            path=fallback/'include'/header
        result[header]={'sha256':hash_text(path.read_text()),'file':'include/'+header,'inherited_base':not path.is_relative_to(view)}
    return result

def compare(old,new,overlay=False):
    a,b=functions(old),functions(new,old if overlay else None)
    return {'new_source_is_overlay':overlay,'status':'SOURCE_CONTRACT_COMPARISON_ONLY','contracts':{name:{'old':a[name],'new':b[name],'exact':a[name]['sha256']==b[name]['sha256']} for name in a},
            'all_exact':all(a[name]['sha256']==b[name]['sha256'] for name in a),'runtime_verified':False,'user_saves_accessed':False}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('old',type=Path);parser.add_argument('new',type=Path);parser.add_argument('--new-overlay',action='store_true');args=parser.parse_args()
    print(json.dumps(compare(args.old,args.new,args.new_overlay),indent=2))

if __name__=='__main__': main()
