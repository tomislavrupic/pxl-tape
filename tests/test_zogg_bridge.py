import importlib.util,json,threading,unittest,urllib.request,urllib.error,tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('bridge',Path(__file__).parents[1]/'tools/zogg_bridge.py');bridge=importlib.util.module_from_spec(spec);spec.loader.exec_module(bridge)
class TestBridge(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.calls=[]
  def call(name,args):
   self.calls.append((name,args))
   if name=='search_sounds':return {'results':[],'total':0}
   if name=='get_sound':return {'asset':{'id':7,'filename':'hit.wav','duration_seconds':1},'source':{'available_and_unchanged':True,'current_fingerprint':'a'*64,'path':'private'}}
   if name=='preview_sound':
    f=Path(self.tmp.name)/'preview.wav';f.write_bytes(b'RIFFtest');return {'preview':{'output_path':str(f)}}
   return {'libraries':[]}
  self.server=bridge.make_server(0,'secret',Path(__file__).parents[1]/'dist',Path(self.tmp.name),call);self.thread=threading.Thread(target=self.server.serve_forever,daemon=True);self.thread.start();self.url='http://127.0.0.1:'+str(self.server.server_port)
 def tearDown(self):self.server.shutdown();self.server.server_close();self.tmp.cleanup()
 def request(self,path,data={},headers=None):
  return urllib.request.urlopen(urllib.request.Request(self.url+path,data=json.dumps(data).encode() if data is not None else None,headers={'Authorization':'Bearer secret','Content-Type':'application/json',**(headers or {})}),timeout=3)
 def test_boundaries(self):
  for headers in [{'Authorization':''},{'Origin':'https://evil.test'},{'Host':'evil.test'}]:
   with self.assertRaises(urllib.error.HTTPError) as e:self.request('/api/status',headers=headers)
   self.assertIn(e.exception.code,[401,403])
  with self.assertRaises(urllib.error.HTTPError):self.request('/api/search',{'query':'x','limit':500})
  with self.assertRaises(urllib.error.HTTPError):self.request('/api/prepare',{'asset_id':7,'duration':301})
 def test_search_and_handles(self):
  self.assertEqual(json.load(self.request('/api/search',{'query':'kick','offset':25}))['total'],0)
  result=json.load(self.request('/api/preview',{'asset_id':7,'start':0,'duration':1}));self.assertNotIn('private',json.dumps(result));self.assertEqual(self.request(result['audio'],None).read(),b'RIFFtest')
  with self.assertRaises(urllib.error.HTTPError):self.request('/audio/../../etc/passwd',None)
 def test_static_factory_catalogue(self):
  with urllib.request.urlopen(self.url+'/factory-sounds.json?v=0.6.0') as response:self.assertEqual(len(json.load(response)),8)
 def test_factory_identity_guard(self):
  with self.assertRaises(urllib.error.HTTPError) as e:self.request('/api/prepare',{'asset_id':7,'duration':1,'expected_fingerprint':'b'*64})
  self.assertEqual(e.exception.code,400);self.assertNotIn('prepare_sound',[name for name,args in self.calls])
 def test_local_window_cookie(self):
  with urllib.request.urlopen(self.url+'/') as response:
   self.assertIsNone(response.headers.get('Set-Cookie'));self.assertIn(b'data-zogg-local="true"',response.read())
  with self.request('/api/pair') as response:
   cookie=response.headers['Set-Cookie'];self.assertIn('HttpOnly',cookie);self.assertIn('SameSite=Strict',cookie)
  with self.request('/api/status',headers={'Authorization':'','Cookie':cookie.split(';')[0]}) as response:self.assertEqual(response.status,200)
  with self.assertRaises(urllib.error.HTTPError):self.request('/api/pair',headers={'Authorization':''})
 def test_error_is_reported(self):
  self.server.call=lambda n,a:(_ for _ in ()).throw(RuntimeError('SOURCE_CHANGED'))
  with self.assertRaises(urllib.error.HTTPError) as e:self.request('/api/status')
  self.assertIn('SOURCE_CHANGED',e.exception.read().decode())
if __name__=='__main__':unittest.main()
