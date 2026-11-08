// Standalone sequencer tests for GNARL. No JUCE, no audio hardware.
#include "sequencer/PatternGenerator.h"
#include "sequencer/StepScheduler.h"
#include "sequencer/MidiExporter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { ++failures; \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

using gnarl::sequencer::GeneratorSettings;
using gnarl::sequencer::Pattern;
using gnarl::sequencer::StepScheduler;
using gnarl::sequencer::TimedEvent;
using gnarl::sequencer::exportMidiFile;
using gnarl::sequencer::generatePattern;

static Pattern craftedPattern() {
    Pattern p;
    p.steps[0].noteOn = true; p.steps[0].rest = false; p.steps[0].note = 60; p.steps[0].velocity = 0.8f;
    p.steps[2].noteOn = true; p.steps[2].rest = false; p.steps[2].note = 64; p.steps[2].velocity = 0.5f;
    p.steps[2].glide = false;
    return p;
}

static void testDeterminism() {
    GeneratorSettings s;
    s.seed = 42;
    const Pattern a = generatePattern(s);
    const Pattern b = generatePattern(s);
    bool identical = true;
    for (size_t i = 0; i < a.steps.size(); ++i) {
        identical &= a.steps[i].noteOn == b.steps[i].noteOn
                  && a.steps[i].note == b.steps[i].note
                  && a.steps[i].velocity == b.steps[i].velocity
                  && a.steps[i].glide == b.steps[i].glide
                  && a.steps[i].accent == b.steps[i].accent;
    }
    CHECK(identical);

    GeneratorSettings s2 = s;
    s2.seed = 43;
    const Pattern c = generatePattern(s2);
    bool different = false;
    for (size_t i = 0; i < a.steps.size(); ++i)
        different |= a.steps[i].note != c.steps[i].note || a.steps[i].noteOn != c.steps[i].noteOn;
    CHECK(different);
}

static void testDensityAndRange() {
    GeneratorSettings s;
    s.seed = 7;
    s.density = 0.0f;
    Pattern empty = generatePattern(s);
    for (const auto& st : empty.steps) CHECK(st.rest && !st.noteOn);

    s.density = 1.0f;
    Pattern full = generatePattern(s);
    for (const auto& st : full.steps) {
        CHECK(!st.rest && st.noteOn);
        CHECK(st.note >= s.rootNote && st.note <= s.rootNote + 12 * (s.octaveRange + 1));
        CHECK(st.velocity > 0.0f && st.velocity <= 1.0f);
    }
}

static void testSchedulerTiming() {
    // 120 BPM at 48 kHz: one 16th-note step = 48000 * 60/120 / 4 = 6000 samples.
    StepScheduler sched;
    sched.setSampleRate(48000.0);
    sched.setTempo(120.0);
    sched.setPattern(craftedPattern());
    sched.start();
    CHECK(sched.isPlaying());

    std::vector<TimedEvent> all;
    for (int block = 0; block < 40; ++block) { // 40 * 512 = 20480 samples
        auto ev = sched.process(512);
        for (auto& e : ev) { e.samplePosition += block * 512LL; all.push_back(e); }
    }
    // Expect: on@0 (note 60), off@6000, on@12000 (note 64), off@18000.
    CHECK(all.size() == 4);
    if (all.size() == 4) {
        CHECK(all[0].noteOn && all[0].note == 60 && all[0].samplePosition == 0);
        CHECK(!all[1].noteOn && all[1].note == 60 && all[1].samplePosition == 6000);
        CHECK(all[2].noteOn && all[2].note == 64 && all[2].samplePosition == 12000);
        CHECK(!all[3].noteOn && all[3].note == 64 && all[3].samplePosition == 18000);
    }
}

