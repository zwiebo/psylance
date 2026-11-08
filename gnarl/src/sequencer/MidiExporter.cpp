#include "MidiExporter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gnarl::sequencer {

namespace {

constexpr int kPpqn = 480;                 // ticks per quarter note
constexpr int kStepTicks = kPpqn / 4;      // one 16th-note step = 120 ticks

void appendU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

void appendVlq(std::vector<uint8_t>& out, uint32_t v) {
    uint8_t buf[5];
    int n = 0;
    buf[n++] = static_cast<uint8_t>(v & 0x7F);
    v >>= 7;
    while (v > 0) {
        buf[n++] = static_cast<uint8_t>((v & 0x7F) | 0x80);
        v >>= 7;
    }
    for (int i = n - 1; i >= 0; --i)
        out.push_back(buf[i]);
}

struct MidiEvent {
    int tick;
    bool noteOn;
    int note;
    uint8_t velocity;
};

} // namespace

std::vector<uint8_t> exportMidiFile(const Pattern& pattern, double bpm) {
    const double clampedBpm = bpm > 1.0 ? bpm : 120.0;
    const uint32_t usPerQuarter = static_cast<uint32_t>(std::llround(60000000.0 / clampedBpm));

    std::vector<MidiEvent> events;
    for (int i = 0; i < static_cast<int>(pattern.steps.size()); ++i) {
        const StepEvent& s = pattern.steps[static_cast<size_t>(i)];
        if (!s.noteOn || s.rest)
            continue;
        const int start = i * kStepTicks;
        const int len = std::clamp(static_cast<int>(s.length * kStepTicks), 1,
                                   static_cast<int>(pattern.steps.size()) * kStepTicks - start);
        const int vel127 = static_cast<int>(std::lround(s.velocity * 127.0f));
        const uint8_t vel = static_cast<uint8_t>(std::clamp(vel127, 1, 127));
        events.push_back({ start, true, s.note, vel });
        events.push_back({ start + len, false, s.note, 0 });
    }
    // Note-offs sort before note-ons at the same tick so a retriggered note
    // can never overlap itself.
    std::sort(events.begin(), events.end(), [](const MidiEvent& a, const MidiEvent& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return !a.noteOn && b.noteOn;
    });

    std::vector<uint8_t> track;
    // Tempo meta event at tick 0.
    appendVlq(track, 0);
    track.push_back(0xFF); track.push_back(0x51); track.push_back(0x03);
    track.push_back(static_cast<uint8_t>(usPerQuarter >> 16));
    track.push_back(static_cast<uint8_t>((usPerQuarter >> 8) & 0xFF));
    track.push_back(static_cast<uint8_t>(usPerQuarter & 0xFF));

    int lastTick = 0;
    for (const auto& e : events) {
        appendVlq(track, static_cast<uint32_t>(e.tick - lastTick));
        lastTick = e.tick;
        track.push_back(static_cast<uint8_t>(e.noteOn ? 0x90 : 0x80));
        track.push_back(static_cast<uint8_t>(std::clamp(e.note, 0, 127)));
        track.push_back(e.velocity);
    }
    // End of track.
    appendVlq(track, 0);
    track.push_back(0xFF); track.push_back(0x2F); track.push_back(0x00);

    std::vector<uint8_t> file;
    file.insert(file.end(), { 'M', 'T', 'h', 'd' });
    appendU32(file, 6);
    appendU16(file, 0);          // format 0
    appendU16(file, 1);          // one track
    appendU16(file, static_cast<uint16_t>(kPpqn));
    file.insert(file.end(), { 'M', 'T', 'r', 'k' });
    appendU32(file, static_cast<uint32_t>(track.size()));
    file.insert(file.end(), track.begin(), track.end());
    return file;
}

} // namespace gnarl::sequencer
