// Standalone DSP tests for GNARL. No JUCE, no audio hardware.
// Build:  g++ -std=c++17 -I../src dsp_tests.cpp ../src/dsp/Voice.cpp -o dsp_tests
#include "dsp/Envelope.h"
#include "dsp/Oscillator.h"
#include "dsp/Filter.h"
#include "dsp/Waveshaper.h"
#include "dsp/FMSynth.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { ++failures; \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

static bool finite(float x) { return std::isfinite(x); }

static void testEnvelope() {
    gnarl::dsp::AdsrEnvelope env;
    env.setSampleRate(48000.0);
    env.setParams(0.01f, 0.1f, 0.5f, 0.1f);
    env.noteOn();
    float peak = 0.0f;
    for (int i = 0; i < 48000; ++i)
        peak = std::max(peak, env.process());
    CHECK(peak > 0.99f);
    CHECK(env.isActive());
    env.noteOff();
    float last = 1.0f;
    for (int i = 0; i < 48000 && env.isActive(); ++i)
        last = env.process();
    CHECK(!env.isActive());
    CHECK(last < 0.01f);
}

static void testOscillatorFrequency() {
    gnarl::dsp::SineOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(440.0);
    std::vector<float> buf(44100);
    for (auto& s : buf) s = osc.process();
    int crossings = 0;
    for (size_t i = 1; i < buf.size(); ++i)
        if (buf[i - 1] <= 0.0f && buf[i] > 0.0f) ++crossings;
    CHECK(std::abs(crossings - 440) <= 1);
    for (auto s : buf) CHECK(finite(s));
}

static void testFilterStability() {
    gnarl::dsp::StateVariableFilter f;
    f.setSampleRate(96000.0);
    f.setMode(gnarl::dsp::StateVariableFilter::Mode::LowPass);
    f.setResonance(0.95);
    f.setCutoff(18000.0);
    float maxAbs = 0.0f;
    float x = 1.0f;
    for (int i = 0; i < 96000; ++i) {
        float y = f.process(x);
        x = -x;
        CHECK(finite(y));
        maxAbs = std::max(maxAbs, std::abs(y));
    }
    CHECK(maxAbs < 100.0f);

    gnarl::dsp::StateVariableFilter fA, fB;
    for (auto* ff : { &fA, &fB }) {
        ff->setSampleRate(48000.0);
        ff->setMode(gnarl::dsp::StateVariableFilter::Mode::LowPass);
        ff->setCutoff(100.0);
        ff->setResonance(0.9);
    }
    double energy100 = 0.0, energy10k = 0.0;
    gnarl::dsp::SineOscillator o100, o10k;
    o100.setSampleRate(48000.0); o100.setFrequency(100.0);
    o10k.setSampleRate(48000.0); o10k.setFrequency(10000.0);
    for (int i = 0; i < 48000; ++i) {
        float y100 = fA.process(o100.process());
        float y10k = fB.process(o10k.process());
        if (i > 24000) { energy100 += y100 * y100; energy10k += y10k * y10k; }
    }
    CHECK(energy10k < energy100 * 0.01);
}

static void testWaveshaper() {
    for (float d = 0.0f; d <= 1.0f; d += 0.1f) {
        for (float x = -2.0f; x <= 2.0f; x += 0.05f) {
            float y = gnarl::dsp::softClip(x, d);
            CHECK(finite(y));
            CHECK(std::abs(y) <= 1.0f + 1e-5f);
        }
    }
}

static void runVoiceAt(double sampleRate, int blockSize) {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(sampleRate);
    gnarl::dsp::VoiceParams p;
    p.ratio = 2.0f; p.modIndex = 7.0f; p.fmEnvDecay = 0.1f;
    p.attack = 0.002f; p.decay = 0.2f; p.sustain = 0.8f; p.release = 0.1f;
    p.cutoff = 5000.0f; p.resonance = 0.8f; p.drive = 0.8f; p.glide = 0.05f;

    std::vector<float> buf(static_cast<size_t>(blockSize));
    float maxAbs = 0.0f;

    engine.noteOn(60, 1.0f);
    for (int block = 0; block < static_cast<int>(sampleRate / blockSize); ++block) {
        std::fill(buf.begin(), buf.end(), 0.0f);
        engine.render(buf.data(), blockSize, p);
        for (auto s : buf) {
            CHECK(finite(s));
            maxAbs = std::max(maxAbs, std::abs(s));
        }
    }
    CHECK(maxAbs > 0.01f);
    CHECK(maxAbs < 4.0f);

    engine.noteOff(60);
    double tailEnergy = 0.0;
    for (int block = 0; block < static_cast<int>(3.0 * sampleRate / blockSize); ++block) {
        std::fill(buf.begin(), buf.end(), 0.0f);
        engine.render(buf.data(), blockSize, p);
        for (auto s : buf) {
            CHECK(finite(s));
            if (block > static_cast<int>(2.0 * sampleRate / blockSize))
                tailEnergy += static_cast<double>(s) * s;
        }
    }
    CHECK(tailEnergy < 1e-6); // no hanging note after release
}

static void testSilenceOnIdle() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(48000.0);
    gnarl::dsp::VoiceParams p;
    std::vector<float> buf(512);
    for (int block = 0; block < 100; ++block) {
        std::fill(buf.begin(), buf.end(), 0.0f);
        engine.render(buf.data(), static_cast<int>(buf.size()), p);
        for (auto s : buf) CHECK(s == 0.0f);
    }
}

static void testAllNotesOffClearsVoice() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(48000.0);
    gnarl::dsp::VoiceParams p;
    p.attack = 0.001f; p.decay = 0.1f; p.sustain = 1.0f; p.release = 0.05f;
    std::vector<float> buf(256);
    engine.noteOn(60, 1.0f);
    engine.render(buf.data(), static_cast<int>(buf.size()), p);
    engine.allNotesOff();
    CHECK(!engine.isActive());
    double tailEnergy = 0.0;
    for (int block = 0; block < 20; ++block) {
        std::fill(buf.begin(), buf.end(), 0.0f);
        engine.render(buf.data(), static_cast<int>(buf.size()), p);
        for (auto s : buf) {
            CHECK(finite(s));
            if (block > 0) // skip the single bounded filter-tail block
                tailEnergy += static_cast<double>(s) * s;
        }
    }
    CHECK(tailEnergy < 1e-9);
}

static void testVoiceNoStuckNotes() {
    runVoiceAt(44100.0, 64);
    runVoiceAt(48000.0, 256);
    runVoiceAt(96000.0, 1024);
}

int main() {
    testEnvelope();
    testOscillatorFrequency();
    testFilterStability();
    testWaveshaper();
    testSilenceOnIdle();
    testAllNotesOffClearsVoice();
    testVoiceNoStuckNotes();
    if (failures == 0)
        std::printf("ALL DSP TESTS PASSED\n");
    else
        std::printf("%d CHECK(S) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
