#pragma once
#include "SurgeXT.hpp"
#include "XTModule.hpp"
#include "dsp/Oscillator.h"
#include "rack.hpp"
#include <cstring>
#include <sst/filters/HalfRateFilter.h>

namespace sst::surgext_rack::vco
{
template <int oscType> struct VCO;

template <int oscType> struct VCOConfig
{
    static constexpr bool supportsUnison() { return false; }
    static constexpr bool requiresWavetables() { return false; }

    struct KnobDef
    {
        enum Type
        {
            PARAM,
            INPUT,
            BLANK
        } type{PARAM};
        std::string name;
        int colspan{1};
        int id{-1};

        KnobDef(int kid, const std::string &nm) : type(PARAM), id(kid), name(nm) {}
        KnobDef(Type t, int kid, const std::string &nm) : type(t), id(kid), name(nm) {}
        KnobDef(Type t, int kid, const std::string &nm, int colspan) : type(t), id(kid), name(nm), colspan(colspan) {}
        KnobDef(Type t) : type(t) {}
    };
    typedef std::vector<KnobDef> knobs_t;
    static knobs_t getKnobs() { return {}; }

    typedef std::vector<std::pair<int,int>> lightOnTo_t;
    static lightOnTo_t getLightsOnKnobsTo() { return {}; }

    static int rightMenuParamId() { return -1; }

    static void oscillatorSpecificSetup(VCO<oscType> *) {}
    static void processLightParameters(VCO<oscType> *) {}

    /*
     * Wavetable Updates from the UI to the Processing Thread.
     * This queue can be pretty small. You can't change more quickly than we
     * load em really
     */
    static constexpr int wavetableQueueSize() { return requiresWavetables() ? 32 : 1; }
};

template <int oscType> struct VCO : public modules::XTModule
{
    static constexpr int n_mod_inputs{4};
    static constexpr int n_arbitrary_switches{4};

    enum ParamIds
    {
        PITCH_0,

        OSC_CTRL_PARAM_0,

        OSC_MOD_PARAM_0 = OSC_CTRL_PARAM_0 + n_osc_params,

        OCTAVE_SHIFT = OSC_MOD_PARAM_0 + (n_osc_params + 1) * n_mod_inputs,

        ARBITRARY_SWITCH_0,

        NUM_PARAMS = ARBITRARY_SWITCH_0 + n_arbitrary_switches
    };
    enum InputIds
    {
        PITCH_CV,

        RETRIGGER,

        OSC_MOD_INPUT,

        AUDIO_INPUT = OSC_MOD_INPUT + n_mod_inputs,
        NUM_INPUTS
    };
    enum OutputIds
    {
        OUTPUT_L,
        OUTPUT_R,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
    };

    static constexpr const char *name = osc_type_names[oscType];
    std::array<std::string, n_osc_params> paramNames;

    VCO() : XTModule()
    {
        surge_osc.fill(nullptr);
        lastUnison.fill(-1);

        setupSurgeCommon(NUM_PARAMS, VCOConfig<oscType>::requiresWavetables());

        oscstorage = &(storage->getPatch().scene[0].osc[0]);
        oscstorage_display = &(storage->getPatch().scene[0].osc[1]);
        oscstorage->type.val.i = oscType;
        oscstorage_display->type.val.i = oscType;
        setupStorageRanges(&(oscstorage->type), &(oscstorage->retrigger));

        if constexpr (VCOConfig<oscType>::requiresWavetables())
        {
            oscstorage->wt.queue_id = 0;
            oscstorage_display->wt.queue_id = 0;
            storage->perform_queued_wtloads();
        }

        auto config_osc = spawn_osc(oscType, storage.get(), oscstorage,
                                    storage->getPatch().scenedata[0], oscdisplaybuffer);
        config_osc->init(72.0);
        config_osc->init_ctrltypes();
        config_osc->init_default_values();
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < n_osc_params; ++i)
        {
            paramNames[i] = oscstorage->p[i].get_name();
        }

        configParam<modules::MidiNoteParamQuantity<0>>(PITCH_0, 1, 127, 60, "Pitch");
        auto os = configParam(OCTAVE_SHIFT, -3, 3, 0, "Octave Shift");
        os->snapEnabled = true;

        for (int i = 0; i < n_osc_params + 1; ++i)
        {
            configParam<modules::SurgeParameterParamQuantity>(
                OSC_CTRL_PARAM_0 + i, 0, 1, oscstorage->p[i].get_value_f01());
        }

        for (int i = OSC_MOD_PARAM_0; i < OSC_MOD_PARAM_0 + (n_osc_params + 1) * n_mod_inputs; ++i)
        {
            configParam(i, -1, 1, 0);
        }

        for (int i=0; i<n_arbitrary_switches; ++i)
        {
            configParam(ARBITRARY_SWITCH_0 + i, 0, 1, 0);
        }

        // pc.update(this);

        config_osc->~Oscillator();

        auto display_config_osc = spawn_osc(oscType, storage.get(), oscstorage_display,
                                    storage->getPatch().scenedata[0], oscdisplaybuffer);
        display_config_osc->init(72.0);
        display_config_osc->init_ctrltypes();
        display_config_osc->init_default_values();
        display_config_osc->~Oscillator();

        for (int i = 0; i < MAX_POLY; ++i)
        {
            halfbandOUT.emplace_back(6, true);
            halfbandOUT[i].reset();
        }

        VCOConfig<oscType>::oscillatorSpecificSetup(this);
    }

