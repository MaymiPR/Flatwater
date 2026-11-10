#include "Presets.h"
#include "PresetTable.h"

namespace bf
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
