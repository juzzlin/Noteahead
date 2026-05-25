// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#ifndef SYNTH_CONTROLLER_TEST_HPP
#define SYNTH_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class SynthControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sampleRateChange_shouldUpdateHzValues();
};

} // namespace noteahead

#endif // SYNTH_CONTROLLER_TEST_HPP
