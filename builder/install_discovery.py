"""Shared UI classification for bounded macOS/Windows Steam discovery.

Only root acquisition is platform-specific. Both fingerprints must match; no
unknown edition is enabled and no game/profile/process is launched or written.
"""
from pathlib import Path
from .input_identity import matches as input_matches
from .archive import Archive, InputError
from .discovery import APP_ID
MAX_FINGERPRINT_CANDIDATES=16

def classify(raw, archive_sha, manifest_sha, *, case_insensitive=False, archive_factory=Archive):
    result={'status':'not_found','selected_game':None,'supported':[],
            'unqualified':[],'warnings':list(raw.get('warnings',[])),
            'game_launched':False,'network_access':False,
            'game_profiles_or_saves_accessed':False,'registry_written':False,'global_scan':False}
    candidates=raw.get('candidates',[])
    if len(candidates)>MAX_FINGERPRINT_CANDIDATES:
        result['status']='discovery_incomplete'
        result['warnings'].append({'reason':'Fingerprint candidate limit exceeded; choose manually'})
        return result
    matches={}
    for candidate in candidates:
        try:
            with archive_factory(candidate['game_path']) as archive:
                fingerprint=archive.fingerprint();archive.assert_unchanged()
                if input_matches({k:fingerprint[k] for k in ('archive_sha256','manifest_sha256')},{'archive_sha256':archive_sha,'manifest_sha256':manifest_sha}):
                    key=str(archive.path);key=key.casefold() if case_insensitive else key
                    matches.setdefault(key,dict(candidate));continue
                reason='Archive or member fingerprint is not a registered supported input'
        except (InputError,OSError,ValueError,UnicodeError) as exc:
            reason='Unreadable or unsupported installed archive: '+str(exc)
        result['unqualified'].append({**candidate,'reason':reason})
    result['supported']=list(matches.values())
    if len(matches)==1:result.update(status='supported',selected_game=next(iter(matches.values()))['game_path'])
    elif matches:result['status']='multiple_supported'
    elif result['unqualified'] or any(Path(w.get('manifest','')).name.casefold()==f'appmanifest_{APP_ID}.acf' for w in result['warnings']):result['status']='found_unqualified'
    elif result['warnings']:result['status']='discovery_incomplete'
    return result


def scan_and_classify(scanner, archive_sha, manifest_sha, **options):
    try:
        raw=scanner()
    except (InputError,OSError,UnicodeError,ValueError) as exc:
        raw={'candidates':[],'warnings':[{'reason':str(exc)}]}
    return classify(raw,archive_sha,manifest_sha,**options)
