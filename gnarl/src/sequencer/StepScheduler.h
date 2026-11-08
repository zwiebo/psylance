#pragma once

#include <cstdint>
#include <vector>

#include "PatternGenerator.h"

namespace gnarl::sequencer {

// A scheduled MIDI channel message at an absolute sample position.
struct ScheduledEvent {
    int64_t sample = 0;    // absolute sample index in the audio stream
    uint8_t status = 0x90; // 0x9n note on, 0x8n note off (n = channel 0)
    uint8_t note = 0;      // MIDI note number 0..127
    uint8_t velocity = 0;  // 1..127 on note-on, 0 on note-off

    bool isNoteOn() const { return (status & 0xF0) == 0x90 && velocity > 0; }
    bool isNoteOff() const { return (status & 0xF0) == 0x80 || velocity == 0; }
};

// Sample-accurate 16-step sequencer (M3).
//
// The host calls process() once per audio buffer with the buffer's absolute
// start sample. Beat<->sample conversion uses an anchored, piecewise-integrated
// mapping, so tempo changes and seeks stay sample-accurate. Events are robust
// across buffer boundaries (note-offs of notes started in earlier buffers are
// tracked and emitted when due), transport jumps and stops (active notes are
// cut at the current sample), and loops (notes are cut at the loop end).
// Buffers must be contiguous; a gap is treated as a transport jump.
class StepScheduler {
public:
    void setPattern(const Pattern& p) { m_pattern = p; }
    const Pattern& pattern() const { return m_pattern; }

    void setSampleRate(double sr);
    void setTempo(double bpm);   // applies from the next processed sample
    double tempo() const { return m_tempo; }
    void setGate(double gate);   // note length as a fraction of the step run
    void setLoop(bool enabled, double startBeats, double endBeats);

    void start();
    void stop();                          // cuts all active notes at the current sample
    void setPositionBeats(double beats);  // transport jump; cuts active notes
    bool isPlaying() const { return m_playing; }
    double positionBeats() const;         // position at the current sample
    int64_t currentSample() const { return m_sample; }
    int activeNoteCount() const { return static_cast<int>(m_active.size()); }

    // Events for the buffer [bufferStart, bufferStart + numSamples), sorted by
    // sample; at equal samples note-offs precede note-ons.
    std::vector<ScheduledEvent> process(int64_t bufferStart, int64_t numSamples);

private:
    struct ActiveNote {
        int note;
        int64_t offSample;
    };

    double beatsAt(int64_t sample) const;
    int64_t sampleAt(double beats) const;
    void reanchor(int64_t sample);
    void cutActiveNotes(int64_t sample);
    int stepIndexAt(double beat) const;

    Pattern m_pattern{};
    bool m_playing = false;
    double m_sampleRate = 48000.0;
    double m_tempo = 150.0;
    double m_gate = 1.0;
    bool m_loopEnabled = true;
    double m_loopStartBeats = 0.0;
    double m_loopEndBeats = 4.0;  // 16 steps = 4 beats = one pattern pass
    int64_t m_sample = 0;
    int64_t m_anchorSample = 0;
    double m_anchorBeats = 0.0;
    std::vector<ActiveNote> m_active;
    std::vector<ScheduledEvent> m_pending;  // note-offs from stop()/seek
};

// Live MIDI output port for the VST3 host (M4): the plugin implements this and
// forwards each message to its juce::MidiBuffer at the given sample offset.
// Header-only so the sequencer stays JUCE-free and unit-testable.
class MidiOutputSink {
public:
    virtual ~MidiOutputSink() = default;
    virtual void send(int64_t sampleOffset, uint8_t status, uint8_t data1, uint8_t data2) = 0;
};

// Forwards a buffer of scheduled events to a sink, passing each event's offset
// relative to the buffer start.
void dispatchEvents(MidiOutputSink& sink, const std::vector<ScheduledEvent>& events,
                    int64_t bufferStart);

} // namespace gnarl::sequencer
