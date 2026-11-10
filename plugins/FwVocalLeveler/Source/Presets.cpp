#include "Presets.h"
#include "PresetTable.h"

namespace vl
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
