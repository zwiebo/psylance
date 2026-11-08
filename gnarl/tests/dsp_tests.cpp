// Standalone DSP tests for GNARL. No JUCE, no audio hardware.
// Build:  g++ -std=c++17 -Isrc tests/dsp_tests.cpp src/dsp/FMSynth.cpp src/dsp/Envelope.cpp -o dsp_tests
#include "dsp/Envelope.h"
#include "dsp/Oscillator.h"
#include "dsp/Filter.h"
#include "dsp/Waveshaper.h"
#include "dsp/FMSynth.h"
#include "dsp/Patch.h"
#include "dsp/SoundMutation.h"

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

static void testEnvelopeTiming() {
    gnarl::dsp::AdsrEnvelope env;
    env.setSampleRate(48000.0);
    env.setParams(0.02f, 0.05f, 0.5f, 0.2f);
    env.noteOn();
    int attackSamples = 0;
    while (env.process() < 1.0f && attackSamples < 48000)
        ++attackSamples;
    CHECK(attackSamples >= static_cast<int>(0.02 * 48000) - 2);
    CHECK(attackSamples <= static_cast<int>(0.02 * 48000) + 2);
    float last = 1.0f;
    for (int i = 0; i < 48000; ++i)
        last = env.process();
    CHECK(std::abs(last - 0.5f) < 1e-3f);
    env.noteOff();
    int releaseSamples = 0;
    while (env.isActive() && releaseSamples < 96000) {
        last = env.process();
        ++releaseSamples;
    }
    CHECK(!env.isActive());
    CHECK(last < 1e-4f);
    CHECK(releaseSamples < static_cast<int>(3.9 * 0.2 * 48000));
    CHECK(releaseSamples > static_cast<int>(0.5 * 0.2 * 48000));

    gnarl::dsp::AdsrEnvelope edge;
    edge.setSampleRate(44100.0);
    edge.setParams(0.0f, 0.0f, 0.0f, 0.0f);
    edge.noteOn();
    for (int i = 0; i < 4410; ++i)
        CHECK(finite(edge.process()));
    edge.noteOff();
    for (int i = 0; i < 4410 && edge.isActive(); ++i)
        CHECK(finite(edge.process()));
    CHECK(!edge.isActive());
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

static void testFilterModes() {
    gnarl::dsp::SineOscillator o100, o1k, o10k;
    o100.setSampleRate(48000.0); o100.setFrequency(100.0);
    o1k.setSampleRate(48000.0); o1k.setFrequency(1000.0);
    o10k.setSampleRate(48000.0); o10k.setFrequency(10000.0);

    auto makeFilter = [](gnarl::dsp::StateVariableFilter::Mode m) {
        gnarl::dsp::StateVariableFilter f;
        f.setSampleRate(48000.0);
        f.setMode(m);
        f.setCutoff(1000.0);
        f.setResonance(0.3);
        return f;
    };
    auto bpA = makeFilter(gnarl::dsp::StateVariableFilter::Mode::BandPass);
    auto bpB = makeFilter(gnarl::dsp::StateVariableFilter::Mode::BandPass);
    double e1k = 0.0, e100 = 0.0;
    for (int i = 0; i < 48000; ++i) {
        const float y1 = bpA.process(o1k.process());
        const float y2 = bpB.process(o100.process());
        if (i > 24000) { e1k += y1 * y1; e100 += y2 * y2; }
    }
    CHECK(e1k > 10.0 * e100);

    auto hpA = makeFilter(gnarl::dsp::StateVariableFilter::Mode::HighPass);
    auto hpB = makeFilter(gnarl::dsp::StateVariableFilter::Mode::HighPass);
    double eH10k = 0.0, eH100 = 0.0;
    for (int i = 0; i < 48000; ++i) {
        const float y1 = hpA.process(o10k.process());
        const float y2 = hpB.process(o100.process());
        if (i > 24000) { eH10k += y1 * y1; eH100 += y2 * y2; }
    }
    CHECK(eH10k > 10.0 * eH100);

    for (int m = 0; m < 3; ++m) {
        gnarl::dsp::StateVariableFilter f;
        f.setSampleRate(96000.0);
        f.setMode(static_cast<gnarl::dsp::StateVariableFilter::Mode>(m));
        f.setCutoff(18000.0);
        f.setResonance(0.95);
        float x = 1.0f, maxAbs = 0.0f;
        for (int i = 0; i < 96000; ++i) {
            const float y = f.process(x);
            x = -x;
            CHECK(finite(y));
            maxAbs = std::max(maxAbs, std::abs(y));
        }
        CHECK(maxAbs < 100.0f);
    }
}

static void testWaveshaper() {
    for (float d = 0.0f; d <= 1.0f; d += 0.1f) {
        for (float x = -2.0f; x <= 2.0f; x += 0.05f) {
            float y = gnarl::dsp::softClip(x, d);
            CHECK(finite(y));
            CHECK(std::abs(y) <= 1.0f + 1e-5f);
            CHECK(std::abs(gnarl::dsp::softClip(-x, d) + y) < 1e-6f);
        }
    }
    CHECK(std::abs(gnarl::dsp::softClip(1.0f, 1.0f) - 1.0f) < 1e-6f);
    float prev = -2.0f;
    for (float x = -2.0f; x <= 2.0f; x += 0.01f) {
        const float y = gnarl::dsp::softClip(x, 0.5f);
        CHECK(y >= prev - 1e-6f);
        prev = y;
    }
}

static int countUpCrossings(const std::vector<float>& buf, int from, int to) {
    int crossings = 0;
    for (int i = from + 1; i < to; ++i)
        if (buf[i - 1] <= 0.0f && buf[i] > 0.0f)
            ++crossings;
    return crossings;
}

static void renderSeconds(gnarl::dsp::FMSynth& engine, std::vector<float>& buf,
                          double seconds, double sampleRate, const gnarl::dsp::VoiceParams& p) {
    const int total = static_cast<int>(seconds * sampleRate);
    buf.assign(static_cast<size_t>(total), 0.0f);
    const int blockSize = 256;
    int pos = 0;
    while (pos < total) {
        const int n = std::min(blockSize, total - pos);
        engine.render(buf.data() + pos, n, p);
        pos += n;
    }
}

static gnarl::dsp::VoiceParams pureToneParams() {
    gnarl::dsp::VoiceParams p;
    p.ratio = 1.0f; p.modIndex = 0.0f; p.fmEnvDecay = 0.1f; p.fmEnvAmount = 0.0f;
    p.attack = 0.001f; p.decay = 1.0f; p.sustain = 1.0f; p.release = 0.05f;
    p.cutoff = 18000.0f; p.resonance = 0.0f; p.drive = 0.0f; p.glide = 0.0f; p.gain = 1.0f;
    p.legato = 0.0f; p.filterMode = 0;
    return p;
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

static void testAllNotesOff() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(44100.0);
    auto p = pureToneParams();
    p.modIndex = 6.0f; p.fmEnvAmount = 1.0f;
    engine.noteOn(60, 1.0f);
    std::vector<float> buf(4096, 0.0f);
    engine.render(buf.data(), 4096, p);
    engine.allNotesOff();
    std::fill(buf.begin(), buf.end(), 0.0f);
    engine.render(buf.data(), 4096, p);
    float maxAbs = 0.0f;
    for (auto s : buf) {
        CHECK(finite(s));
        maxAbs = std::max(maxAbs, std::abs(s));
    }
    CHECK(maxAbs < 1e-6f);
    CHECK(!engine.isActive());
}

static void testNaNAndExtremeParams() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(96000.0);
    gnarl::dsp::VoiceParams p;
    p.ratio = std::nanf("");
    p.modIndex = HUGE_VALF;
    p.fmEnvDecay = -5.0f;
    p.fmEnvAmount = 99.0f;
    p.attack = 0.0f;
    p.decay = -1.0f;
    p.sustain = 7.0f;
    p.release = 0.0f;
    p.cutoff = 1e9f;
    p.resonance = 42.0f;
    p.drive = -3.0f;
    p.glide = -1.0f;
    p.gain = 1e6f;
    p.legato = std::nanf("");
    p.filterMode = 99;
    engine.noteOn(60, 1.0f);
    std::vector<float> buf(8192);
    float maxAbs = 0.0f;
    for (int block = 0; block < 12; ++block) {
        std::fill(buf.begin(), buf.end(), 0.0f);
        engine.render(buf.data(), 8192, p);
        for (auto s : buf) {
            CHECK(finite(s));
            maxAbs = std::max(maxAbs, std::abs(s));
        }
    }
    CHECK(maxAbs > 0.0f);
    CHECK(maxAbs <= 4.0f + 1e-4f);
    engine.noteOff(60);
    engine.allNotesOff();

    for (double sr : { 44100.0, 48000.0, 96000.0 }) {
        gnarl::dsp::FMSynth e2;
        e2.setSampleRate(sr);
        gnarl::dsp::VoiceParams q;
        q.ratio = 16.0f; q.modIndex = 12.0f; q.fmEnvDecay = 0.005f; q.fmEnvAmount = 1.0f;
        q.attack = 0.001f; q.decay = 4.0f; q.sustain = 0.0f; q.release = 0.005f;
        q.cutoff = 18000.0f; q.resonance = 0.95f; q.drive = 1.0f; q.glide = 0.0f; q.gain = 1.0f;
        q.legato = 1.0f; q.filterMode = 2;
        e2.noteOn(36, 1.0f);
        std::vector<float> b(static_cast<size_t>(sr));
        float m = 0.0f;
        for (int pos = 0; pos < static_cast<int>(sr); pos += 512) {
            const int n = std::min(512, static_cast<int>(sr) - pos);
            std::fill(b.begin() + pos, b.begin() + pos + n, 0.0f);
            e2.render(b.data() + pos, n, q);
            for (int i = 0; i < n; ++i) {
                CHECK(finite(b[pos + i]));
                m = std::max(m, std::abs(b[pos + i]));
            }
        }
        CHECK(m > 0.001f);
        CHECK(m <= 4.0f + 1e-4f);
        e2.noteOff(36);
    }
}

