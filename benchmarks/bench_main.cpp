/*
    MultiScoper Google Benchmark entry point.

    Initializes the JUCE MessageManager on the main thread before running
    benchmarks so that MessageManager::isThisTheMessageThread() returns true
    from any benchmark code. SignalProcessor::ProcessedSignal::resize() (see
    include/core/dsp/SignalProcessor.h) gates dynamic allocation on that
    predicate; without a live MessageManager owned by the benchmark thread
    the ProcessedSignal workload would silently skip work.

    Cleans up the MessageManager after all benchmarks complete.
*/

#include <juce_events/juce_events.h>

#include <benchmark/benchmark.h>

int main(int argc, char** argv)
{
    // Creates the MessageManager on the current thread. From this point on
    // isThisTheMessageThread() returns true for this thread, which is the
    // thread Google Benchmark will run every benchmark on.
    juce::MessageManager::getInstance();

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    {
        juce::MessageManager::deleteInstance();
        return 1;
    }

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    juce::MessageManager::deleteInstance();
    return 0;
}
