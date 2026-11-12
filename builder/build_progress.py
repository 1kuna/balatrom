"""Private bounded phase/worker trace; factual progress, never an estimated percent."""
import contextvars
import json
import os
from pathlib import Path
import threading
import time

_current=contextvars.ContextVar('build_progress',default=None)

def current():return _current.get()
def phase(stage,message):
    reporter=current()
    if reporter:reporter.phase(stage,message)

class BuildProgress:
    def __init__(self,path):
        self.path=Path(path);self.lock=threading.RLock();self.stop=threading.Event();self.began=time.monotonic();self.since=self.began
        self.stage='prepare';self.message='Preparing sources';self.completed=None;self.total=None;self.active={};self.history=[];self.thread=None;self.outcome=None
    def __enter__(self):
        self.token=_current.set(self);self.emit();self.thread=threading.Thread(target=self.heartbeat,name='build-progress',daemon=True);self.thread.start();return self
    def heartbeat(self):
        while not self.stop.wait(2):self.emit()
    def phase(self,stage,message):
        with self.lock:
            now=time.monotonic();self.history.append({'stage':self.stage,'message':self.message,'seconds':round(now-self.since,3)})
            self.stage=stage;self.message=message;self.since=now
            if stage=='compile':self.completed=0;self.total=198
            self.emit()
    def unit_started(self,name):
        with self.lock:self.active[name]=time.monotonic();self.emit()
    def unit_finished(self,name):
        with self.lock:self.active.pop(name,None)
    def units_completed(self,count):
        with self.lock:self.completed=count;self.emit()
    def emit(self):
        with self.lock:
            now=time.monotonic();value={'schema':1,'stage':self.stage,'message':self.message,'elapsedSeconds':round(now-self.began,1),'phaseSeconds':round(now-self.since,1),'completed':self.completed,'total':self.total,'outcome':self.outcome,'activeUnits':[{ 'name':name,'elapsedSeconds':round(now-began,1)} for name,began in self.active.items()]}
            print('BUILD_STATUS '+json.dumps(value,separators=(',',':')),flush=True)
            payload=dict(value,history=self.history);temporary=self.path.with_name(self.path.name+'.tmp')
            with temporary.open('w',encoding='utf8') as f:json.dump(payload,f,indent=2)
            os.replace(temporary,self.path)
    def __exit__(self,kind,error,tb):
        self.stop.set()
        if self.thread:self.thread.join()
        with self.lock:
            now=time.monotonic();self.history.append({'stage':self.stage,'message':self.message,'seconds':round(now-self.since,3)})
            self.outcome='succeeded' if error is None else 'failed';self.message='Build complete' if error is None else 'Build stopped';self.emit()
        _current.reset(self.token)