static void testParameterSmoothing() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(48000.0);
    auto p = pureToneParams();
    engine.noteOn(84, 1.0f);
    std::vector<float> buf;
    renderSeconds(engine, buf, 0.05, 48000.0, p);
    double rmsBefore = 0.0;
    for (int i = static_cast<int>(0.03 * 48000); i < static_cast<int>(0.05 * 48000); ++i)
        rmsBefore += static_cast<double>(buf[i]) * buf[i];
    rmsBefore = std::sqrt(rmsBefore / (0.02 * 48000));

    p.cutoff = 100.0f;
    float maxDelta = 0.0f, prev = buf.back();
    double rmsAfter = 0.0;
    int count = 0;
    for (int block = 0; block < static_cast<int>(0.3 * 48000 / 64); ++block) {
        std::vector<float> b(64, 0.0f);
        engine.render(b.data(), 64, p);
        for (int i = 0; i < 64; ++i) {
            maxDelta = std::max(maxDelta, std::abs(b[i] - prev));
            prev = b[i];
            if (block >= static_cast<int>(0.25 * 48000 / 64)) {
                rmsAfter += static_cast<double>(b[i]) * b[i];
                ++count;
            }
        }
    }
    rmsAfter = std::sqrt(rmsAfter / count);
    CHECK(maxDelta < 0.25f);
    CHECK(rmsAfter < 0.3 * rmsBefore);
}