static void testTempoChangeAtStepBoundary() {
    // 120 BPM @48k = 6000 samples/step. Switching to 240 BPM (3000 samples/step)
    // mid-step must take effect at the next step boundary: step boundaries at
    // 0, 6000, 12000 stay 120 BPM-spaced, the following one jumps to 15000.
    StepScheduler sched;
    sched.setSampleRate(48000.0);
    sched.setTempo(120.0);
    Pattern p;
    for (int i = 0; i < 3; ++i) {
        p.steps[static_cast<size_t>(i)].noteOn = true;
        p.steps[static_cast<size_t>(i)].rest = false;
        p.steps[static_cast<size_t>(i)].note = 60 + i;
        p.steps[static_cast<size_t>(i)].velocity = 1.0f;
    }
    sched.setPattern(p);
    sched.start();

    std::vector<TimedEvent> all;
    for (int block = 0; block < 40; ++block) { // 40 * 512 = 20480 samples
        if (block == 12) sched.setTempo(240.0); // after sample 6144: inside step 1
        auto ev = sched.process(512);
        for (auto& e : ev) { e.samplePosition += block * 512LL; all.push_back(e); }
    }
    // on@0, off+on@6000, off+on@12000, off@15000 (240 BPM step length).
    CHECK(all.size() == 6);
    auto has = [&](int64_t pos, bool on, int note) {
        return std::any_of(all.begin(), all.end(), [&](const TimedEvent& e) {
            return e.samplePosition == pos && e.noteOn == on && e.note == note;
        });
    };
    CHECK(has(0, true, 60));
    CHECK(has(6000, false, 60));
    CHECK(has(6000, true, 61));
    CHECK(has(12000, false, 61));
    CHECK(has(12000, true, 62));
    CHECK(has(15000, false, 62)); // 240 BPM boundary, not 18000
}

static void testStopEmitsNoteOffAndHalts() {
    StepScheduler sched;
    sched.setSampleRate(48000.0);
    sched.setTempo(120.0);
    sched.setPattern(craftedPattern());
    sched.start();
    sched.process(512); // note 60 (step 0) is sounding
    auto off = sched.stop();
    CHECK(!sched.isPlaying());
    CHECK(off.size() == 1 && !off[0].noteOn && off[0].note == 60);
    auto after = sched.process(512);
    CHECK(after.empty());
}

static void testNoStuckNotesAcrossPattern() {
    // Strict pairing (no glide ties): every note-on must get a note-off.
    for (double bpm : { 90.0, 150.0, 200.0 }) {
        StepScheduler sched;
        sched.setSampleRate(44100.0);
        sched.setTempo(bpm);
        GeneratorSettings s;
        s.seed = 1234;
        s.density = 0.9f;
        s.glideProbability = 0.0f;
        sched.setPattern(generatePattern(s));
        sched.start();
        std::map<int, int> openNotes; // note -> open count
        for (int block = 0; block < 400; ++block)
            for (const auto& e : sched.process(256)) {
                if (e.noteOn) ++openNotes[e.note];
                else --openNotes[e.note];
            }
        for (const auto& kv : sched.stop()) --openNotes[kv.note];
        for (const auto& kv : openNotes) CHECK(kv.second == 0);
    }
}

static void testGlideTieSuppressesIntermediateNoteOff() {
    // Step 0 ties into step 1: no note-off for note 60 between them.
    StepScheduler sched;
    sched.setSampleRate(48000.0);
    sched.setTempo(120.0); // 6000 samples/step
    Pattern p;
    p.steps[0].noteOn = true; p.steps[0].rest = false; p.steps[0].note = 60;
    p.steps[0].velocity = 0.9f; p.steps[0].glide = true;
    p.steps[1].noteOn = true; p.steps[1].rest = false; p.steps[1].note = 64;
    p.steps[1].velocity = 0.9f;
    sched.setPattern(p);
    sched.start();

    std::vector<TimedEvent> all;
    for (int block = 0; block < 10; ++block) { // 5120 samples: covers step 0 only
        auto ev = sched.process(512);
        for (auto& e : ev) { e.samplePosition += block * 512LL; all.push_back(e); }
    }
    CHECK(all.size() == 1);
    if (all.size() == 1) CHECK(all[0].noteOn && all[0].note == 60 && all[0].samplePosition == 0);

    auto ev = sched.process(2048); // crosses the 6000-sample step boundary
    bool sawOff60 = false, sawOn64 = false;
    for (const auto& e : ev) {
        if (!e.noteOn && e.note == 60) sawOff60 = true;
        if (e.noteOn && e.note == 64) sawOn64 = true;
    }
    CHECK(!sawOff60); // tie: no note-off for 60
    CHECK(sawOn64);
    auto off = sched.stop();
    CHECK(off.size() == 1 && !off[0].noteOn && off[0].note == 64);
}