    static int modulatorIndexFor(int baseParam, int modulator)
    {
        int offset = baseParam - PITCH_0;
        return OSC_MOD_PARAM_0 + offset * n_mod_inputs + modulator;
    }

    ~VCO()
    {
        for (int i = 0; i < MAX_POLY; ++i)
        {
            if (surge_osc[i])
                surge_osc[i]->~Oscillator();
            surge_osc[i] = nullptr;
        }
    }

    std::string getName() override
    {
        return std::string("VCO<") + osc_type_names[oscType] + ">";
    }


    Parameter *surgeParameterForParamId(int paramId) override {
        if (paramId < OSC_CTRL_PARAM_0 || paramId >= OSC_CTRL_PARAM_0 + n_osc_params)
            return nullptr;

        return &oscstorage->p[paramId-OSC_CTRL_PARAM_0];
    }


    int processPosition = BLOCK_SIZE + 1;

    void moduleSpecificSampleRateChange() override { forceRespawnDueToSampleRate = true; }

    struct WavetableMessage
    {
        int index{-1};
        char filename[1024];
    };
    rack::dsp::RingBuffer<WavetableMessage, VCOConfig<oscType>::wavetableQueueSize()> wavetableQueue;
    std::atomic<int> wavetableIndex{-1};
    std::string getWavetableName()
    {
        int idx = wavetableIndex;
        if (idx >= 0)
            return storage->wt_list[idx].name;
        else
            return oscstorage->wavetable_display_name;
    }