static void testEngineFrequencyStability() {
    for (double sr : { 44100.0, 48000.0, 96000.0 }) {
        gnarl::dsp::FMSynth engine;
        engine.setSampleRate(sr);
        auto p = pureToneParams();
        engine.noteOn(69, 1.0f);
        std::vector<float> buf;
        renderSeconds(engine, buf, 1.0, sr, p);
        const int from = static_cast<int>(0.05 * sr);
        const double measured = countUpCrossings(buf, from, static_cast<int>(sr)) * sr / (sr - from);
        CHECK(std::abs(measured - 440.0) < 1.5);
        for (auto s : buf) CHECK(finite(s));
        engine.noteOff(69);
    }
}

static void testGlideAndLegato() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(44100.0);
    auto p = pureToneParams();
    p.glide = 0.15f;
    engine.noteOn(48, 1.0f);
    std::vector<float> buf;
    renderSeconds(engine, buf, 0.6, 44100.0, p);
    engine.noteOn(72, 1.0f);
    renderSeconds(engine, buf, 1.0, 44100.0, p);
    const int earlyFrom = 0, earlyTo = static_cast<int>(0.05 * 44100);
    const double earlyHz = countUpCrossings(buf, earlyFrom, earlyTo) * 44100.0 / (earlyTo - earlyFrom);
    const int lateFrom = static_cast<int>(0.5 * 44100), lateTo = 44100;
    const double lateHz = countUpCrossings(buf, lateFrom, lateTo) * 44100.0 / (lateTo - lateFrom);
    CHECK(earlyHz < 420.0);
    CHECK(std::abs(lateHz - 523.25) < 2.5);

    auto legatoPeak = [](float legato) {
        gnarl::dsp::FMSynth e;
        e.setSampleRate(44100.0);
        auto q = pureToneParams();
        q.legato = legato; q.glide = 0.02f; q.attack = 0.05f; q.decay = 0.02f; q.sustain = 0.2f;
        e.noteOn(60, 1.0f);
        std::vector<float> b;
        renderSeconds(e, b, 0.5, 44100.0, q);
        e.noteOn(67, 1.0f);
        renderSeconds(e, b, 0.06, 44100.0, q);
        float peak = 0.0f;
        for (auto s : b) peak = std::max(peak, std::abs(s));
        return peak;
    };
    const float peakLegato = legatoPeak(1.0f);
    const float peakRetrigger = legatoPeak(0.0f);
    CHECK(peakLegato < 0.45f);
    CHECK(peakRetrigger > 0.8f);
}

