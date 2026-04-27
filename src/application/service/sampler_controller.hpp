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

#ifndef SAMPLER_CONTROLLER_HPP
#define SAMPLER_CONTROLLER_HPP

#include <QObject>
#include <memory>

namespace noteahead {

class SamplerDevice;
class SamplerPadModel;

class SamplerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(noteahead::SamplerPadModel * padModel READ padModel CONSTANT)
    Q_PROPERTY(int selectedPad READ selectedPad WRITE setSelectedPad NOTIFY selectedPadChanged)
    Q_PROPERTY(double playbackPosition READ playbackPosition NOTIFY playbackPositionChanged)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY isFinishedChanged)
    Q_PROPERTY(double selectedPadPan READ selectedPadPan WRITE setSelectedPadPan NOTIFY selectedPadPanChanged)
    Q_PROPERTY(double selectedPadVolume READ selectedPadVolume WRITE setSelectedPadVolume NOTIFY selectedPadVolumeChanged)
    Q_PROPERTY(bool channelMode READ channelMode WRITE setChannelMode NOTIFY channelModeChanged)

public:
    explicit SamplerController(std::shared_ptr<SamplerDevice> sampler, QObject * parent = nullptr);
    ~SamplerController() override;

    SamplerPadModel * padModel() const;
    std::shared_ptr<SamplerDevice> sampler() const;

    int selectedPad() const;
    void setSelectedPad(int selectedPad);

    double playbackPosition() const;
    bool isFinished() const;

    double selectedPadPan() const;
    void setSelectedPadPan(double pan);

    double selectedPadVolume() const;
    void setSelectedPadVolume(double volume);

    bool channelMode() const;
    void setChannelMode(bool enabled);

    Q_INVOKABLE QVariantList getWaveformData(int numPoints);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();

    Q_INVOKABLE void loadSample(int padIndex, const QString & filePath);
    Q_INVOKABLE void clearSample(int padIndex);
    Q_INVOKABLE void playSample(int padIndex, double velocity = 1.0);
    Q_INVOKABLE void stopSample(int padIndex);
    Q_INVOKABLE void updatePlaybackStatus();

signals:
    void selectedPadChanged();
    void playbackPositionChanged();
    void isFinishedChanged();
    void selectedPadPanChanged();
    void selectedPadVolumeChanged();
    void channelModeChanged();

private:
    std::shared_ptr<SamplerDevice> m_sampler;
    std::unique_ptr<SamplerPadModel> m_padModel;
    int m_selectedPad = 0;
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_HPP
