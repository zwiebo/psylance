#include "StepScheduler.h"

#include <algorithm>

namespace gnarl::sequencer {

double StepScheduler::stepDurationSamples() const {
    // One step = one 16th note = 0.25 beats. Clamped to >= 1 sample so the
    // scheduler can never spin inside a single block at extreme tempos.
    const double d = sampleRate * 60.0 * 0.25 / tempoBpm;
    return d > 1.0 ? d : 1.0;
}

void StepScheduler::start() {
    playing = true;
    stepIndex = 0;
    soundingNote = -1;
    prevGlide = false;
    nextStepSample = totalSamples;
}

std::vector<TimedEvent> StepScheduler::stop() {
    std::vector<TimedEvent> events;
    if (soundingNote >= 0)
        events.push_back({ 0, false, soundingNote, 0.0f });
    soundingNote = -1;
    prevGlide = false;
    playing = false;
    return events;
}

void StepScheduler::enterStep(int64_t atSample, std::vector<TimedEvent>& out) {
    const StepEvent& step = pattern.steps[static_cast<size_t>(stepIndex)];

    if (soundingNote >= 0 && !prevGlide) {
        out.push_back({ atSample, false, soundingNote, 0.0f });
        soundingNote = -1;
    }

    if (step.noteOn && !step.rest) {
        out.push_back({ atSample, true, step.note, step.accent ? 1.0f : step.velocity });
        soundingNote = step.note;
    } else if (soundingNote >= 0) {
        // A glide into a rest still has to release the note.
        out.push_back({ atSample, false, soundingNote, 0.0f });
        soundingNote = -1;
    }

    prevGlide = step.noteOn && !step.rest && step.glide;
    stepIndex = (stepIndex + 1) % kNumSteps;
    nextStepSample = atSample + static_cast<int64_t>(stepDurationSamples());
}

std::vector<TimedEvent> StepScheduler::process(int numSamples) {
    const int64_t blockStart = totalSamples;
    const int64_t blockEnd = totalSamples + numSamples;
    totalSamples = blockEnd;

    std::vector<TimedEvent> events;
    if (!playing || numSamples <= 0)
        return events;

    std::vector<TimedEvent> absolute;
    while (nextStepSample < blockEnd)
        enterStep(nextStepSample, absolute);

    for (const auto& e : absolute) {
        const int64_t rel = e.samplePosition - blockStart;
        if (rel >= 0 && rel < numSamples)
            events.push_back({ rel, e.noteOn, e.note, e.velocity });
    }
    std::sort(events.begin(), events.end(),
              [](const TimedEvent& a, const TimedEvent& b) { return a.samplePosition < b.samplePosition; });
    return events;
}

#include <cmath>

namespace gnarl::sequencer {

namespace {
constexpr double kStepBeats = 0.25;  // 16th note = quarter note / 4
constexpr double kPatternBeats = Pattern::kNumSteps * kStepBeats;
constexpr double kEps = 1e-9;

uint8_t velocityToMidi(float v) {
    int m = 1 + static_cast<int>(std::lround(v * 126.0f));
    return static_cast<uint8_t>(std::clamp(m, 1, 127));
}
} // namespace

void StepScheduler::setSampleRate(double sr) {
    if (sr <= 0.0) return;
    reanchor(m_sample);
    m_sampleRate = sr;
}

void StepScheduler::setTempo(double bpm) {
    if (bpm <= 0.0) return;
    reanchor(m_sample);
    m_tempo = bpm;
}

void StepScheduler::setGate(double gate) {
    m_gate = (gate > 0.0) ? gate : 1.0;
}

void StepScheduler::setLoop(bool enabled, double startBeats, double endBeats) {
    m_loopEnabled = enabled;
    if (endBeats > startBeats) {
        m_loopStartBeats = startBeats;
        m_loopEndBeats = endBeats;
    }
}

void StepScheduler::start() { m_playing = true; }

void StepScheduler::stop() {
    if (m_playing) cutActiveNotes(m_sample);
    m_playing = false;
}

void StepScheduler::setPositionBeats(double beats) {
    cutActiveNotes(m_sample);
    m_anchorBeats = beats;
    m_anchorSample = m_sample;
}

double StepScheduler::positionBeats() const { return beatsAt(m_sample); }

double StepScheduler::beatsAt(int64_t sample) const {
    return m_anchorBeats +
           static_cast<double>(sample - m_anchorSample) * (m_tempo / 60.0) / m_sampleRate;
}

int64_t StepScheduler::sampleAt(double beats) const {
    double s = m_anchorSample + (beats - m_anchorBeats) * m_sampleRate * 60.0 / m_tempo;
    return static_cast<int64_t>(std::llround(s));
}

void StepScheduler::reanchor(int64_t sample) {
    m_anchorBeats = beatsAt(sample);
    m_anchorSample = sample;
}

void StepScheduler::cutActiveNotes(int64_t sample) {
    for (const auto& n : m_active) {
        m_pending.push_back(ScheduledEvent{sample, 0x80, static_cast<uint8_t>(n.note), 0});
    }
    m_active.clear();
}

int StepScheduler::stepIndexAt(double beat) const {
    double phase;
    if (m_loopEnabled) {
        const double len = m_loopEndBeats - m_loopStartBeats;
        double b = beat;
        while (b >= m_loopEndBeats) b -= len;
        while (b < m_loopStartBeats) b += len;
        phase = b - m_loopStartBeats;
    } else {
        phase = std::fmod(beat, kPatternBeats);
        if (phase < 0.0) phase += kPatternBeats;
    }
    int idx = static_cast<int>(std::llround(phase / kStepBeats)) % Pattern::kNumSteps;
    if (idx < 0) idx += Pattern::kNumSteps;
    return idx;
}

std::vector<ScheduledEvent> StepScheduler::process(int64_t bufferStart, int64_t numSamples) {
    std::vector<ScheduledEvent> events;
    const int64_t end = bufferStart + numSamples;

    if (bufferStart != m_sample) {
        // Gap in the stream: treat as a transport jump.
        cutActiveNotes(m_sample);
        reanchor(m_sample);
        m_anchorSample = bufferStart;
    }
    m_sample = bufferStart;

    // Deliver pending events (note-offs from stop/seek) due within this buffer.
    for (auto it = m_pending.begin(); it != m_pending.end();) {
        if (it->sample >= bufferStart && it->sample < end) {
            events.push_back(*it);
            it = m_pending.erase(it);
        } else {
            ++it;
        }
    }

    if (m_playing) {
        const double startBeat = beatsAt(bufferStart);
        const double endBeat = beatsAt(end);

        // 1) Note-offs of notes started in earlier buffers that fall in here.
        for (auto it = m_active.begin(); it != m_active.end();) {
            if (it->offSample >= bufferStart && it->offSample < end) {
                events.push_back(ScheduledEvent{it->offSample, 0x80,
                                                static_cast<uint8_t>(it->note), 0});
                it = m_active.erase(it);
            } else {
                ++it;
            }
        }

        // 2) Note-ons for every step onset inside this buffer.
        const double offset = m_loopEnabled ? m_loopStartBeats : 0.0;
        double first = std::ceil((startBeat - offset) / kStepBeats - kEps) * kStepBeats + offset;
        for (double onset = first; onset < endBeat - kEps; onset += kStepBeats) {
            const int idx = stepIndexAt(onset);
            const StepEvent& st = m_pattern.steps[idx];
            if (!st.noteOn || st.rest) continue;
            // A step covered by the previous step's glide is not re-articulated.
            const StepEvent& prev = m_pattern.steps[(idx + Pattern::kNumSteps - 1) % Pattern::kNumSteps];
            if (prev.noteOn && !prev.rest && prev.glide) continue;

            int64_t onSample = std::max(sampleAt(onset), bufferStart);
            double offBeat = onset + noteLengthSteps(m_pattern, idx) * kStepBeats * m_gate;
            if (m_loopEnabled) {
                const double len = m_loopEndBeats - m_loopStartBeats;
                double limit = m_loopEndBeats;
                while (limit <= onset + kEps) limit += len;
                if (offBeat > limit) offBeat = limit;  // cut at the loop boundary
            }
            int64_t offSample = std::max(sampleAt(offBeat), onSample + 1);

            events.push_back(ScheduledEvent{onSample, 0x90, static_cast<uint8_t>(st.note),
                                            velocityToMidi(st.velocity)});
            if (offSample < end) {
                events.push_back(ScheduledEvent{offSample, 0x80, static_cast<uint8_t>(st.note), 0});
            } else {
                m_active.push_back(ActiveNote{st.note, offSample});
            }
        }
    }

    m_sample = end;
    std::stable_sort(events.begin(), events.end(),
                     [](const ScheduledEvent& a, const ScheduledEvent& b) {
                         if (a.sample != b.sample) return a.sample < b.sample;
                         return a.isNoteOn() < b.isNoteOn();  // note-offs first
                     });
    return events;
}

void dispatchEvents(MidiOutputSink& sink, const std::vector<ScheduledEvent>& events,
                    int64_t bufferStart) {
    for (const auto& e : events) {
        sink.send(e.sample - bufferStart, e.status, e.note, e.velocity);
    }
}

} // namespace gnarl::sequencer
