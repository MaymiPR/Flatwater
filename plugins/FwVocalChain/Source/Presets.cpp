#include "Presets.h"
#include "PresetTable.h"

namespace ch
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