    std::array<int, MAX_POLY> lastUnison{-1};
    int lastNChan{-1};
    bool forceRespawnDueToSampleRate = false;
    void process(const typename rack::Module::ProcessArgs &args) override
    {
        int nChan = std::max(1, inputs[PITCH_CV].getChannels());
        outputs[OUTPUT_L].setChannels(nChan);
        outputs[OUTPUT_R].setChannels(nChan);
        std::array<bool, MAX_POLY> respawned;
        respawned.fill(false);

        if (nChan != lastNChan || forceRespawnDueToSampleRate)
        {
            lastNChan = nChan;
            // Set up unmodulated values
            for (int i = 0; i < n_osc_params; ++i)
            {
                oscstorage->p[i].set_value_f01(params[OSC_CTRL_PARAM_0 + i].getValue());
                oscstorage_display->p[i].set_value_f01(params[OSC_CTRL_PARAM_0 + i].getValue());
            }

            copyScenedataSubset(0, storage_id_start, storage_id_end);

            for (int c = 0; c < nChan; ++c)
            {
                float pitch0 = params[PITCH_0].getValue() + (params[OCTAVE_SHIFT].getValue() + inputs[PITCH_CV].getVoltage(c)) * 12;
                if (!surge_osc[c])
                {
                    surge_osc[c] = spawn_osc(oscType, storage.get(), oscstorage,
                                             storage->getPatch().scenedata[0], oscbuffer[c]);

                    surge_osc[c]->init(pitch0);
                    surge_osc[c]->init_ctrltypes();
                }
                else
                {
                    surge_osc[c]->init(pitch0);
                }
                respawned[c] = true;
            }
            forceRespawnDueToSampleRate = false;
            processPosition = BLOCK_SIZE + 1;
        }

        if (processPosition >= BLOCK_SIZE)
        {
            // As @Vortico says "think like a hardware engineer; only snap
            // values when you need them".
            processPosition = 0;

            VCOConfig<oscType>::processLightParameters(this);

            float modMatrix[n_osc_params + 1][n_mod_inputs];

            for (int i = 0; i < n_osc_params + 1; ++i)
            {
                for (int m = 0; m < n_mod_inputs; ++m)
                {
                    modMatrix[i][m] = 0.f;
                }
            }
            const auto &knobConfig = VCOConfig<oscType>::getKnobs();
            for (const auto k : knobConfig)
            {
                int id = k.id - PITCH_0;
                for (int m = 0; m < n_mod_inputs; ++m)
                {
                    int modid = modulatorIndexFor(k.id, m);
                    modMatrix[id][m] = params[modid].getValue();
                }
            }

            for (int m = 0; m < n_mod_inputs; ++m)
                modMatrix[0][m] *= 12; // volts per octave

            float modValue[rack::PORT_MAX_CHANNELS][n_osc_params + 1];

            for (int c = 0; c < nChan; ++c)
            {
                float pitch0 = params[PITCH_0].getValue() + (params[OCTAVE_SHIFT].getValue() + inputs[PITCH_CV].getVoltage(c)) * 12.0;

                modValue[c][0] = pitch0;
                for (int i = 0; i < n_osc_params; ++i)
                {
                    modValue[c][i + 1] = params[OSC_CTRL_PARAM_0 + i].getValue();
                }
            }
            for (int m = 0; m < n_mod_inputs; ++m)
            {
                if (inputs[OSC_MOD_INPUT + m].isConnected())
                {
                    auto nModChan = inputs[OSC_MOD_INPUT + m].getChannels();

                    for (int i = 0; i < n_osc_params + 1; ++i)
                    {
                        for (int c = 0; c < nChan; ++c)
                        {
                            auto q = c > nModChan ? 0 : c;
                            modValue[c][i] += modMatrix[i][m] *
                                              inputs[OSC_MOD_INPUT + m].getVoltage(q) *
                                              RACK_TO_SURGE_CV_MUL;
                        }
                    }
                }
            }

            for (int i = 0; i < n_osc_params; ++i)
            {
                // This is the non-modulated version
                // oscstorage_display->p[i].set_value_f01(params[OSC_CTRL_PARAM_0 + i].getValue());
                oscstorage_display->p[i].set_value_f01(modValue[0][i + 1]);
            }

            for (int c = 0; c < nChan; ++c)
            {
                bool needsReInit{false};

                if (inputs[RETRIGGER].isConnected() &&
                    reTrigger[c].process(inputs[RETRIGGER].getVoltage(c)))
                {
                    needsReInit = true;
                }

                if (outputs[OUTPUT_L].isConnected() || outputs[OUTPUT_R].isConnected())
                {
                    for (int i = 0; i < n_osc_params; ++i)
                    {
                        oscstorage->p[i].set_value_f01(modValue[c][i + 1]);
                    }
                    if constexpr (VCOConfig<oscType>::supportsUnison())
                    {
                        if (oscstorage->p[n_osc_params - 1].val.i != lastUnison[c])
                        {
                            lastUnison[c] = oscstorage->p[n_osc_params - 1].val.i;
                            needsReInit = true;
                        }
                    }

                    copyScenedataSubset(0, storage_id_start, storage_id_end);
                    if (needsReInit)
                    {
                        surge_osc[c]->init(modValue[c][0]);
                    }
                    surge_osc[c]->process_block(modValue[c][0], 0, true);
                    copy_block(surge_osc[c]->output, osc_downsample[0][c], BLOCK_SIZE_OS_QUAD);
                    copy_block(surge_osc[c]->outputR, osc_downsample[1][c], BLOCK_SIZE_OS_QUAD);
                    halfbandOUT[c].process_block_D2(osc_downsample[0][c], osc_downsample[1][c],
                                                    BLOCK_SIZE_OS);
                }
            }
            // pc.update(this);
        }

        for (int c = 0; c < nChan; ++c)
        {
            if (outputs[OUTPUT_L].isConnected() && !outputs[OUTPUT_R].isConnected())
            {
                // Special mono mode
                float output = (osc_downsample[0][c][processPosition] +
                                osc_downsample[1][c][processPosition]) *
                               0.5 * SURGE_TO_RACK_OSC_MUL;
                outputs[OUTPUT_L].setVoltage(output, c);
            }
            else
            {
                if (outputs[OUTPUT_L].isConnected())
                    outputs[OUTPUT_L].setVoltage(
                        osc_downsample[0][c][processPosition] * SURGE_TO_RACK_OSC_MUL, c);

                if (outputs[OUTPUT_R].isConnected())
                    outputs[OUTPUT_R].setVoltage(
                        osc_downsample[1][c][processPosition] * SURGE_TO_RACK_OSC_MUL, c);
            }
        }

        processPosition++;
    }

    // With surge-xt the oscillator memory is owned by the synth after spawn
    std::array<Oscillator *, MAX_POLY> surge_osc;
    unsigned char oscbuffer alignas(16)[MAX_POLY][oscillator_buffer_size];
    unsigned char oscdisplaybuffer alignas(16)[oscillator_buffer_size];

    OscillatorStorage *oscstorage, *oscstorage_display;
    float osc_downsample alignas(16)[2][MAX_POLY][BLOCK_SIZE_OS];
    std::vector<sst::filters::HalfRate::HalfRateFilter> halfbandOUT;

    rack::dsp::SchmittTrigger reTrigger[MAX_POLY];
};
}