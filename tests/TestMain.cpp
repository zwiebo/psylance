#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

int main()
{
    juce::MessageManager::getInstance();

    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failed = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (auto* result = runner.getResult (i))
            if (result->failures > 0)
                ++failed;
    }

    std::printf ("%d test result(s), %d failed\n", runner.getNumResults(), failed);
    return failed == 0 ? 0 : 1;
}
