from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
entries=json.loads((root/'dist/factory-sounds.json').read_text())
assert len(entries)==16 and all(0<e['duration']<2 for e in entries)
header=root.parent/'PXLTape/Source/FactoryManifest.h'
header.write_text('#pragma once\n// Generated from PXLTape-Web/dist/factory-sounds.json; metadata only, no source audio.\n#include <juce_core/juce_core.h>\ninline juce::var factorySounds(){return juce::JSON::parse(R"PXLF('+json.dumps(entries,separators=(',',':'))+')PXLF");}\n')
print('Generated16 local Zogg factory entries')
