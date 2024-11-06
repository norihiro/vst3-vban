//------------------------------------------------------------------------
// Copyright(c) 2024 Nagater Networks.
//------------------------------------------------------------------------

#include "vban_processor.h"
#include "vban_controller.h"
#include "vban_cids.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "VBAN"

using namespace Steinberg::Vst;
using namespace NagaterNet;

//------------------------------------------------------------------------
//  VST Plug-in Entry
//------------------------------------------------------------------------
// Windows: do not forget to include a .def file in your project to export
// GetPluginFactory function!
//------------------------------------------------------------------------

BEGIN_FACTORY_DEF("Nagater Networks", "https://www.nagater.net/", "mailto:norihiro@nagater.net")

//---First Plug-in included in this factory-------
// its kVstAudioEffectClass component
DEF_CLASS2(INLINE_UID_FROM_FUID(kCVBANPluginProcessorUID),
	   PClassInfo::kManyInstances, // cardinality
	   kVstAudioEffectClass,       // the component category (do not changed this)
	   stringPluginName,           // here the Plug-in name (to be changed)
	   Vst::kDistributable,     // means that component and controller could be distributed on different computers
	   CVBANPluginVST3Category, // Subcategory for this Plug-in (to be changed)
	   FULL_VERSION_STR,        // Plug-in version (to be changed)
	   kVstVersionString,       // the VST 3 SDK version (do not changed this, use always this define)
	   CVBANPluginProcessor::createInstance) // function pointer called when this component should be instantiated

// its kVstComponentControllerClass component
DEF_CLASS2(INLINE_UID_FROM_FUID(kCVBANPluginControllerUID),
	   PClassInfo::kManyInstances,            // cardinality
	   kVstComponentControllerClass,          // the Controller category (do not changed this)
	   stringPluginName "Controller",         // controller name (could be the same than component name)
	   0,                                     // not used here
	   "",                                    // not used here
	   FULL_VERSION_STR,                      // Plug-in version (to be changed)
	   kVstVersionString,                     // the VST 3 SDK version (do not changed this, use always this define)
	   CVBANPluginController::createInstance) // function pointer called when this component should be instantiated

//----for others Plug-ins contained in this factory, put like for the first Plug-in different DEF_CLASS2---

END_FACTORY
