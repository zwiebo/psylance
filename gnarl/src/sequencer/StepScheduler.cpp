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

} // namespace gnarl::sequencer