static void testSeededRandomization() {
    const auto a = gnarl::dsp::randomizeParams(1234u);
    const auto b = gnarl::dsp::randomizeParams(1234u);
    const auto c = gnarl::dsp::randomizeParams(4321u);
    const auto d = gnarl::dsp::mutateSoundSeeded(1234u);
    CHECK(a.ratio == b.ratio && a.modIndex == b.modIndex && a.cutoff == b.cutoff &&
          a.drive == b.drive && a.glide == b.glide && a.attack == b.attack &&
          a.decay == b.decay && a.sustain == b.sustain && a.release == b.release &&
          a.resonance == b.resonance && a.fmEnvDecay == b.fmEnvDecay &&
          a.fmEnvAmount == b.fmEnvAmount && a.gain == b.gain);
    CHECK(a.ratio == d.ratio && a.modIndex == d.modIndex && a.cutoff == d.cutoff);
    const bool anyDiff = a.ratio != c.ratio || a.modIndex != c.modIndex || a.cutoff != c.cutoff ||
        a.drive != c.drive || a.glide != c.glide || a.attack != c.attack ||
        a.resonance != c.resonance || a.fmEnvDecay != c.fmEnvDecay;
    CHECK(anyDiff);

    for (uint32_t seed = 0; seed < 64; ++seed) {
        const auto p = gnarl::dsp::randomizeParams(seed);
        CHECK(p.ratio >= 0.5f && p.ratio <= 8.0f);
        CHECK(p.modIndex >= 2.0f && p.modIndex <= 10.0f);
        CHECK(p.cutoff >= 800.0f && p.cutoff <= 14000.0f);
        CHECK(p.drive >= 0.1f && p.drive <= 0.8f);
        CHECK(p.resonance >= 0.1f && p.resonance <= 0.7f);
        CHECK(p.glide >= 0.0f && p.glide <= 0.25f);
        CHECK(p.sustain >= 0.4f && p.sustain <= 1.0f);
    }

    for (uint32_t seed = 0; seed < 16; ++seed) {
        gnarl::dsp::FMSynth engine;
        engine.setSampleRate(44100.0);
        const auto p = gnarl::dsp::randomizeParams(seed);
        engine.noteOn(48, 1.0f);
        std::vector<float> buf;
        renderSeconds(engine, buf, 0.2, 44100.0, p);
        float maxAbs = 0.0f;
        for (auto s : buf) {
            CHECK(finite(s));
            maxAbs = std::max(maxAbs, std::abs(s));
        }
        CHECK(maxAbs > 0.001f);
        CHECK(maxAbs <= 4.0f + 1e-4f);
        engine.noteOff(48);
    }
}

