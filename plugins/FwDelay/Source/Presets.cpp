#include "Presets.h"
#include "PresetTable.h"

namespace de
{
const std::vector<flatwater::FactoryPreset>& factoryPresets() { return presetTable(); }
}
