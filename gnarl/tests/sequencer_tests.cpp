// Standalone sequencer tests for GNARL. No JUCE, no audio hardware.
// Build:  g++ -std=c++17 -Isrc sequencer_tests.cpp src/sequencer/*.cpp -o sequencer_tests
#include "sequencer/MidiExporter.h"
#include "sequencer/PatternGenerator.h"
#include "sequencer/StepScheduler.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace gnarl::sequencer;

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { ++failures; \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

// ---------------------------------------------------------------- theory

static void test_scales() {
    CHECK(scaleIntervals(ScalePhrygianDominant) ==
          std::vector<int>({0, 1, 4, 5, 7, 8, 10}));
    CHECK(numScales() == 7);
    CHECK(isNoteInScale(45, 45, ScalePhrygianDominant));
    CHECK(isNoteInScale(46, 45, ScalePhrygianDominant));   // minor 2nd
    CHECK(!isNoteInScale(47, 45, ScalePhrygianDominant));  // natural 2nd is not in it
    CHECK(noteName(69) == "A4");
    CHECK(noteName(45) == "A2");
    CHECK(noteName(33) == "A1");
    auto notes = scaleNotesInRange(36, ScalePhrygianDominant, 33, 60);
    CHECK(!notes.empty());
    for (int n : notes) {
        CHECK(n >= 33 && n <= 60);
        CHECK(isNoteInScale(n, 36, ScalePhrygianDominant));
    }
}

// ---------------------------------------------------------------- pattern

static GeneratorSettings settings(uint32_t seed) {
    GeneratorSettings s;
    s.seed = seed;
    return s;
}

static void test_determinism() {
    Pattern a = generatePattern(settings(1234));
    Pattern b = generatePattern(settings(1234));
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        CHECK(a.steps[i].noteOn == b.steps[i].noteOn);
        CHECK(a.steps[i].note == b.steps[i].note);
        CHECK(a.steps[i].velocity == b.steps[i].velocity);
        CHECK(a.steps[i].glide == b.steps[i].glide);
    }
    // Regenerating with the same seed reproduces the pattern.
    Pattern c = generatePattern(settings(999));
    regeneratePattern(c, settings(999));
    Pattern d = generatePattern(settings(999));
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        CHECK(c.steps[i].note == d.steps[i].note);
        CHECK(c.steps[i].noteOn == d.steps[i].noteOn);
    }
    // A different seed changes the pattern.
    Pattern e = generatePattern(settings(1235));
    bool differs = false;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        if (a.steps[i].note != e.steps[i].note || a.steps[i].noteOn != e.steps[i].noteOn)
            differs = true;
    }
    CHECK(differs);
}

static void test_constraints() {
    for (uint32_t seed = 1; seed <= 60; ++seed) {
        GeneratorSettings s = settings(seed * 7919);
        Pattern p = generatePattern(s);
        for (int i = 0; i < Pattern::kNumSteps; ++i) {
            const StepEvent& st = p.steps[i];
            CHECK(st.rest == !st.noteOn);
            if (!st.noteOn) continue;
            CHECK(st.note >= s.pitchMin && st.note <= s.pitchMax);
            CHECK(isNoteInScale(st.note, s.rootNote, s.scale));
            CHECK(st.velocity > 0.0f && st.velocity <= 1.0f);
            if (st.accent) CHECK(st.velocity == 1.0f);
        }
    }
}

static void test_density() {
    GeneratorSettings s = settings(1);
    s.density = 0.0f;
    Pattern rests = generatePattern(s);
    for (int i = 0; i < Pattern::kNumSteps; ++i) CHECK(!rests.steps[i].noteOn);

    s.density = 1.0f;
    s.glideProbability = 0.0f;
    Pattern full = generatePattern(s);
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        CHECK(full.steps[i].noteOn);
        CHECK(!full.steps[i].glide);
    }
}

static void test_locks_and_edit() {
    GeneratorSettings s = settings(555);
    Pattern p = generatePattern(s);
    p.steps[3].locked = true;
    StepEvent saved = p.steps[3];
    regeneratePattern(p, settings(777));  // different seed: locked step survives
    CHECK(p.steps[3].note == saved.note);
    CHECK(p.steps[3].noteOn == saved.noteOn);
    CHECK(p.steps[3].velocity == saved.velocity);
    CHECK(p.steps[3].locked);
    CHECK(!p.steps[4].locked);

    // Manual edit of a step.
    StepEvent edit;
    edit.noteOn = true;
    edit.rest = false;
    edit.note = 40;
    edit.velocity = 0.7f;
    p.steps[5] = edit;
    p.steps[5].locked = false;
    CHECK(p.steps[5].note == 40);
}

