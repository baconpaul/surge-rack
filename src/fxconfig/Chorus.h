//
// Created by Paul Walker on 9/27/22.
//

#include "dsp/effects/ChorusEffect.h"

#ifndef RACK_HACK_CHORUS_H
#define RACK_HACK_CHORUS_H

namespace sst::surgext_rack::fx
{
/*
 * CHORUS:
 * One extra input for a clock
 * Two extra params for power on the lo and hi cut
 */
template <> constexpr bool FXConfig<fxt_chorus4>::usesClock() { return true; }
template <> constexpr int FXConfig<fxt_chorus4>::specificParamCount() { return 2; }

template <> FXConfig<fxt_chorus4>::layout_t FXConfig<fxt_chorus4>::getLayout()
{
    const auto &col = widgets::StandardWidthWithModulationConstants::columnCenters_MM;

    const auto outputRow = FXLayoutHelper::rowStart_MM;
    const auto delayRow = outputRow - FXLayoutHelper::labeledGap_MM;
    const auto bigRow = delayRow - FXLayoutHelper::knobGap16_MM;

    const auto endOfPanel = bigRow - 8;

    typedef FX<fxt_chorus4> fx_t;
    typedef ChorusEffect<4> sfx_t;

    // fixme use the enums
    // clang-format off
    return {
        {LayoutItem::KNOB16, "RATE", sfx_t::ch_rate, FXLayoutHelper::bigCol0, bigRow},
        {LayoutItem::KNOB16, "DEPTH", sfx_t::ch_depth, FXLayoutHelper::bigCol1, bigRow},

        {LayoutItem::PORT, "CLOCK", FX<fxt_delay>::INPUT_CLOCK,
                 col[0], delayRow },
        {LayoutItem::KNOB9, "TIME", sfx_t::ch_time, col[2], delayRow},
        {LayoutItem::KNOB9, "F/BACK", sfx_t::ch_feedback, col[3], delayRow},
        LayoutItem::createGrouplabel("DELAY", col[2], delayRow, 2),

        {LayoutItem::KNOB9, "LO", sfx_t::ch_lowcut, col[0], outputRow},
        {LayoutItem::POWER_LIGHT, "", fx_t::FX_SPECIFIC_PARAM_0, col[0], outputRow, -1},

        {LayoutItem::KNOB9, "HI", sfx_t::ch_highcut, col[1], outputRow},
        {LayoutItem::POWER_LIGHT, "", fx_t::FX_SPECIFIC_PARAM_0+1, col[1], outputRow, +1},
        LayoutItem::createGrouplabel("EQ", col[0], outputRow, 2).withExtra("SHORTLEFT",1).withExtra("SHORTRIGHT",1),

        {LayoutItem::KNOB9, "WIDTH", sfx_t::ch_width, col[2], outputRow},
        {LayoutItem::KNOB9, "MIX", sfx_t::ch_mix, col[3], outputRow},
        LayoutItem::createGrouplabel("OUTPUT", col[2], outputRow, 2),

        LayoutItem::createPresetLCDArea(),
    };

    // clang-format on
}

template <> void FXConfig<fxt_chorus4>::configSpecificParams(FX<fxt_chorus4> *m)
{
    typedef FX<fxt_chorus4> fx_t;
    m->configParam(fx_t::FX_SPECIFIC_PARAM_0, 0, 1, 1, "Enable LowCut");
    m->configParam(fx_t::FX_SPECIFIC_PARAM_0 + 1, 0, 1, 1, "Enable HiCut");
}

template <> void FXConfig<fxt_chorus4>::processSpecificParams(FX<fxt_chorus4> *m)
{
    typedef FX<fxt_chorus4> fx_t;
    FXLayoutHelper::processDeactivate(m, ChorusEffect<4>::ch_lowcut, fx_t::FX_SPECIFIC_PARAM_0);
    FXLayoutHelper::processDeactivate(m, ChorusEffect<4>::ch_highcut,
                                      fx_t::FX_SPECIFIC_PARAM_0 + 1);
}

template <>
void FXConfig<fxt_chorus4>::loadPresetOntoSpecificParams(
    FX<fxt_chorus4> *m, const Surge::Storage::FxUserPreset::Preset &ps)
{
    typedef FX<fxt_chorus4> fx_t;
    typedef ChorusEffect<4> sx_t;
    m->params[fx_t::FX_SPECIFIC_PARAM_0].setValue(ps.da[sx_t::ch_lowcut] ? 0 : 1);
    m->params[fx_t::FX_SPECIFIC_PARAM_0 + 1].setValue(ps.da[sx_t::ch_highcut] ? 0 : 1);
}

template <>
bool FXConfig<fxt_chorus4>::isDirtyPresetVsSpecificParams(
    FX<fxt_chorus4> *m, const Surge::Storage::FxUserPreset::Preset &ps)
{
    typedef FX<fxt_chorus4> fx_t;
    typedef ChorusEffect<4> sx_t;
    auto p0 = m->params[fx_t::FX_SPECIFIC_PARAM_0].getValue() > 0.5;
    auto p1 = m->params[fx_t::FX_SPECIFIC_PARAM_0 + 1].getValue() > 0.5;
    return !(p0 == !ps.da[sx_t::ch_lowcut] && p1 == !ps.da[sx_t::ch_highcut]);
}
} // namespace sst::surgext_rack::fx
#endif // RACK_HACK_CHORUS_H