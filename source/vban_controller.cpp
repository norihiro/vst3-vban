//------------------------------------------------------------------------
// Copyright(c) 2024 Nagater Networks.
//------------------------------------------------------------------------

#include "vban_controller.h"
#include "vban_cids.h"
#include "paramids.h"

#include "base/source/fstreamer.h"

using namespace Steinberg;

namespace NagaterNet {

//------------------------------------------------------------------------
// CVBANPluginController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::initialize(FUnknown *context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize(context);
	if (result != kResultOk) {
		return result;
	}

	// Here you could register some parameters

	using Steinberg::Vst::Parameter;
	using Steinberg::Vst::RangeParameter;

	Parameter *param;
	param = new RangeParameter(STR16("IPv4 Address 1"), paramid_ipv4_0, nullptr, 0.0, 255.0, 0.0, 255);
	parameters.addParameter(param);

	param = new RangeParameter(STR16("IPv4 Address 2"), paramid_ipv4_1, nullptr, 0.0, 255.0, 0.0, 255);
	parameters.addParameter(param);

	param = new RangeParameter(STR16("IPv4 Address 3"), paramid_ipv4_2, nullptr, 0.0, 255.0, 0.0, 255);
	parameters.addParameter(param);

	param = new RangeParameter(STR16("IPv4 Address 4"), paramid_ipv4_3, nullptr, 0.0, 255.0, 0.0, 255);
	parameters.addParameter(param);

	param = new RangeParameter(STR16("Port"), paramid_port, nullptr, 0.0, 65535.0, 6980.0, 65535);
	parameters.addParameter(param);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::terminate()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::setComponentState(IBStream *state)
{
	/* Called to load the configuration of the processor from `state` */
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	uint32_t dest_addr = 0;
	uint16_t dest_port = 0;

	uint32_t version = 0;
	streamer.readInt32u(version);
	uint32_t version_major = version >> 24;
	uint32_t version_minor = (version >> 16) & 0xFF;
	uint32_t version_patch = version & 0xFFFF;
	if (version_major > 0x01)
		return kResultFalse;

	streamer.readInt32u(dest_addr);
	streamer.readInt16u(dest_port);

	setParamNormalized(paramid_ipv4_0, ((dest_addr >> 24) & 0xFF) / 255.0);
	setParamNormalized(paramid_ipv4_1, ((dest_addr >> 16) & 0xFF) / 255.0);
	setParamNormalized(paramid_ipv4_2, ((dest_addr >> 8) & 0xFF) / 255.0);
	setParamNormalized(paramid_ipv4_3, ((dest_addr >> 0) & 0xFF) / 255.0);
	setParamNormalized(paramid_port, dest_port / 65535.0);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::setState(IBStream *state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::getState(IBStream *state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView *PLUGIN_API CVBANPluginController::createView(FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual(name, Vst::ViewType::kEditor)) {
		// create your editor here and return a IPlugView ptr of it
		return nullptr;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized(tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized,
								Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API CVBANPluginController::getParamValueByString(Vst::ParamID tag, Vst::TChar *string,
								Vst::ParamValue &valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace NagaterNet
