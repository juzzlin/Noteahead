// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#ifndef SAMPLER_CONTROLLER_TEST_HPP
#define SAMPLER_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class SamplerControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sampleRateChange_shouldUpdateHzValues();
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_TEST_HPP
