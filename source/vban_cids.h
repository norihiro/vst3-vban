//------------------------------------------------------------------------
// Copyright(c) 2024 Nagater Networks.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace NagaterNet {
//------------------------------------------------------------------------
static const Steinberg::FUID kCVBANPluginProcessorUID (0x14B7F584, 0x641B57E0, 0x85DC5391, 0x4E5369FA);
static const Steinberg::FUID kCVBANPluginControllerUID (0xE8116636, 0x3BB351D3, 0xBA5F6435, 0x1C3B62A8);

#define CVBANPluginVST3Category "Fx"

//------------------------------------------------------------------------
} // namespace NagaterNet
