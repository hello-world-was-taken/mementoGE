#pragma once

#include "core/Event.h"

#include <algorithm>
#include <vector>

// An ordered batch shared by every reader through const FrameContext. Reading
// events never removes them from another system's input.
struct InputSnapshot
{
    // Uncapped for now: normal frame input batches are small, so retain every event.
    std::vector<Event> events;

    // Preserve ordered one-shot events across frames without a tick. Replace
    // held/pointer samples with the latest capture so each appears once per tick.
    void accumulate(const InputSnapshot &latest)
    {
        events.erase(std::remove_if(events.begin(), events.end(), isPersistent), events.end());
        events.insert(events.end(), latest.events.begin(), latest.events.end());
    }

    // Catch-up ticks retain current held inputs and pointer position, without
    // replaying presses, releases, movement deltas, scroll, or native key repeat.
    void clearTransientState()
    {
        events.erase(std::remove_if(events.begin(), events.end(),
                         [](const Event &event) { return !isPersistent(event); }),
            events.end());
    }

private:
    static bool isPersistent(const Event &event)
    {
        return event.type == EventType::KeyHeld || event.type == EventType::MouseButtonHeld ||
               event.type == EventType::MousePosition;
    }
};
