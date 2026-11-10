#include "Presets.h"
#include "PresetTable.h"

namespace vc
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
