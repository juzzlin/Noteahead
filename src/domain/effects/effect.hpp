// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../dsp/dsp_component.hpp"
#include "../tracker/parameter_container.hpp"

namespace noteahead {

struct AudioContext;

class Effect : public DspComponent, public ParameterContainer
{
public:
    Effect() = default;
    virtual ~Effect() override;

    //! Mix and Solo are held as pointers into this object's own parameter map, so a copy has to
    //! re-resolve them against the copy's map rather than inherit pointers into the original's.
    Effect(const Effect & other);
    Effect & operator=(const Effect & other);
    Effect(Effect && other);
    Effect & operator=(Effect && other);

    //! How an effect's Mix control blends its output against the signal that came in.
    //!
    //! Which law applies is a property of the effect, not of the control: a crossfade is right for
    //! something that replaces the signal, and wrong for a reverb, whose Mix is really a send amount
    //! and whose dry has to stay whole.
    enum class MixLaw
    {
        //! dry * (1 - mix) + wet * mix. The usual blend for an effect that reshapes the signal.
        Crossfade,
        //! dry + wet * mix. The dry stays whole and the wet is added on top of it.
        Additive,
        //! Both at full across the middle of the travel, each fading out towards its own end. Gives
        //! a centre position where nothing is lost from either side.
        DualSlope,
        //! The effect blends its own Mix and this class leaves it alone. For an effect that shapes
        //! at an oversampled rate: its dry path is delayed by the same resampling filters as its
        //! wet one, and blending an undelayed dry against that would comb filter.
        Internal
    };

    virtual std::string type() const = 0;
    virtual std::string typeId() const = 0;

    //! The two entry points the rack calls. Deliberately not virtual: this is where the controls
    //! every effect shares are applied around the effect's own work, so that no effect has to
    //! remember to honour them and none can honour them differently.
    void process(double & left, double & right);
    void process(AudioContext & context);
    using StringList = std::vector<std::string>;
    virtual StringList parameterNames() const;

    virtual std::optional<size_t> sidechainSourceDeviceIndex() const
    {
        return std::nullopt;
    }

    bool enabled() const;
    void setEnabled(bool enabled);

    //! Whether this effect is running on a send bus rather than as an insert.
    //!
    //! A send bus returns what the effect added, worked out as the difference between what came
    //! back and the signal the bus handed over. That only holds if the dry survives the effect
    //! whole: a Mix law that fades the dry out turns the difference into an inverted copy of the
    //! source, which cancels it in the master mix instead of adding wet to it. In send mode every
    //! Mix law therefore blends additively -- the dry is passed through untouched and the wet is
    //! added on top -- so the whole travel of the control adds effect rather than removing source.
    //!
    //! Set per block by whichever path is processing the effect, so an effect moved between a send
    //! rack and an insert rack is right on the next block either way. Not a setting: never
    //! serialized, and not carried across a copy.
    void setSendMode(bool enabled);
    bool sendMode() const;

    virtual void reset() override;
    virtual void sync();
    virtual void setBpm(float bpm);
    float bpm() const;

    //! Whether the effect is back at rest, i.e. it would do nothing to a silent input. The engine
    //! stops processing a device once it has gone silent, which freezes the state of its whole
    //! insert rack. That is harmless for anything driven by the device's own signal, but a gain
    //! envelope following a detector -- a ducker, a compressor -- has somewhere to go on its own,
    //! and freezing it half way leaves the next note starting on stale gain reduction. Such an
    //! effect reports itself unsettled until its envelope has released, and the engine keeps the
    //! device running until then. True unless overridden: most effects have nothing to settle.
    virtual bool isSettled() const;

    //! Internal oversampling factor (1, 2 or 4) for nonlinear effects that render their shaping stage
    //! at a higher rate to suppress aliasing. Pushed per block from the AudioContext; 1 (no
    //! oversampling) unless set, so linear effects and direct per-sample use are unaffected.
    void setOversampleFactor(uint8_t factor);
    uint8_t oversampleFactor() const;

protected:
    //! The effect's own work on one frame.
    virtual void processSample(double & left, double & right) = 0;

