// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#ifndef DRUM_SYNTH_CONTROLLER_TEST_HPP
#define DRUM_SYNTH_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class DrumSynthControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sampleRateChange_shouldUpdateHzValues();
    void test_properties_shouldUpdateDeviceAndEmitSignals();
    void test_reset_shouldRestoreDefaultValues();
};

} // namespace noteahead

#endif // DRUM_SYNTH_CONTROLLER_TEST_HPP
