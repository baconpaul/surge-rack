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

#include "DualFunction.h"
#include "XTModuleWidget.h"
#include "XTWidgets.h"
#include "SurgeXT.h"
#include <unordered_map>

namespace sst::surgext_rack::dualfunction::ui
{
struct DualFunctionWidget : public widgets::XTModuleWidget
{
    typedef sst::surgext_rack::dualfunction::DualFunction M;
    DualFunctionWidget(M *module);

    std::array<std::array<rack::Widget *, M::n_mod_inputs>, M::n_mod_params> overlays;
    std::array<widgets::ModulatableKnob *, M::n_mod_params> underKnobs;
    std::array<widgets::ModToggleButton *, M::n_mod_inputs> toggles;

    void selectModulator(int mod) override
    {
        if (toggles[mod])
            toggles[mod]->onToggle(!toggles[mod]->pressedState);
    }

    void appendModuleSpecificMenu(rack::ui::Menu *menu) override
    {
        if (!module)
            return;

    }
    widgets::BufferedDrawFunctionWidget *divWidget{nullptr};
    void drawDividingLines(NVGcontext *vg)
    {
        if (!divWidget)
            return;

        auto bs = divWidget->box.size;

        auto col = style()->getColor(style::XTStyle::Colors::PLOT_MARKS);
        auto sw = 0.75;

        nvgBeginPath(vg);
        nvgStrokeColor(vg, col);
        nvgStrokeWidth(vg, sw);
        nvgMoveTo(vg, 2, rack::mm2px(5));
        nvgLineTo(vg, bs.x - 2, rack::mm2px(5));
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokeColor(vg, col);
        nvgStrokeWidth(vg, sw);
        nvgMoveTo(vg, bs.x * 0.5, 1);
        nvgLineTo(vg, bs.x * 0.5, bs.y - 2);
        nvgStroke(vg);
    }