    //! The effect's own work on a whole block, for anything that cannot be done a frame at a time:
    //! lookahead, side chains, transforms. Loops processSample() unless overridden.
    virtual void processBlock(AudioContext & context);

    //! Registers the Mix control with the shape the effect wants, and declares how it blends. The
    //! blending itself is done here rather than by the effect.
    void addMixParameter(float defaultValue, MixLaw law = MixLaw::Crossfade, int xmlMin = 0, int xmlMax = 10000, int xmlScale = 100, LegacyNameList legacyNames = {});

    //! For an effect that registers Mix itself, because its stored shape predates this.
    void setMixLaw(MixLaw law);

    //! Current Mix, or 1 for an effect that has no Mix control.
    float mix() const;

    //! Registers the Solo control, which passes only what the effect adds to the signal, so that it
    //! can be heard on its own. Effects that have something to add opt in; one that only shapes what
    //! is already there, an equalizer say, has nothing to isolate.
    void addSoloParameter();

    //! Whether Solo is registered and engaged.
    bool solo() const;

    //! The wet-carrying half of the blend, for an effect that shapes rather than adds and so
    //! blends under MixLaw::Internal. A plain crossfade as an insert; in send mode the wet alone,
    //! the dry being put back by completeBlend() once the output gain is known.
    //!
    //! Splitting it in two is what lets an effect blend inside its oversampled section, where the
    //! dry has to share the resampler's latency with the wet, and still hand a send bus the dry it
    //! started with. Returning a resampled copy of the dry instead would leave the difference the
    //! bus takes carrying a comb-filtered residual of the source.
    double blendWetPart(double dry, double wet, double mix) const;

    //! Applies the effect's output gain to a blend from blendWetPart(), putting the untouched dry
    //! back when in send mode. Output gain belongs to the wet path there: applying it to the dry as
    //! well would leave a scaled copy of the source in the difference the bus takes, at any Mix.
    double completeBlend(double dry, double blendedPart, double outputGain) const;

    //! blendWetPart() and completeBlend() in one, for an effect that blends at the base rate.
    double blendWet(double dry, double wet, double mix, double outputGain = 1.0) const;

private:
    //! Everything the shared controls need for one block, read once instead of once per sample.
    //! Resolving Mix and Solo means a map lookup keyed by a name that has to be built first, which
    //! is nothing per block and roughly as much as a reverb costs per sample.
    struct BlendState
    {
        MixLaw law { MixLaw::Crossfade };
        double mix { 1.0 };
        //! Whether the Mix law has anything to do at this setting.
        bool blends { false };
        bool solo { false };
        //! Collapses every law to Additive. See setSendMode().
        bool sendMode { false };
    };

    BlendState blendState() const;

    //! Blends the effect's output against the dry signal under the effect's own Mix law, then, when
    //! Solo is engaged, replaces the result with the difference between it and the dry signal, which
    //! is exactly what the effect contributed.
    void applyBlend(const BlendState & blend, double dryLeft, double dryRight, double & left, double & right) const;

    //! Points Mix and Solo at their entries in the parameter map, or at nothing when the effect did
    //! not register them. Map nodes are stable and nothing ever erases a parameter, so the pointers
    //! stay good for as long as this object does.
    void resolveSharedParameters();

    const Parameter * m_mixParameter { nullptr };
    const Parameter * m_soloParameter { nullptr };

    //! Dry copy for block-form blending, kept between blocks so the audio thread does not allocate.
    std::vector<double> m_dryBuffer;

    MixLaw m_mixLaw { MixLaw::Crossfade };
    bool m_enabled { true };
    bool m_sendMode { false };
    float m_bpm = 120;
    uint8_t m_oversampleFactor { 1 };
};

} // namespace noteahead

#endif // EFFECT_HPP
