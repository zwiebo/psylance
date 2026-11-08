#include "MidiExporter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gnarl::sequencer {

namespace {

int wrapIndex(int index) {
    int i = index % Pattern::kNumSteps;
    if (i < 0) i += Pattern::kNumSteps;
    return i;
}

uint8_t velocityToMidi(float v) {
    int m = 1 + static_cast<int>(std::lround(v * 126.0f));
    return static_cast<uint8_t>(std::clamp(m, 1, 127));
}

void appendVlq(std::vector<uint8_t>& out, uint32_t value) {
    uint8_t buf[4];
    int n = 0;
    buf[n++] = static_cast<uint8_t>(value & 0x7F);
    value >>= 7;
    while (value > 0) {
        buf[n++] = static_cast<uint8_t>((value & 0x7F) | 0x80);
        value >>= 7;
    }
    for (int i = n - 1; i >= 0; --i) out.push_back(buf[i]);
}

void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

void appendU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

} // namespace

std::vector<MidiNote> patternToMidiNotes(const Pattern& pattern, int ppq) {
    const int ticksPerStep = ppq / 4;  // 16 steps per 4 beats
    std::vector<MidiNote> out;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        const StepEvent& st = pattern.steps[i];
        if (!st.noteOn || st.rest) continue;
        // Steps covered by a preceding glide are merged into that note.
        const StepEvent& prev = pattern.steps[wrapIndex(i - 1)];
        if (prev.noteOn && !prev.rest && prev.glide) continue;
        int len = static_cast<int>(std::lround(noteLengthSteps(pattern, i) * ticksPerStep));
        if (len < 1) len = 1;
        out.push_back(MidiNote{i * ticksPerStep, i * ticksPerStep + len, st.note,
                               velocityToMidi(st.velocity)});
    }
    return out;
}

std::vector<uint8_t> buildMidiFile(const std::vector<MidiNote>& notes, double bpm, int ppq) {
    std::vector<uint8_t> trk;

    // Meta: tempo (microseconds per quarter note).
    const uint32_t usPerQuarter = static_cast<uint32_t>(std::lround(60000000.0 / bpm));
    appendVlq(trk, 0);
    trk.insert(trk.end(), {0xFF, 0x51, 0x03,
                           static_cast<uint8_t>(usPerQuarter >> 16),
                           static_cast<uint8_t>(usPerQuarter >> 8),
                           static_cast<uint8_t>(usPerQuarter)});

    // Meta: 4/4 time signature.
    appendVlq(trk, 0);
    trk.insert(trk.end(), {0xFF, 0x58, 0x04, 0x04, 0x02, 0x18, 0x08});

    // Meta: track name.
    const char* name = "gnarl";
    appendVlq(trk, 0);
    trk.push_back(0xFF);
    trk.push_back(0x03);
    trk.push_back(static_cast<uint8_t>(std::char_traits<char>::length(name)));
    trk.insert(trk.end(), name, name + std::char_traits<char>::length(name));

    // Note events sorted by tick; at equal ticks note-offs come first.
    struct Ev {
        int tick;
        bool off;
        uint8_t note;
        uint8_t velocity;
    };
    std::vector<Ev> evs;
    for (const auto& n : notes) {
        evs.push_back(Ev{n.startTick, false, static_cast<uint8_t>(n.note),
                         static_cast<uint8_t>(n.velocity)});
        evs.push_back(Ev{n.endTick, true, static_cast<uint8_t>(n.note), 0});
    }
    std::stable_sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.off < b.off;
    });

    int lastTick = 0;
    for (const auto& e : evs) {
        appendVlq(trk, static_cast<uint32_t>(std::max(0, e.tick - lastTick)));
        lastTick = e.tick;
        trk.push_back(static_cast<uint8_t>(e.off ? 0x80 : 0x90));
        trk.push_back(e.note);
        trk.push_back(e.velocity);
    }

    // End of track.
    appendVlq(trk, 0);
    trk.insert(trk.end(), {0xFF, 0x2F, 0x00});

    std::vector<uint8_t> file;
    file.insert(file.end(), {'M', 'T', 'h', 'd'});
    appendU32(file, 6);  // header length
    appendU16(file, 0);  // format 0: single track
    appendU16(file, 1);  // one track
    appendU16(file, static_cast<uint16_t>(ppq));
    file.insert(file.end(), {'M', 'T', 'r', 'k'});
    appendU32(file, static_cast<uint32_t>(trk.size()));
    file.insert(file.end(), trk.begin(), trk.end());
    return file;
}

std::vector<uint8_t> exportMidiFile(const Pattern& pattern, double bpm) {
    return buildMidiFile(patternToMidiNotes(pattern), bpm);
}

bool writeMidiFile(const std::string& path, const Pattern& pattern, double bpm, int ppq) {
    const auto bytes = buildMidiFile(patternToMidiNotes(pattern, ppq), bpm, ppq);
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const bool ok = std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    std::fclose(f);
    return ok;
}

} // namespace gnarl::sequencer
