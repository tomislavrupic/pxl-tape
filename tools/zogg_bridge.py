#!/usr/bin/env python3
"""PXL Tape's local-only adapter to the configured Zogg MCP. Python3.11+."""
import urllib.request
import argparse,json,math,mimetypes,os,secrets,select,shlex,shutil,subprocess,threading,time,tomllib,uuid,webbrowser
from http.cookies import SimpleCookie
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit,unquote

def command():
 if os.environ.get('ZOGG_MCP_COMMAND'):return shlex.split(os.environ['ZOGG_MCP_COMMAND'])+shlex.split(os.environ.get('ZOGG_MCP_ARGS',''))
 config=Path(os.environ.get('ZOGG_CODEX_CONFIG',str(Path.home()/'.codex/config.toml')))
 if config.is_file():
  spec=tomllib.loads(config.read_text()).get('mcp_servers',{}).get('zogg-sound-audit',{})
  if spec.get('command'):return [spec['command'],*spec.get('args',[])]
 exe=shutil.which('zoggs-sound-audit-mcp')
 if exe:return [exe]
 raise RuntimeError('Zogg MCP is not configured. Set ZOGG_MCP_COMMAND and ZOGG_MCP_ARGS, or use your existing Codex Zogg connection.')

class MCP:
 def __init__(self):self.lock=threading.Lock()
 def __call__(self,name,args):
  if not self.lock.acquire(timeout=60):raise RuntimeError('Zogg is busy. Retry after the current preview finishes.')
  try:
   p=subprocess.Popen(command(),stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,bufsize=0)
   try:
    pending=b'';serial=0
    def send(method,params,notify=False):
     nonlocal serial,pending
     serial+=1;msg={'jsonrpc':'2.0','method':method,'params':params}
     if not notify:msg['id']=serial
     p.stdin.write((json.dumps(msg)+'\n').encode());p.stdin.flush()
     if notify:return
     deadline=time.monotonic()+60
     while time.monotonic()<deadline:
      if b'\n' not in pending:
       ready,_,_=select.select([p.stdout],[],[],max(0,deadline-time.monotonic()))
       if not ready:break
       chunk=os.read(p.stdout.fileno(),65536)
       if not chunk:raise RuntimeError('Zogg MCP stopped. Check its configuration and mounted libraries.')
       pending+=chunk
       if len(pending)>8*1024*1024:raise RuntimeError('Zogg response exceeds8MB')
       continue
      line,pending=pending.split(b'\n',1);r=json.loads(line)
      if r.get('id')==serial:
       if 'error' in r:raise RuntimeError(str(r['error']))
       return r.get('result',{})
     raise RuntimeError('Zogg timed out. Check that your sample drives are connected.')
    send('initialize',{'protocolVersion':'2024-11-05','capabilities':{},'clientInfo':{'name':'pxl-tape','version':'0.6.0'}});send('notifications/initialized',{},True)
    result=send('tools/call',{'name':name,'arguments':args})
    for block in result.get('content',[]):
     if block.get('type')=='text':
      if result.get('isError'):raise RuntimeError(block['text'])
      return json.loads(block['text'])
    raise RuntimeError('Zogg returned no sound data')
   finally:
    if p.poll() is None:p.terminate()
    try:p.wait(timeout=2)
    except subprocess.TimeoutExpired:p.kill();p.wait()
  finally:self.lock.release()

def number(data,key,default,lo,hi,integer=False):
 v=data.get(key,default)
 if isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) or v<lo or v>hi or (integer and int(v)!=v):raise ValueError('Invalid '+key)
 return int(v) if integer else float(v)

