//------------------------------------------------------------------------
// Copyright(c) 2024 Nagater Networks.
//------------------------------------------------------------------------

#include "vban_processor.h"
#include "vban_cids.h"

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

tresult PLUGIN_API CVBANPluginProcessor::process(Vst::ProcessData &data)
{
	if (auto *paramChanges = data.inputParameterChanges) {
		int32_t n = paramChanges->getParameterCount();
		for (int i = 0; i < n; i++) {
			auto *paramQueue = paramChanges->getParameterData(i);
			if (!paramQueue)
				continue;

			auto numPoints = paramQueue->getPointCount();

			switch (paramQueue->getParameterId()) {
				/* TODO: Process parameter changes */
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

	/* TODO: Send the audio data through VBAN. */

	return kResultOk;
}

tresult PLUGIN_API CVBANPluginProcessor::setupProcessing(Vst::ProcessSetup &newSetup)
{
	//--- called before any processing ----
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
	// called when we load a preset, the model has to be reloaded
	IBStreamer streamer(state, kLittleEndian);

	return kResultOk;
}

tresult PLUGIN_API CVBANPluginProcessor::getState(IBStream *state)
{
	// here we need to save the model
	IBStreamer streamer(state, kLittleEndian);

	return kResultOk;
}

} // namespace NagaterNet