static void test_glide_run() {
    Pattern p;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        StepEvent s;
        s.noteOn = true;
        s.rest = false;
        s.note = 36 + i;
        p.steps[i] = s;
    }
    p.steps[0].glide = true;
    p.steps[1].glide = true;
    CHECK(glideRunLength(p, 0) == 3);
    CHECK(glideRunLength(p, 1) == 2);
    CHECK(glideRunLength(p, 2) == 1);
    CHECK(noteLengthSteps(p, 0) == 3.0f);
    // Glide into a rest does not extend.
    p.steps[2].rest = true;
    p.steps[2].noteOn = false;
    CHECK(glideRunLength(p, 0) == 2);
    // All glides: capped at kNumSteps.
    p.steps[2].rest = false;
    p.steps[2].noteOn = true;
    for (int i = 0; i < Pattern::kNumSteps; ++i) p.steps[i].glide = true;
    CHECK(glideRunLength(p, 0) == Pattern::kNumSteps);
}

// ---------------------------------------------------------------- scheduler

static Pattern allActivePattern() {
    Pattern p;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        StepEvent s;
        s.noteOn = true;
        s.rest = false;
        s.note = 45;
        s.velocity = 0.8f;
        p.steps[i] = s;
    }
    return p;
}

static std::vector<ScheduledEvent> runTransport(StepScheduler& sch, int64_t totalSamples,
                                                int64_t blockSize) {
    std::vector<ScheduledEvent> all;
    for (int64_t start = 0; start < totalSamples; start += blockSize) {
        auto evs = sch.process(start, std::min(blockSize, totalSamples - start));
        all.insert(all.end(), evs.begin(), evs.end());
    }
    return all;
}

static void test_basic_timing() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);  // 1 beat = 24000 samples, 1 step = 6000 samples
    sch.setLoop(false, 0.0, 4.0);
    sch.setPattern(allActivePattern());
    sch.start();

    auto evs = sch.process(0, 48000);  // 1 s = 2 beats = 8 steps
    int ons = 0, offs = 0;
    for (const auto& e : evs) {
        if (e.isNoteOn()) {
            CHECK(e.sample == ons * 6000);
            CHECK(e.note == 45 && e.velocity == 1 + static_cast<int>(std::lround(0.8f * 126.0f)));
            ++ons;
        } else {
            ++offs;
        }
    }
    CHECK(ons == 8);
    CHECK(offs == 7);  // last note-off falls into the next buffer
    CHECK(sch.activeNoteCount() == 1);
}

static void test_buffer_independence() {
    auto run = [](int64_t block) {
        StepScheduler sch;
        sch.setSampleRate(44100.0);
        sch.setTempo(142.0);
        sch.setPattern(generatePattern(settings(2024)));
        sch.start();
        return runTransport(sch, 44100 * 4, block);  // 4 seconds
    };
    auto a = run(4096);
    auto b = run(1000);
    auto c = run(1);
    CHECK(a.size() == b.size());
    CHECK(a.size() == c.size());
    for (size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].sample == b[i].sample && a[i].sample == c[i].sample);
        CHECK(a[i].status == b[i].status && a[i].note == b[i].note &&
              a[i].velocity == b[i].velocity);
    }
}

static void test_noteoff_across_boundary() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);
    sch.setLoop(false, 0.0, 4.0);
    sch.setPattern(allActivePattern());
    sch.start();
    sch.process(0, 6000);  // step 0: on at 0, off at 6000 (next buffer)
    auto evs = sch.process(6000, 6000);
    bool sawOff = false, sawOn = false;
    for (const auto& e : evs) {
        if (e.isNoteOff() && e.sample == 6000) sawOff = true;
        if (e.isNoteOn() && e.sample == 6000) sawOn = true;
    }
    CHECK(sawOff);
    CHECK(sawOn);
}

