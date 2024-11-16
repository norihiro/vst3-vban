//------------------------------------------------------------------------
// Copyright(c) 2024 Nagater Networks.
//------------------------------------------------------------------------

#include "vban_processor.h"
#include "vban_cids.h"
#include "paramids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

using namespace Steinberg;

namespace NagaterNet {

CVBANPluginProcessor::CVBANPluginProcessor()
{
	//--- set the wanted controller for our processor
	setControllerClass(kCVBANPluginControllerUID);
}

CVBANPluginProcessor::~CVBANPluginProcessor()
{
	if (cont)
		thread_stop();
}

tresult PLUGIN_API CVBANPluginProcessor::initialize(FUnknown *context)
{
	// Here the Plug-in will be instantiated

	//---always initialize the parent-------
	tresult result = AudioEffect::initialize(context);
	// if everything Ok, continue
	if (result != kResultOk) {
		return result;
	}

	//--- create Audio IO ------
	addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	return kResultOk;
}

tresult PLUGIN_API CVBANPluginProcessor::terminate()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return AudioEffect::terminate();
}

tresult PLUGIN_API CVBANPluginProcessor::setActive(TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive(state);
}

static uint32_t param_to_u32(double value, uint32_t max)
{
	return std::clamp((uint32_t)(value * max + 0.5), 0u, max);
}

tresult PLUGIN_API CVBANPluginProcessor::process(Vst::ProcessData &data)
{
	if (auto *paramChanges = data.inputParameterChanges) {
		int32_t n = paramChanges->getParameterCount();
		for (int i = 0; i < n; i++) {
			auto *paramQueue = paramChanges->getParameterData(i);
			if (!paramQueue)
				continue;

			auto numPoints = paramQueue->getPointCount();
			int offset;
			double value = 0.0;
			paramQueue->getPoint(0, offset, value);

			std::unique_lock lk(props_mutex);

			switch (paramQueue->getParameterId()) {
			case paramid_ipv4_0:
				dest_addr = (param_to_u32(value, 255) << 24) | (dest_addr & 0x00FFFFFF);
				break;
			case paramid_ipv4_1:
				dest_addr = (param_to_u32(value, 255) << 16) | (dest_addr & 0xFF00FFFF);
				break;
			case paramid_ipv4_2:
				dest_addr = (param_to_u32(value, 255) << 8) | (dest_addr & 0xFFFF00FF);
				break;
			case paramid_ipv4_3:
				dest_addr = param_to_u32(value, 255) | (dest_addr & 0xFFFFFF00);
				break;
			case paramid_port:
				dest_port = param_to_u32(value, 65535);
				break;
			}
		}
	}

	if (data.numInputs == 0 || data.numOutputs == 0)
		return kResultOk;

	int32_t numChannels = data.inputs[0].numChannels;
	uint32_t sampleFramesSize = Steinberg::Vst::getSampleFramesSizeInBytes(processSetup, data.numSamples);
	void **in = getChannelBuffersPointer(processSetup, data.inputs[0]);
	void **out = getChannelBuffersPointer(processSetup, data.outputs[0]);

	if (data.inputs[0].silenceFlags == Steinberg::Vst::getChannelMask(data.inputs[0].numChannels)) {
		/* all channels are silent */
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		for (int32_t i = 0; i < numChannels; i++)
			memset(out[i], 0, sampleFramesSize);
	} else {
		data.outputs[0].silenceFlags = 0;

		for (int32_t i = 0; i < numChannels; i++) {
			if (in[i] != out[i])
				memcpy(out[i], in[i], sampleFramesSize);
		}
	}

	packets.add_float(out, numChannels, data.numSamples);

	return kResultOk;
}

tresult PLUGIN_API CVBANPluginProcessor::setupProcessing(Vst::ProcessSetup &newSetup)
{
	//--- called before any processing ----

	if (cont)
		thread_stop();

	thread_start();

	return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API CVBANPluginProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

tresult PLUGIN_API CVBANPluginProcessor::setState(IBStream *state)
{
	/* Called to load the configuration from `state` */
	IBStreamer streamer(state, kLittleEndian);

	uint32_t version = 0;
	streamer.readInt32u(version);
	uint32_t version_major = version >> 24;
	uint32_t version_minor = (version >> 16) & 0xFF;
	uint32_t version_patch = version & 0xFFFF;
	if (version_major > 0x01)
		return kResultFalse;

	std::unique_lock lk(props_mutex);
	streamer.readInt32u(dest_addr);
	streamer.readInt16u(dest_port);

	return kResultOk;
}

tresult PLUGIN_API CVBANPluginProcessor::getState(IBStream *state)
{
	/* Called to save the configuration into `state` */
	IBStreamer streamer(state, kLittleEndian);

	uint32_t version = 0x01'00'0000;
	streamer.writeInt32u(version);

	std::unique_lock lk(props_mutex);
	streamer.writeInt32u(dest_addr);
	streamer.writeInt16u(dest_port);

	return kResultOk;
}

} // namespace NagaterNet