static void testFactoryPatches() {
    CHECK(gnarl::dsp::kNumFactoryPatches == 5);
    for (int i = 0; i < gnarl::dsp::kNumFactoryPatches; ++i) {
        const auto patch = gnarl::dsp::factoryPatch(i);
        CHECK(patch.name != nullptr);
        for (double sr : { 44100.0, 96000.0 }) {
            gnarl::dsp::FMSynth engine;
            engine.setSampleRate(sr);
            engine.noteOn(45, 0.9f);
            std::vector<float> buf;
            renderSeconds(engine, buf, 0.4, sr, patch.params);
            float maxAbs = 0.0f;
            for (auto s : buf) {
                CHECK(finite(s));
                maxAbs = std::max(maxAbs, std::abs(s));
            }
            CHECK(maxAbs > 0.005f);
            CHECK(maxAbs <= 4.0f + 1e-4f);
            engine.noteOff(45);
            renderSeconds(engine, buf, 3.0, sr, patch.params);
            double tail = 0.0;
            for (int k = static_cast<int>(2.0 * sr); k < static_cast<int>(3.0 * sr); ++k)
                tail += static_cast<double>(buf[k]) * buf[k];
            CHECK(tail < 1e-6);
        }
    }
}

static void testFMPresenceAndInstantGlide() {
    gnarl::dsp::FMSynth engine;
    engine.setSampleRate(44100.0);
    auto p = pureToneParams();
    p.modIndex = 8.0f; p.fmEnvAmount = 1.0f; p.fmEnvDecay = 1.0f;
    engine.noteOn(69, 1.0f);
    std::vector<float> buf;
    renderSeconds(engine, buf, 0.5, 44100.0, p);
    const int from = static_cast<int>(0.1 * 44100);
    const int fmCrossings = countUpCrossings(buf, from, 22050);
    engine.noteOff(69);
    engine.allNotesOff();

    gnarl::dsp::FMSynth engine2;
    engine2.setSampleRate(44100.0);
    engine2.noteOn(69, 1.0f);
    std::vector<float> buf2;
    renderSeconds(engine2, buf2, 0.5, 44100.0, pureToneParams());
    const int pureCrossings = countUpCrossings(buf2, from, 22050);
    CHECK(pureCrossings >= 174 && pureCrossings <= 178);
    CHECK(fmCrossings > 3 * pureCrossings);

    gnarl::dsp::FMSynth engine3;
    engine3.setSampleRate(44100.0);
    auto q = pureToneParams();
    q.glide = 0.0f;
    engine3.noteOn(72, 1.0f);
    std::vector<float> buf3;
    renderSeconds(engine3, buf3, 0.05, 44100.0, q);
    const double instantHz = countUpCrossings(buf3, 0, static_cast<int>(0.05 * 44100)) * 44100.0 / (0.05 * 44100);
    CHECK(std::abs(instantHz - 523.25) < 25.0);
}

int main() {
    testEnvelope();
    testEnvelopeTiming();
    testOscillatorFrequency();
    testFilterStability();
    testFilterModes();
    testWaveshaper();
    testSilenceOnIdle();
    testAllNotesOffClearsVoice();
    testVoiceNoStuckNotes();
    testAllNotesOff();
    testNaNAndExtremeParams();
    testParameterSmoothing();
    testEngineFrequencyStability();
    testGlideAndLegato();
    testSeededRandomization();
    testFactoryPatches();
    testFMPresenceAndInstantGlide();
    if (failures == 0)
        std::printf("ALL DSP TESTS PASSED\n");
    else
        std::printf("%d CHECK(S) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
