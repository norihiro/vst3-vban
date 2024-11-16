//------------------------------------------------------------------------
// Copyright(c) 2024 Nagater Networks.
//------------------------------------------------------------------------

#pragma once

#include <pthread.h>
#include "audio_buffer.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

namespace NagaterNet {

class CVBANPluginProcessor : public Steinberg::Vst::AudioEffect
{
public:
	CVBANPluginProcessor();
	~CVBANPluginProcessor() SMTG_OVERRIDE;

	// Create function
	static Steinberg::FUnknown *createInstance(void * /*context*/)
	{
		return (Steinberg::Vst::IAudioProcessor *)new CVBANPluginProcessor;
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *context) SMTG_OVERRIDE;

	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	/** Switch the Plug-in on/off */
	Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup &newSetup) SMTG_OVERRIDE;

	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData &data) SMTG_OVERRIDE;

	/** For persistence */
	Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *state) SMTG_OVERRIDE;

protected:
	uint32_t dest_addr;
	uint16_t dest_port;
	std::mutex props_mutex;

	struct audio_buffer packets;
	pthread_t thread;
	volatile bool cont = false;
	bool has_error;

private:
	void thread_start();
	void thread_stop();
	void thread_loop();
	static void *thread_entry(void *data);

private:
	bool thread_loop_init(struct loop_context &);
	bool thread_loop_obtain_from_queue(struct loop_context &);
	uint32_t thread_loop_send(struct loop_context &);
};

} // namespace NagaterNet
