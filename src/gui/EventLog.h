#ifndef VAULT_HEIST_EVENT_LOG_H
#define VAULT_HEIST_EVENT_LOG_H

#include "../models/BinaryTree.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

class EventBus {
public:
    using Listener = std::function<void(const Event&, const std::string&)>;
    void subscribe(Listener listener) { listeners.push_back(std::move(listener)); }
    void publish(const Event& event, const std::string& state) const {
        for (const Listener& listener : listeners) listener(event, state);
    }
private:
    std::vector<Listener> listeners;
};

class EventLog {
public:
    void attach(EventBus& bus) {
        bus.subscribe([this](const Event& event, const std::string& state) {
            events.push_back(event);
            states.push_back(state);
        });
    }
    void append(EventType type, int nodeId = -1, int otherNodeId = -1,
                const std::string& state = {}) {
        Event event{};
        event.type = type;
        event.nodeId = nodeId;
        event.otherNodeId = otherNodeId;
        events.push_back(event);
        states.push_back(state);
    }

    void clear() { events.clear(); states.clear(); }
    size_t size() const { return events.size(); }
    bool empty() const { return events.empty(); }
    const Event& at(size_t index) const { return events.at(index); }
    const std::string& stateAt(size_t index) const { return states.at(index); }
    const std::vector<Event>& all() const { return events; }

private:
    std::vector<Event> events;
    std::vector<std::string> states;
};

#endif
