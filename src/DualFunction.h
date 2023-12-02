/*
 * SurgeXT for VCV Rack - a Surge Synth Team product
 *
 * A set of modules expressing Surge XT into the VCV Rack Module Ecosystem
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * Surge XT for VCV Rack is released under the GNU General Public License
 * 3.0 or later (GPL-3.0-or-later). A copy of the license is in this
 * repository in the file "LICENSE" or at:
 *
 * or at https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for Surge XT for VCV Rack is available at
 * https://github.com/surge-synthesizer/surge-rack/
 */

#ifndef SURGE_XT_RACK_SRC_QUADAD_H
#define SURGE_XT_RACK_SRC_QUADAD_H

#include "SurgeXT.h"
#include "dsp/Effect.h"
#include "XTModule.h"
#include "rack.hpp"
#include <cstring>

#include "DebugHelpers.h"
#include "FxPresetAndClipboardManager.h"

#include "LayoutEngine.h"

#include "sst/basic-blocks/modulators/ADAREnvelope.h"

namespace sst::surgext_rack::dualfunction
{
struct DualFunction : modules::XTModule
{
    static constexpr int n_funs{2};
    static constexpr int n_mod_params{n_funs * 4};
    static constexpr int n_mod_inputs{4};

    enum ParamIds
    {
        // Important modulation targets are contiguous and first
        FP_0_0,
        FP_0_1,
        FP_0_2,
        FP_0_3,

        FP_1_0,
        FP_1_1,
        FP_1_2,
        FP_1_3,

        MODE_0,
        MODE_1,

        MOD_PARAM_0,
        NUM_PARAMS = MOD_PARAM_0 + n_mod_params * n_mod_inputs
    };

    enum InputIds
    {
        TRIGGER_0,
        TRIGGER_1,

        MOD_INPUT_0,
        NUM_INPUTS = MOD_INPUT_0 + n_mod_inputs,
    };

    enum OutputIds
    {
        OUTPUT_0,
        OUTPUT_1,
        EOR_0,
        EOR_1,
        EOC_0,
        EOC_1,
        NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
    };

    modules::ModulationAssistant<DualFunction, n_mod_params, FP_0_0, n_mod_inputs, MOD_INPUT_0>
        modAssist;

    DualFunction() : XTModule()
    {
        {
            std::lock_guard<std::mutex> lgxt(xtSurgeCreateMutex);
            setupSurge();
        }
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < n_mod_params * n_mod_inputs; ++i)
        {
            int modi = i % n_mod_inputs;
            int pari = i / n_mod_inputs;
            std::string tgt = "FP";
            auto name = fmt::format("Mod {} to {} {}", modi + 1, tgt, pari + 1);
            configParam(MOD_PARAM_0 + i, -1, 1, 0, name, "%", 0, 100);
        }

        modAssist.initialize(this);
        modAssist.setupMatrix(this);
        modAssist.updateValues(this);

        snapCalculatedNames();
    }

    void setupSurge() { setupSurgeCommon(NUM_PARAMS, false, false); }

    Parameter *surgeDisplayParameterForParamId(int paramId) override
    {
        std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
        return nullptr;
    }

    int polyChannelCount() { return nChan; }
    static int paramModulatedBy(int modIndex)
    {
        int offset = modIndex - FP_0_0;
        if (offset >= n_mod_inputs * (n_mod_params + 1) || offset < 0)
            return -1;
        return offset / n_mod_inputs;
    }

    static int modulatorIndexFor(int baseParam, int modulator)
    {
        int offset = baseParam - FP_0_0;
        return MOD_PARAM_0 + offset * n_mod_inputs + modulator;
    }

    float modulationDisplayValue(int paramId) override
    {
        int idx = paramId - FP_0_0;
        if (idx < 0 || idx >= n_mod_params)
            return 0;

        return modAssist.modvalues[idx][0];
    }

    bool isBipolar(int paramId) override { return false; }

    void moduleSpecificSampleRateChange() override {}
    std::string getName() override { return {"QuadAD"}; }

    int nChan{-1};

    int processCount{BLOCK_SIZE};

    void process(const typename rack::Module::ProcessArgs &args) override
    {
        if (processCount == BLOCK_SIZE)
        {
            processCount = 0;

            modAssist.setupMatrix(this);
            modAssist.updateValues(this);
        }

        processCount++;
    }

    /*
    json_t *makeModuleSpecificJson() override
    {
        auto qv = json_object();
        json_object_set_new(qv, "attackFromZero", json_boolean(attackFromZero));
        return qv;
    }

    void readModuleSpecificJson(json_t *modJ) override
    {
        {
            auto az = json_object_get(modJ, "attackFromZero");
            if (az)
            {
                attackFromZero = json_boolean_value(az);
            }
        }
    }
     */

};
} // namespace sst::surgext_rack::quadad
#endif