static void test_glide_merges_notes() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);
    sch.setLoop(false, 0.0, 4.0);
    Pattern p;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        StepEvent s;
        s.noteOn = (i < 2);
        s.rest = (i >= 2);
        s.note = 45 + i;
        s.velocity = 0.8f;
        p.steps[i] = s;
    }
    p.steps[0].glide = true;  // step 1 is not re-articulated
    sch.setPattern(p);
    sch.start();
    auto evs = sch.process(0, 48000);
    int ons = 0, offs = 0;
    int64_t offSample = -1;
    for (const auto& e : evs) {
        if (e.isNoteOn()) ++ons;
        if (e.isNoteOff()) {
            ++offs;
            offSample = e.sample;
        }
    }
    CHECK(ons == 1);
    CHECK(offs == 1);
    CHECK(offSample == 12000);  // note-off at the end of step 1
}

static void test_stop_cuts_notes() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);
    sch.setPattern(allActivePattern());
    sch.start();
    sch.process(0, 12000);  // step 1's note is still sounding
    CHECK(sch.activeNoteCount() == 1);
    sch.stop();
    auto evs = sch.process(12000, 6000);
    bool cut = false;
    for (const auto& e : evs)
        if (e.isNoteOff() && e.sample == 12000) cut = true;
    CHECK(cut);
    CHECK(sch.activeNoteCount() == 0);
    CHECK(sch.process(18000, 24000).empty());  // nothing while stopped
}

static void test_transport_jump() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);
    sch.setLoop(false, 0.0, 4.0);
    sch.setPattern(allActivePattern());
    sch.start();
    sch.process(0, 12000);
    sch.setPositionBeats(4.0);  // jump ahead one bar
    CHECK(std::fabs(sch.positionBeats() - 4.0) < 1e-9);
    auto evs = sch.process(12000, 6000);
    bool cut = false, onAtJump = false;
    for (const auto& e : evs) {
        if (e.isNoteOff() && e.sample == 12000) cut = true;
        if (e.isNoteOn() && e.sample == 12000) onAtJump = true;
    }
    CHECK(cut);        // note from before the jump is cut at the jump sample
    CHECK(onAtJump);   // playback resumes at the new position (beat 4)
}

static void test_loop() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);
    sch.setLoop(true, 0.0, 4.0);
    sch.setPattern(allActivePattern());
    sch.start();
    // Two loop passes = 8 beats = 4 s = 192000 samples -> 32 note-ons.
    auto evs = runTransport(sch, 192000, 4096);
    int ons = 0;
    for (const auto& e : evs)
        if (e.isNoteOn()) ++ons;
    CHECK(ons == 32);
    // Every note-off matches a note-on; at most the final note is still open.
    std::multiset<int> open;
    for (const auto& e : evs) {
        if (e.isNoteOn()) open.insert(e.note);
        if (e.isNoteOff()) {
            CHECK(open.count(e.note) > 0);
            open.erase(open.find(e.note));
        }
    }
    CHECK(open.size() == static_cast<size_t>(sch.activeNoteCount()));
    CHECK(sch.activeNoteCount() <= 1);
}

static void test_tempo_change() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);  // step = 6000 samples
    sch.setLoop(false, 0.0, 4.0);
    sch.setPattern(allActivePattern());
    sch.start();
    sch.process(0, 48000);  // 8 steps at 120 bpm
    sch.setTempo(60.0);     // step = 12000 samples from here
    auto evs = sch.process(48000, 48000);
    std::vector<int64_t> onSamples;
    for (const auto& e : evs)
        if (e.isNoteOn()) onSamples.push_back(e.sample);
    CHECK(onSamples.size() == 4);  // 1 s at 60 bpm = 1 beat = 4 steps
    for (size_t i = 1; i < onSamples.size(); ++i)
        CHECK(onSamples[i] - onSamples[i - 1] == 12000);
    CHECK(onSamples.front() == 48000);
}

// ---------------------------------------------------------------- midi file

