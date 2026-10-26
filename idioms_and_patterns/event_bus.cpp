
#include <functional>
#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <chrono>
#include <stdexcept>

struct Event {
    std::string type;
    std::string data;
    std::chrono::system_clock::time_point timestamp;

    Event(const std::string& t, const std::string& d)
        : type(t),
          data(d),
          timestamp(std::chrono::system_clock::now()) {}
};

class EventBus {
private:
    using Handler = std::function<void(const Event&)>;

    std::map<std::string, std::vector<Handler>> subscribers;

public:
    // Accept lambdas, function objects, and compatible functions.
    void subscribe(const std::string& eventType,
                   const Handler& handler) {
        if (!handler) {
            throw std::invalid_argument("Handler cannot be empty");
        }

        subscribers[eventType].push_back(handler);
    }

    void publish(const Event& event) {
        auto it = subscribers.find(event.type);

        if (it == subscribers.end()) {
            return; // No subscribers for this event type.
        }

        // Copy handlers so subscriptions added during dispatch
        // do not invalidate the iteration.
        auto handlers = it->second;

        for (const auto& handler : handlers) {
            handler(event);
            // std::invoke(handler, event); // To avoid type erasure from std::function
        }
    }

    std::size_t getSubscriberCount(
        const std::string& eventType) const {
        auto it = subscribers.find(eventType);

        return it == subscribers.end() ? 0 : it->second.size();
    }
};

// Abstract processor interface
class EventProcessor {
public:
    virtual ~EventProcessor() = default;
    virtual void process(const Event& event) = 0;
    virtual std::string getType() const = 0;
};

class LogProcessor : public EventProcessor {
public:
    void process(const Event& event) override {
        std::cout << "[LOG] " << event.type
                  << ": " << event.data << '\n';
    }

    std::string getType() const override {
        return "log";
    }
};

class AlertProcessor : public EventProcessor {
public:
    void process(const Event& event) override {
        std::cout << "[ALERT] *** " << event.type
                  << ": " << event.data << " ***\n";
    }

    std::string getType() const override {
        return "alert";
    }
};

class ProcessorFactory {
public:
    enum class ProcessorType { LOG, ALERT };

    static std::unique_ptr<EventProcessor> create(
        ProcessorType type) {
        switch (type) {
            case ProcessorType::LOG:
                return std::make_unique<LogProcessor>();

            case ProcessorType::ALERT:
                return std::make_unique<AlertProcessor>();
        }

        throw std::invalid_argument("Unknown processor type");
    }
};

int main() {
    EventBus bus;

    // Subscriber 1: lambda
    bus.subscribe("USER_LOGIN", [](const Event& event) {
        std::cout << "Subscriber 1: User activity: "
                  << event.data << '\n';
    });

    // Subscriber 2: another lambda
    bus.subscribe("USER_LOGIN", [](const Event& event) {
        std::cout << "Subscriber 2: Login detected at "
                  << std::chrono::system_clock::to_time_t(
                         event.timestamp)
                  << '\n';
    });

    // Subscriber 3: alert processor
    auto alert = std::shared_ptr<EventProcessor>(ProcessorFactory::create(
        ProcessorFactory::ProcessorType::ALERT));

    bus.subscribe("SYSTEM_ERROR",
        [processor = std::move(alert)](const Event& event) {
            processor->process(event);
        });

    // Subscriber 4: log processor
    auto logger = std::shared_ptr<EventProcessor>(ProcessorFactory::create(
        ProcessorFactory::ProcessorType::LOG));

    bus.subscribe("USER_LOGIN",
        [processor = std::move(logger)](const Event& event) {
            processor->process(event);
        });

    std::cout << "USER_LOGIN subscribers: "
              << bus.getSubscriberCount("USER_LOGIN") << '\n';

    std::cout << "SYSTEM_ERROR subscribers: "
              << bus.getSubscriberCount("SYSTEM_ERROR") << '\n';

    bus.publish(Event("USER_LOGIN", "Alice logged in"));
    bus.publish(Event("SYSTEM_ERROR", "Database unavailable"));
    bus.publish(Event("UNKNOWN", "No subscribers"));

    return 0;
}
