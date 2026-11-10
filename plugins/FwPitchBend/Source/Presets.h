#pragma once
// Flatwater Pitch Bend's factory presets: the generated table (PresetTable.h) behind one call, for the toolbar and the
// host's program list; flatwater::applyPreset (shared/Presets.h) loads one.
#include "shared/Presets.h"

namespace pb
{
const std::vector<flatwater::FactoryPreset>& factoryPresets();
}