// --- Minimal SMF parser used to validate the exporter output ---
struct ParsedMidi {
    int format = -1;
    int numTracks = -1;
    int division = -1;
    std::vector<std::pair<int, std::vector<uint8_t>>> events; // tick -> raw event bytes
    int tempoUsPerQuarter = -1;
    bool hasEndOfTrack = false;
};

static uint32_t readVlq(const std::vector<uint8_t>& d, size_t& i) {
    uint32_t v = 0;
    uint8_t b;
    do {
        b = d[i++];
        v = (v << 7) | (b & 0x7F);
    } while (b & 0x80);
    return v;
}

static ParsedMidi parseSmf(const std::vector<uint8_t>& d) {
    ParsedMidi m;
    size_t i = 0;
    auto u32 = [&]() { uint32_t v = (d[i] << 24) | (d[i+1] << 16) | (d[i+2] << 8) | d[i+3]; i += 4; return v; };
    auto u16 = [&]() { uint32_t v = (d[i] << 8) | d[i+1]; i += 2; return v; };
    CHECK(d.size() >= 14);
    CHECK(d[0] == 'M' && d[1] == 'T' && d[2] == 'h' && d[3] == 'd');
    i = 4;
    CHECK(u32() == 6);
    m.format = static_cast<int>(u16());
    m.numTracks = static_cast<int>(u16());
    m.division = static_cast<int>(u16());
    CHECK(d[i] == 'M' && d[i+1] == 'T' && d[i+2] == 'r' && d[i+3] == 'k');
    i += 4;
    const uint32_t trackLen = u32();
    const size_t trackEnd = i + trackLen;
    int tick = 0;
    while (i < trackEnd) {
        tick += static_cast<int>(readVlq(d, i));
        const uint8_t status = d[i++];
        if (status == 0xFF) {
            const uint8_t meta = d[i++];
            const uint32_t len = readVlq(d, i);
            if (meta == 0x51) {
                m.tempoUsPerQuarter = (d[i] << 16) | (d[i+1] << 8) | d[i+2];
            } else if (meta == 0x2F) {
                m.hasEndOfTrack = true;
            }
            i += len;
        } else if (status == 0x90 || status == 0x80) {
            m.events.push_back({ tick, { status, d[i], d[i+1] } });
            i += 2;
        } else {
            CHECK(false); // unexpected event type in our format-0 output
            break;
        }
    }
    return m;
}

static void testMidiExportValidity() {
    GeneratorSettings s;
    s.seed = 99;
    s.density = 1.0f;
    s.glideProbability = 0.0f;
    const Pattern p = generatePattern(s);
    const double bpm = 150.0;
    const auto data = exportMidiFile(p, bpm);
    CHECK(!data.empty());

    ParsedMidi m = parseSmf(data);
    CHECK(m.format == 0);
    CHECK(m.numTracks == 1);
    CHECK(m.division == 480);
    CHECK(m.tempoUsPerQuarter == 400000); // 60e6 / 150
    CHECK(m.hasEndOfTrack);

    // Every note-on must have a matching note-off at a later tick.
    std::map<int, std::vector<int>> open; // note -> on-ticks
    int maxTick = 0;
    for (const auto& ev : m.events) {
        const int tick = ev.first;
        const uint8_t status = ev.second[0];
        const int note = ev.second[1];
        maxTick = std::max(maxTick, tick);
        if (status == 0x90 && ev.second[2] > 0) open[note].push_back(tick);
        else {
            CHECK(!open[note].empty());
            if (!open[note].empty()) {
                CHECK(open[note].back() <= tick);
                open[note].pop_back();
            }
        }
    }
    for (const auto& kv : open) CHECK(kv.second.empty());
    CHECK(maxTick == 16 * 120); // one bar of 16 steps at 480 PPQN

    // All 16 steps had notes (density 1.0) => 16 note-ons.
    int noteOns = 0;
    for (const auto& ev : m.events) if (ev.second[0] == 0x90) ++noteOns;
    CHECK(noteOns == 16);
}

int main() {
    testDeterminism();
    testDensityAndRange();
    testSchedulerTiming();
    testTempoChangeAtStepBoundary();
    testStopEmitsNoteOffAndHalts();
    testNoStuckNotesAcrossPattern();
    testGlideTieSuppressesIntermediateNoteOff();
    testMidiExportValidity();
    if (failures == 0)
        std::printf("ALL SEQUENCER TESTS PASSED\n");
    else
        std::printf("%d CHECK(S) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
