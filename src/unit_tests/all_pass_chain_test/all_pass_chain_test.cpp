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

#include "all_pass_chain_test.hpp"

#include "../../domain/dsp/all_pass_chain.hpp"

#include <QTest>
#include <cmath>
#include <numeric>
#include <vector>

namespace noteahead {

void AllPassChainTest::test_process_zeroCoefficient_shouldActAsPureDelay()
{
    // With coeff=0 each stage y[n] = x[n-1], so output is the input delayed by 'stages' samples.
    AllPassChain chain;
    chain.setStages(1);
    chain.setCoefficient(0.0);

    // First sample feeds into the delay — output is still 0 (prev x is 0)
    QCOMPARE(chain.process(1.0), 0.0);
    // Second sample: output is the first sample
    QCOMPARE(chain.process(0.0), 1.0);
}

void AllPassChainTest::test_process_nonzeroCoefficient_shouldPreserveEnergy()
{
    // An all-pass filter must have unit magnitude at all frequencies.
    // Driving it with an impulse and summing the squared output energy should
    // equal the input energy (1.0²) as the impulse response decays.
    AllPassChain chain;
    chain.setStages(4);
    chain.setCoefficient(0.1);

    double energy = 0.0;
    energy += std::pow(chain.process(1.0), 2.0);
    for (int i = 0; i < 500; i++) {
        energy += std::pow(chain.process(0.0), 2.0);
    }

    // Energy should be very close to 1.0
    QVERIFY2(std::abs(energy - 1.0) < 0.001,
             QString("Energy mismatch: %1").arg(energy).toUtf8().constData());
}

void AllPassChainTest::test_reset_shouldClearState()
{
    AllPassChain chain;
    chain.setStages(2);
    chain.setCoefficient(0.3);

    chain.process(1.0);
    chain.process(0.5);

    chain.reset();

    // After reset, output of a zero input must be zero
    QCOMPARE(chain.process(0.0), 0.0);
}

void AllPassChainTest::test_stages_shouldChainIndependently()
{
    // With coeff=0 and N stages, the impulse should appear at output sample N.
    AllPassChain chain;
    chain.setCoefficient(0.0);

    for (int stages = 1; stages <= 4; stages++) {
        chain.reset();
        chain.setStages(stages);

        double output = 0.0;
        output = chain.process(1.0);
        for (int i = 1; i < stages; i++) {
            output = chain.process(0.0);
        }
        QCOMPARE(output, 0.0); // impulse has not arrived yet

        output = chain.process(0.0); // one more step
        QCOMPARE(output, 1.0); // impulse arrives exactly here
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AllPassChainTest)