static void test_midi_file_bytes() {
    Pattern p = generatePattern(settings(31337));
    auto notes = patternToMidiNotes(p, 480);
    auto bytes1 = buildMidiFile(notes, 138.0, 480);
    auto bytes2 = buildMidiFile(notes, 138.0, 480);
    CHECK(bytes1 == bytes2);  // deterministic

    CHECK(bytes1.size() > 14);
    CHECK(bytes1[0] == 'M' && bytes1[1] == 'T' && bytes1[2] == 'h' && bytes1[3] == 'd');
    CHECK(bytes1[8] == 0 && bytes1[9] == 0);          // format 0
    CHECK(bytes1[10] == 0 && bytes1[11] == 1);         // one track
    CHECK(bytes1[12] == 0x01 && bytes1[13] == 0xE0);   // 480 PPQ
    CHECK(bytes1[14] == 'M' && bytes1[15] == 'T' && bytes1[16] == 'r' && bytes1[17] == 'k');

    // Minimal parser: walk the track, count events, find the tempo.
    size_t pos = 18;
    uint32_t trkLen = (bytes1[pos] << 24) | (bytes1[pos + 1] << 16) | (bytes1[pos + 2] << 8) |
                      bytes1[pos + 3];
    pos += 4;
    CHECK(trkLen == bytes1.size() - pos);
    auto readVlq = [&]() -> uint32_t {
        uint32_t v = 0;
        uint8_t b;
        do {
            b = bytes1[pos++];
            v = (v << 7) | (b & 0x7F);
        } while (b & 0x80);
        return v;
    };
    uint32_t tick = 0;
    int ons = 0, offs = 0, tempos = 0, endOfTrack = 0;
    uint32_t tempoUs = 0;
    while (pos < bytes1.size()) {
        tick += readVlq();
        uint8_t status = bytes1[pos++];
        if (status == 0xFF) {
            uint8_t meta = bytes1[pos++];
            uint32_t len = readVlq();
            if (meta == 0x51) {
                ++tempos;
                tempoUs = (bytes1[pos] << 16) | (bytes1[pos + 1] << 8) | bytes1[pos + 2];
            }
            if (meta == 0x2F) ++endOfTrack;
            pos += len;
        } else if ((status & 0xF0) == 0x90) {
            ++ons;
            pos += 2;
        } else {
            ++offs;
            pos += 2;
        }
    }
    CHECK(tempos == 1);
    CHECK(tempoUs == static_cast<uint32_t>(std::lround(60000000.0 / 138.0)));
    CHECK(endOfTrack == 1);
    CHECK(ons == static_cast<int>(notes.size()));
    CHECK(offs == ons);
}

static void test_midi_glide_merge_and_write() {
    Pattern p;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        StepEvent s;
        s.noteOn = (i < 3);
        s.rest = (i >= 3);
        s.note = 50 + i;
        s.velocity = 0.9f;
        p.steps[i] = s;
    }
    p.steps[0].glide = true;
    p.steps[1].glide = true;  // one merged note of 3 steps = 360 ticks at 480 PPQ
    auto notes = patternToMidiNotes(p, 480);
    CHECK(notes.size() == 1);
    CHECK(notes[0].startTick == 0);
    CHECK(notes[0].endTick == 360);
    CHECK(notes[0].note == 50);

    const std::string path = "/tmp/gnarl_sequencer_test.mid";
    CHECK(writeMidiFile(path, p, 150.0));
    std::FILE* f = std::fopen(path.c_str(), "rb");
    CHECK(f != nullptr);
    if (f) {
        std::fseek(f, 0, SEEK_END);
        long sz = std::ftell(f);
        std::fclose(f);
        CHECK(sz == static_cast<long>(exportMidiFile(p, 150.0).size()));
    }
}

// ---------------------------------------------------------------- live output

static void test_dispatch_events() {
    StepScheduler sch;
    sch.setSampleRate(48000.0);
    sch.setTempo(120.0);
    sch.setLoop(false, 0.0, 4.0);
    sch.setPattern(allActivePattern());
    sch.start();

    struct Recorder : MidiOutputSink {
        std::vector<std::array<int64_t, 4>> msgs;
        void send(int64_t sampleOffset, uint8_t status, uint8_t d1, uint8_t d2) override {
            msgs.push_back({sampleOffset, status, d1, d2});
        }
    } rec;

    auto evs = sch.process(0, 12000);
    dispatchEvents(rec, evs, 0);
    CHECK(rec.msgs.size() == evs.size());
    CHECK(rec.msgs[0][0] == 0 && rec.msgs[0][1] == 0x90);
}

// ---------------------------------------------------------------- main

int main() {
    test_scales();
    test_determinism();
    test_constraints();
    test_density();
    test_locks_and_edit();
    test_glide_run();
    test_basic_timing();
    test_buffer_independence();
    test_noteoff_across_boundary();
    test_glide_merges_notes();
    test_stop_cuts_notes();
    test_transport_jump();
    test_loop();
    test_tempo_change();
    test_midi_file_bytes();
    test_midi_glide_merge_and_write();
    test_dispatch_events();

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