def make_server(port,token,dist,output,call=None):
 dist=dist.resolve();output=output.resolve()
 class Handler(BaseHTTPRequestHandler):
  def log_message(self,*args):pass
  def respond(self,status,data,kind='application/json'):
   body=json.dumps(data).encode() if kind=='application/json' and not isinstance(data,(bytes,bytearray)) else data
   self.send_response(status);
   if getattr(self,'pairing',False):self.send_header('Set-Cookie',f'pxl-zogg={token}; HttpOnly; SameSite=Strict; Path=/')
   self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(body)));self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.send_header('Referrer-Policy','no-referrer');self.end_headers();self.wfile.write(body)
  def allowed(self,api=False):
   host=self.headers.get('Host','');origin=self.headers.get('Origin')
   if host!=f'127.0.0.1:{self.server.server_port}' or (origin and origin!=f'http://{host}'):
    self.respond(403,{'error':'Use the local PXL Tape bridge window'});return False
   cookie=SimpleCookie();cookie.load(self.headers.get('Cookie',''));credential=self.headers.get('Authorization','') or ('Bearer '+cookie['pxl-zogg'].value if 'pxl-zogg' in cookie else '')
   if api and not secrets.compare_digest(credential,'Bearer '+token):
    self.respond(401,{'error':'Open the connection link printed by the Zogg bridge'});return False
   return True
  def do_GET(self):
   path=urlsplit(self.path).path
   if not self.allowed(path.startswith('/audio/') or path.startswith('/api/')):return
   if path.startswith('/audio/'):
    file=self.server.files.get(path)
    if file is None or not file.is_file():self.respond(404,{'error':'Preview expired. Preview the sound again.'});return
    self.respond(200,file.read_bytes(),'audio/wav');return
   file=(dist/unquote(path).lstrip('/')).resolve() if path!='/' else dist/'index.html'
   if not file.is_relative_to(dist) or not file.is_file() or file.suffix not in {'.html','.js','.css','.wasm','.svg','.json','.zip'}:self.respond(404,{'error':'Not found'});return
   body=file.read_bytes()
   if file.name=='index.html':body=body.replace(b'<html ',b'<html data-zogg-local="true" ',1)
   self.respond(200,body,mimetypes.guess_type(str(file))[0] or 'application/octet-stream')
  def do_POST(self):
   if not self.allowed(True):return
   try:
    length=int(self.headers.get('Content-Length','0'))
    if length<0 or length>8192:raise ValueError('Request exceeds8KB')
    data=json.loads(self.rfile.read(length) or b'{}')
    if not isinstance(data,dict):raise ValueError('Expected JSON object')
    path=urlsplit(self.path).path
    if path=='/api/pair':self.pairing=True;self.respond(200,{'paired':True});return
    if path=='/api/status':self.respond(200,self.server.call('library_status',{}));return
    if path=='/api/search':
     query=data.get('query','')
     if not isinstance(query,str) or len(query)>256:raise ValueError('Search text exceeds256 characters')
     args={'query':query,'available':True,'offset':number(data,'offset',0,0,1000000,True),'limit':number(data,'limit',25,1,50,True),'sort':data.get('sort','name')}
     if args['sort'] not in ['name','shortest','longest','recently_modified']:raise ValueError('Invalid sort')
     if data.get('library_id') is not None:args['library_id']=number(data,'library_id',0,1,1000000,True)
     if data.get('maximum_duration_seconds') is not None:args['maximum_duration_seconds']=number(data,'maximum_duration_seconds',30,.01,86400)
     self.respond(200,self.server.call('search_sounds',args));return
    if path not in ['/api/inspect','/api/preview','/api/prepare']:self.respond(404,{'error':'Not found'});return
    asset=number(data,'asset_id',0,1,2**53-1,True);details=self.server.call('get_sound',{'asset_id':asset})
    if not details.get('source',{}).get('available_and_unchanged'):raise ValueError('Source unavailable or changed. Refresh the Zogg catalogue and reconnect its drive.')
    expected=data.get('expected_fingerprint')
    if expected is not None and (not isinstance(expected,str) or len(expected)!=64 or any(c not in '0123456789abcdef' for c in expected) or expected!=details['source'].get('current_fingerprint')):raise ValueError('Factory source fingerprint differs. Current sound preserved; refresh your Zogg library.')
    public={'asset':details['asset']}
    if path=='/api/inspect':self.respond(200,public);return
    start=number(data,'start',0,0,86400);duration=number(data,'duration',10,.01,60 if path=='/api/preview' else 300)
    if start>=details['asset']['duration_seconds']:raise ValueError('Trim start is beyond the sound')
    args={'asset_id':asset,'duration_seconds':duration,'output_directory':str(output)}
    args['start_seconds' if path=='/api/preview' else 'trim_start_seconds']=start
    if expected is not None and path=='/api/prepare':args['processing']={'fade_in_seconds':.0005,'fade_out_seconds':.006,'gain_db':-3}
    result=self.server.call('preview_sound' if path=='/api/preview' else 'prepare_sound',args)
    receipt=result['preview' if path=='/api/preview' else 'prepared'];file=Path(receipt['output_path']).resolve()
    if not file.is_relative_to(output) or not file.is_file() or file.suffix.lower()!='.wav' or file.stat().st_size>180*1024*1024:raise ValueError('Invalid Zogg audio output')
    handle='/audio/'+uuid.uuid4().hex;self.server.files[handle]=file
    while len(self.server.files)>256:self.server.files.pop(next(iter(self.server.files)))
    public.update(audio=handle,offset=start,duration=receipt.get('output_duration_seconds'),fingerprint=receipt.get('source_fingerprint'));self.respond(200,public)
   except (BrokenPipeError,ConnectionResetError):pass
   except (ValueError,KeyError,TypeError,json.JSONDecodeError) as e:self.respond(400,{'error':str(e)})
   except Exception as e:self.respond(503,{'error':str(e)})
 server=ThreadingHTTPServer(('127.0.0.1',port),Handler);server.daemon_threads=True;server.files={};server.call=call or MCP();return server

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=4186);parser.add_argument('--no-open',action='store_true');parser.add_argument('--session-file',type=Path,default=Path.home()/'Library/Application Support/Pixel Records/PXL Tape/ZoggBridge.json');parser.add_argument('--dist',type=Path,default=Path(__file__).resolve().parents[1]/'dist');args=parser.parse_args()
 output=args.session_file.parent/'ZoggCache';output.mkdir(parents=True,exist_ok=True);token=secrets.token_urlsafe(32)
 try:server=make_server(args.port,token,args.dist.resolve(),output.resolve())
 except OSError:
  existing=json.loads(args.session_file.read_text());base=f'http://127.0.0.1:{args.port}'
  if existing.get('base')!=base:raise RuntimeError('Bridge port is occupied. Close the existing bridge window and retry.')
  request=urllib.request.Request(base+'/api/status',data=b'{}',headers={'Authorization':'Bearer '+existing['token'],'Content-Type':'application/json'})
  with urllib.request.urlopen(request,timeout=5) as response:
   if 'summary' not in json.load(response):raise RuntimeError('Unexpected service on the bridge port')
  url=base+'/#zogg='+existing['token'];print('PXL Tape Zogg bridge is already running: '+url,flush=True)
  if not args.no_open:webbrowser.open(url)
  return
 session={'base':f'http://127.0.0.1:{server.server_port}','token':token};args.session_file.parent.mkdir(parents=True,exist_ok=True)
 fd=os.open(args.session_file,os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600);os.fchmod(fd,0o600)
 with os.fdopen(fd,'w') as file:json.dump(session,file)
 url=session['base']+'/#zogg='+token;print('PXL Tape + Zogg: '+url,flush=True);print('Keep this window open. Originals stay on your drives. Ctrl+C stops the bridge.',flush=True)
 if not args.no_open:webbrowser.open(url)
 try:server.serve_forever()
 except KeyboardInterrupt:pass
 finally:
  server.server_close()
  try:
   if json.loads(args.session_file.read_text()).get('token')==token:args.session_file.unlink()
  except (OSError,ValueError):pass
if __name__=='__main__':main()
