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

#ifndef DRUM_SYNTH_909_DIALOG_MODEL_HPP
#define DRUM_SYNTH_909_DIALOG_MODEL_HPP

#include "../../../view/controllers/drum_pad_dialog_model.hpp"

#include <QVariantList>

namespace noteahead {

//! The Drum Synth 909 dialog's controller: the selected drum's own knobs on top of what every drum
//! machine has. The knobs differ from drum to drum, so they are listed rather than each made a
//! property, and read and written by key.
class DrumSynth909DialogModel : public DrumPadDialogModel
{
    Q_OBJECT

    //! The selected drum's knobs, in the RD-9 panel's order: each a map of its key and its label
    Q_PROPERTY(QVariantList voiceKnobs READ voiceKnobs NOTIFY voiceKnobsChanged)
    //! Bumped whenever any knob may have moved, for the bindings that read them by key
    Q_PROPERTY(int knobRevision READ knobRevision NOTIFY knobsChanged)

public:
    explicit DrumSynth909DialogModel(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    QVariantList voiceKnobs() const;
    int knobRevision() const;

    //! The selected drum's knob @p key as a position scaled by Constants::uiInternalScaling()
    Q_INVOKABLE int knob(const QString & key) const;
    Q_INVOKABLE void setKnob(const QString & key, int value);

    //! The device's kits: Init, Mono, Equal and Mono Equal. The rest are the user's own.
    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    void requestSettings() override;
    void retranslate() override;

signals:
    void voiceKnobsChanged();
    void knobsChanged();

protected:
    bool acceptsDevice(const DeviceS & device) const override;

private:
    int m_knobRevision { 0 };
};

} // namespace noteahead

#endif // DRUM_SYNTH_909_DIALOG_MODEL_HPP
