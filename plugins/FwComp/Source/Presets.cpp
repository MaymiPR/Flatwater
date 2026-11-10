#include "Presets.h"
#include "PresetTable.h"

namespace co
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
