"""Bounded static Lua-field selection. No source evaluation or execution."""
import re
from builder.archive import InputError
from builder.lua_literal import QUOTED_STRING,LONG_START,IDENTIFIER,parse

def tokens(data):
 if len(data)>8*1024*1024:raise InputError('source bound')
 s=data.decode('utf-8-sig');out=[];i=0
 while i<len(s):
  if s[i].isspace():i+=1;continue
  comment=s.startswith('--',i);start=i+2 if comment else i;m=LONG_START.match(s,start)
  if m:
   close=']'+m[1]+']';end=s.find(close,m.end())
   if end<0:raise InputError('unclosed long token')
   if not comment:out.append((s[i:end+len(close)],i,end+len(close)))
   i=end+len(close);continue
  if comment:
   j=s.find('\n',i);i=len(s) if j<0 else j+1;continue
  if s[i] in '\"\'':
   m=QUOTED_STRING.match(s,i)
   if not m:raise InputError('unclosed quote')
   end=m.end()
  else:
   m=IDENTIFIER.match(s,i);end=m.end() if m else i+1
  out.append((s[i:end],i,end));i=end
  if len(out)>400000:raise InputError('token bound')
 return s,out

def initializer(data,path):
 s,t=tokens(data);needle=[]
 for name in path:
  if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name):raise InputError('bad selector')
  if needle:needle.append('.')
  needle.append(name)
 matches=[]
 for i in range(len(t)-len(needle)):
  if [x[0] for x in t[i:i+len(needle)]]==needle and (not i or t[i-1][0] not in ('.',':')):
   j=i+len(needle)
   if t[j][0]=='=' and j+1<len(t) and t[j+1][0]!='=':matches.append(j+1)
 if len(matches)!=1:raise InputError('assignment not unique')
 start=matches[0]
 if t[start][0]!='{':raise InputError('table required')
 depth=0
 for end in range(start,len(t)):
  depth+=(t[end][0]=='{')-(t[end][0]=='}')
  if not depth:
   if end+1<len(t) and t[end+1][0] in ('.','+','-','*','/','(',':','['):raise InputError('trailing expression')
   return s,t[start:end+1]
 raise InputError('unclosed table')

def fields(s,t):
 if t[0][0]!='{' or t[-1][0]!='}':raise InputError('table bounds')
 out={};i=1
 while i<len(t)-1:
  if t[i][0] in (',',';'):i+=1;continue
  name=t[i][0]
  if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*',name) or t[i+1][0]!='=':raise InputError('named field required')
  if name in out:raise InputError('duplicate field')
  start=i+2;i=start;stack=[];pairs={'}':'{',']':'[',')':'('}
  while i<len(t)-1:
   v=t[i][0]
   if v in (',',';') and not stack:break
   if v in ('{','[','('):stack.append(v)
   if v in pairs:
    if not stack or stack.pop()!=pairs[v]:raise InputError('unbalanced expression')
   i+=1
  if stack or i==start:raise InputError('empty/unbalanced field')
  out[name]=t[start:i]
  if len(out)>4096:raise InputError('field bound')
 return out

def catalog(data,path=('self','P_CENTERS')):
 s,t=initializer(data,path);result={}
 for key,value in fields(s,t).items():
  f=fields(s,value);selected={}
  for name in ('name','set','order','pos','atlas'):
   if name in f:
    v=f[name];selected[name]=parse(('return {v='+s[v[0][1]:v[-1][2]]+'}').encode())['v']
  if 'pos' in selected:
   pos=selected['pos']
   if set(pos)!= {'x','y'} or any(type(pos[k])!=int or not 0<=pos[k]<=128 for k in pos):raise InputError('position bounds')
  result[key]=selected
 return result