    void step() override
    {
        if (module)
        {

        }
        rack::ModuleWidget::step();
    }
};

DualFunctionWidget::DualFunctionWidget(DualFunctionWidget::M *module)
{
    setModule(module);
    typedef layout::LayoutEngine<DualFunctionWidget, M::FP_0_0> engine_t;
    engine_t::initializeModulationToBlank(this);

    box.size = rack::Vec(rack::app::RACK_GRID_WIDTH * 12, rack::app::RACK_GRID_HEIGHT);

    auto bg = new widgets::Background(box.size, "Dual Function", "fx", "BlankNoDisplay");
    addChild(bg);

    /*auto portSpacing = layout::LayoutConstants::inputRowCenter_MM -
                       layout::LayoutConstants::modulationRowCenters_MM[1];*/
    auto portSpacing = 0.f;

    const auto row1 = layout::LayoutConstants::vcoRowCenters_MM[1] - portSpacing;
    const auto row2 = layout::LayoutConstants::vcoRowCenters_MM[0] - portSpacing;
    const auto row3 = layout::LayoutConstants::vcoRowCenters_MM[0] - portSpacing - (row1 - row2);

    typedef layout::LayoutItem li_t;
    engine_t::layoutItem(this, li_t::createLCDArea(row3 - rack::mm2px(2.5)), "DUAL FUNCTION");

    {
        auto lc = getFirstDescendantOfType<widgets::LCDBackground>();
        if (lc)
        {
            divWidget = new widgets::BufferedDrawFunctionWidget(
                lc->box.pos, lc->box.size, [this](auto vg) { drawDividingLines(vg); });
            addChild(divWidget);

#if 0
            float cw = lc->box.size.x * 0.25;
            float ch = lc->box.size.y - rack::mm2px(10);
            float cx0 = lc->box.pos.x;
            float cy0 = lc->box.pos.y + rack::mm2px(5);
            float pad = rack::mm2px(0.5);

            for (int i = 0; i < DualFunction::n_ads; ++i)
            {
                auto rbox = rack::math::Rect();
                rbox.pos = rack::Vec(cx0 + cw * i + pad, cy0 + pad);
                rbox.size = rack::Vec(cw - 2 * pad, ch - 2 * pad);
                rbox = rbox.shrink(rack::mm2px(rack::Vec(0.75, 0.75)));
                auto adar = ADARCurveDraw::create(rbox.pos, rbox.size, module, i);
                addChild(adar);
            }
#endif
        }
        else
        {
            std::cout << "NO LCD" << std::endl;
        }
    }
    return;
#if 0


    for (int i = 0; i < M::n_ads; ++i)
    {
        const auto col = layout::LayoutConstants::firstColumnCenter_MM +
                         i * layout::LayoutConstants::columnWidth_MM;
        // fixme use the enums
        // clang-format off
        std::vector<li_t> layout = {
          {li_t::KNOB9, "ATTACK", M::ATTACK_0 + i, col, row3},
          {li_t::KNOB9, "DECAY", M::DECAY_0 + i, col, row2},

          {li_t::PORT, "TRIG", M::TRIGGER_0 + i, col, row1},
        };
        // clang-format on
        layout[2].dynamicLabel = true;
        layout[2].dynLabelFn = [i](modules::XTModule *m) -> std::string {
            if (!m)
                return {"TRIG"};
            auto xtm = static_cast<M *>(m);
            auto isg = xtm->params[M::ADAR_0 + i].getValue() > 0.5;
            if (isg)
                return {"GATE"};
            return {"TRIG"};
        };

        for (const auto &lay : layout)
        {
            engine_t::layoutItem(this, lay, "QuadVCA");
        }

        auto cp = rack::mm2px(rack::Vec(col + layout::LayoutConstants::columnWidth_MM * 0.5, row1));
        auto trigLight =
            rack::createParamCentered<ThreeStateTriggerSwitch>(cp, module, M::LINK_TRIGGER_0 + i);
        addChild(trigLight);

        auto lcdw = rack::app::RACK_GRID_WIDTH * 12 - widgets::LCDBackground::posx * 2;
        auto w = lcdw / 4.0;

        {
            auto yAD = widgets::LCDBackground::posy;
            auto x = widgets::LCDBackground::posx + w * i;
            auto h = rack::mm2px(4.0);
            auto mode = widgets::PlotAreaToggleClick::create(rack::Vec(x, yAD), rack::Vec(w, h),
                                                             module, M::ADAR_0 + i);
            mode->align = widgets::PlotAreaToggleClick::CENTER;
            addChild(mode);
        }
        {
            auto yAD = rack::mm2px(row3) - rack::mm2px(11.5);
            auto xpad = rack::mm2px(0.5);
            auto x = widgets::LCDBackground::posx + w * i;
            auto lw = rack::mm2px(3.5);
            auto h = rack::mm2px(3.0);
            auto A = rack::createParam<widgets::CurveSwitch>(rack::Vec(x + xpad, yAD), module,
                                                             M::A_SHAPE_0 + i);
            A->box.size = rack::Vec(lw, h);
            A->box = A->box.shrink(rack::mm2px(rack::Vec(0.5, 0.25)));
            A->drawDirection = widgets::CurveSwitch::ATTACK;
            addChild(A);
            attackCurves[i] = A;

            auto D = rack::createParam<widgets::CurveSwitch>(rack::Vec(x + w - lw - xpad, yAD),
                                                             module, M::D_SHAPE_0 + i);
            D->box.size = rack::Vec(lw, h);
            D->box = D->box.shrink(rack::mm2px(rack::Vec(0.5, 0.25)));
            D->drawDirection = widgets::CurveSwitch::FULL_RELEASE;
            addChild(D);
            decayCurves[i] = D;

            auto mode = widgets::PlotAreaToggleClick::create(
                rack::Vec(x + lw + xpad, yAD - rack::mm2px(1.0)),
                rack::Vec(w - 2 * lw - 2 * xpad, h + rack::mm2px(1.5)), module, M::MODE_0 + i);
            mode->align = widgets::PlotAreaToggleClick::CENTER;
            mode->getDisplay = [](auto p) {
                if (p->getValue() < 0.5)
                {
                    return "DI";
                }
                else
                {
                    return "AN";
                }
            };
            addChild(mode);
        }
    }

    engine_t::addModulationSection(this, M::n_mod_inputs, M::MOD_INPUT_0, -portSpacing);

    engine_t::addSingleOutputStripBackground(this, 0, 4);

    int kc = 0;
    for (int i = M::OUTPUT_0; i < M::OUTPUT_0 + M::n_ads; ++i)
    {
        auto yc = layout::LayoutConstants::inputRowCenter_MM;
        auto xc = layout::LayoutConstants::firstColumnCenter_MM +
                  layout::LayoutConstants::columnWidth_MM * kc;

        addOutput(
            rack::createOutputCentered<widgets::Port>(rack::mm2px(rack::Vec(xc, yc)), module, i));

        if (i != M::OUTPUT_0 + M::n_ads - 1)
        {
            auto cp =
                rack::mm2px(rack::Vec(xc + layout::LayoutConstants::columnWidth_MM * 0.5, yc));
            auto sumLight = rack::createParamCentered<widgets::ActivateKnobSwitch>(
                cp, module, M::LINK_ENV_0 + i);
            addChild(sumLight);
        }
        auto bl = layout::LayoutConstants::inputLabelBaseline_MM;
        auto lab = engine_t::makeLabelAt(bl, kc, "ENV" + std::to_string(i - M::OUTPUT_0 + 1),
                                         style::XTStyle::TEXT_LABEL_OUTPUT);
        addChild(lab);
        kc++;
    }
#endif

    resetStyleCouplingToModule();
}
} // namespace sst::surgext_rack::DualFunction::ui
// namespace sst::surgext_rack::DualFunction::ui

rack::Model *modelDualFunction =
    rack::createModel<sst::surgext_rack::dualfunction::ui::DualFunctionWidget::M,
                      sst::surgext_rack::dualfunction::ui::DualFunctionWidget>("SurgeXTDualFunction");
