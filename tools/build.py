from pathlib import Path
import os,subprocess,re,hashlib,json,shutil
root=Path(__file__).resolve().parents[1]
sdk=Path(os.environ.get('WASI_SDK_PATH',str(root.parent/'PXLDub/build/toolchains/wasi-sdk-34.0-arm64-macos')))
source=root/'engine/WebBridge.cpp'
exports=list(dict.fromkeys(re.findall(r'\b(tape_\w+)\(',source.read_text())))
cmd=[str(sdk/'bin/clang++'),'-std=c++17','-O2','-ffp-contract=off','-fno-exceptions','-fno-rtti','-nostartfiles','-Wl,--no-entry','-Wl,--export-memory','-Wl,--export=__wasm_call_ctors','-Wl,--initial-memory=33554432','-Wl,--max-memory=402653184','-Wl,-z,stack-size=1048576']+[f'-Wl,--export={x}' for x in exports]+[str(source),'-o',str(root/'dist/engine.wasm')]
subprocess.run(cmd,check=True)
(root/'dist/engine-build.json').write_text(json.dumps({'engine':'PXL Tape 0.2.0','source_sha256':hashlib.sha256((root/'engine/TapeEngine.h').read_bytes()).hexdigest(),'bridge_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'wasm_sha256':hashlib.sha256((root/'dist/engine.wasm').read_bytes()).hexdigest()},indent=2)+'\n')
print('Built shared C++ tape engine:',(root/'dist/engine.wasm').stat().st_size,'bytes')

compiler=shutil.which('clang++') or shutil.which('g++')
if compiler:
 subprocess.run([compiler,'-std=c++17','-O2','-ffp-contract=off',str(root/'tests/NativeOracle.cpp'),'-o',str(root/'artifacts/native-oracle')],check=True)
 with (root/'tests/native-oracle.f32').open('wb') as output:subprocess.run([str(root/'artifacts/native-oracle')],stdout=output,check=True)
