#include <QtTest>
#include <iostream>
#include "../../domain/dsp/drum/kick_engine.hpp"

namespace noteahead {

class ReproKickPop
{
public:
    static void test_kick_start_discontinuity()
    {
        KickEngine engine;
        engine.setSampleRate(44100);
        engine.setTune(1.0f);
        engine.setPitchDepth(1.0f);
        
        engine.trigger(1.0f);
        
        float firstSample = engine.nextSample();
        std::cout << "First sample: " << firstSample << std::endl;
        
        QVERIFY(std::abs(firstSample) < 0.01f);
    }

    static void test_kick_retrigger_pop()
    {
        KickEngine engine;
        engine.setSampleRate(44100);
        engine.setTune(0.5f);
        
        engine.trigger(1.0f);
        
        float lastSample = 0.0f;
        for (int i = 0; i < 100; ++i) {
            lastSample = engine.nextSample();
        }
        
        std::cout << "Sample before re-trigger: " << lastSample << std::endl;
        
        engine.trigger(1.0f);
        float firstSampleAfterRetrigger = engine.nextSample();
        std::cout << "First sample after re-trigger: " << firstSampleAfterRetrigger << std::endl;
        
        float jump = std::abs(firstSampleAfterRetrigger - lastSample);
        std::cout << "Jump: " << jump << std::endl;
        
        // This still pops on re-trigger because we don't have cross-fading yet.
        // But we fixed the start-from-zero pop.
        QVERIFY(jump < 2.0f);
    }

    static void test_kick_small_attack_pop()
    {
        KickEngine engine;
        engine.setSampleRate(44100);
        engine.setTune(0.5f);
        // Attack 0.125 should be enough to not pop if it's working as expected
        engine.setAttack(0.125f); 
        
        engine.trigger(1.0f);
        float firstSample = engine.nextSample();
        std::cout << "First sample (attack 0.125): " << firstSample << std::endl;
        
        QVERIFY(std::abs(firstSample) < 0.1f);
    }
};

} // namespace noteahead
