#include "Presets.h"
#include "PresetTable.h"

namespace bx
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
