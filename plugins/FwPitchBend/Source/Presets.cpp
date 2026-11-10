#include "Presets.h"
#include "PresetTable.h"

namespace pb
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
