#include "Presets.h"
#include "PresetTable.h"

namespace bl
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
