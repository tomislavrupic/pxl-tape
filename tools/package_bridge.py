"""Bundle the loopback helper and public UI; exclude sample caches and native ZIPs."""
from pathlib import Path
import shutil,zipfile
root=Path(__file__).resolve().parents[1]
stage=root/'artifacts/zogg-bridge/PXL-Tape-Zogg-Bridge'
(stage/'tools').mkdir(parents=True,exist_ok=True);(stage/'dist').mkdir(exist_ok=True)
shutil.copy2(root/'Start Zogg.command',stage/'Start Zogg.command');shutil.copy2(root/'tools/zogg_bridge.py',stage/'tools/zogg_bridge.py')
for file in (root/'dist').iterdir():
 if file.is_file() and file.suffix in {'.html','.css','.js','.svg','.json','.wasm'}:shutil.copy2(file,stage/'dist'/file.name)
(stage/'README.md').write_text('PXL Tape / Zogg Bridge0.5\n\nUnzip, then open Start Zogg.command. Requires Python3.11+ and your existing Zogg MCP configuration in ~/.codex/config.toml, or ZOGG_MCP_COMMAND/ZOGG_MCP_ARGS environment variables. Keep the Terminal window open. Use its connection link after each restart. The launcher opens a connected local workstation; native PXL Tape0.5 connects through its SAMPLES button.\n\nSearch, select, set start/length, Preview, then Load Sampler or Load Tape. Sampler30s; tape up to300s per transfer at the playhead of the chosen track with Undo. Original sources unchanged. No samples or catalogue are bundled or uploaded. Localhost and GitHub Pages have separate browser album storage; export your albums and save projects when switching.\n\nCtrl+C stops the bridge. If port4186 is occupied by your running bridge, use its existing Terminal window and connection link. No plugin installation or background service is performed.\n')
out=root/'dist/downloads/PXL-Tape-Zogg-Bridge.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as archive:
 for file in stage.rglob('*'):
  if file.is_file():archive.write(file,file.relative_to(stage.parent))
 assert archive.testzip() is None
print(out, out.stat().st_size,'bytes')
