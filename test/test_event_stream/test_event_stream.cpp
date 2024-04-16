#include <memory>

#include <unity.h>

#include <helpers/string_format.h>
#include <Timer.h>
#include <event_stream/EventStreamProcesses.h>

void test_input_to_event_stream() {
  StateInput input(0);
  InputToEventStream inputToEventStream(&input);
  unsigned long timestamp = 0;
  int value = 0;
  inputToEventStream.registerSubscriber([&timestamp, &value](Event<int> e) {
    timestamp = e.timestamp;
    value = e.value;
  });
  input.write(3);
  inputToEventStream.run();
  TEST_ASSERT_EQUAL(0, timestamp);
  TEST_ASSERT_EQUAL(3, value);
  input.write(15);
  Timekeeper::tick();
  inputToEventStream.run();
  TEST_ASSERT_EQUAL(1, timestamp);
  TEST_ASSERT_EQUAL(15, value);
}

void test_event_stream_filter() {
  DumbEventStream<int> unfiltered;
  int filteredEventsCount = 0;
  int falsePositives = 0;
  EventStreamFilter<int> filteredByTimestamp(
    &unfiltered,
    [](Event<int> e) {
      return e.timestamp < 10;
    }
  );

  // First, try filtering on the whole event -- specifically the timestamp.
  filteredByTimestamp.registerSubscriber([&filteredEventsCount, &falsePositives](Event<int> e) {
      filteredEventsCount ++;
      if (e.timestamp >= 10) {
        falsePositives ++;
      }
    }
  );

  // For all filter hits, it should increase the count but not increase the false positives.
  for (uint8_t i = 0; i < 10; i ++) {
    Timekeeper::setNowSim(i);
    unfiltered.emit(0);
    TEST_ASSERT_EQUAL(i + 1, filteredEventsCount);
    TEST_ASSERT_FALSE(falsePositives > 0);
  }

  // For all filter misses, those two values should stay the same.
  for (uint8_t i = 10; i < 20; i ++) {
    Timekeeper::setNowSim(i);
    unfiltered.emit(0);
    TEST_ASSERT_EQUAL(10, filteredEventsCount);
    TEST_ASSERT_FALSE(falsePositives > 0);
  }

  // Now let's try the same, but only supplying a filter lambda for the value.
  filteredEventsCount = 0;
  EventStreamFilter<int> filteredByValue(
    &unfiltered,
    [](int v) {
      return v < 10;
    }
  );
  filteredByValue.registerSubscriber([&filteredEventsCount, &falsePositives](Event<int> e) {
      filteredEventsCount ++;
      if (e.value >= 10) {
        falsePositives ++;
      }
    }
  );
  for (uint8_t i = 0; i < 10; i ++) {
    Timekeeper::tick();
    unfiltered.emit(i);
    TEST_ASSERT_EQUAL(i + 1, filteredEventsCount);
    TEST_ASSERT_FALSE(falsePositives > 0);
  }
  for (uint8_t i = 10; i < 20; i ++) {
    Timekeeper::tick();
    unfiltered.emit(i);
    TEST_ASSERT_EQUAL(10, filteredEventsCount);
    TEST_ASSERT_FALSE(falsePositives > 0);
  }
}

void test_event_stream_translator() {
  DumbEventStream<int> events;
  EventStreamTranslator<int, int> doubler(&events, [](Event<int> e) {
    return Event{ e.timestamp * 2, e.value * 2 };
  });
  Event<int> receivedEvent{0, 0};
  doubler.registerSubscriber([&receivedEvent](Event<int> e) { receivedEvent = e; });
  for (int i = 0; i < 10; i ++) {
    Timekeeper::setNowSim(i);
    events.emit(i);
    TEST_ASSERT_EQUAL(i * 2, receivedEvent.timestamp);
    TEST_ASSERT_EQUAL(i * 2, receivedEvent.value);
  }

  EventStreamTranslator<int, int> tripler(&events, [](int v) { return v * 3; });
  int receivedValue;
  tripler.registerSubscriber([&receivedValue](Event<int> e) { receivedValue = e.value; });
  for (int i = 0; i < 10; i ++) {
    events.emit(i);
    TEST_ASSERT_EQUAL(i * 3, receivedValue);
  }
}

void test_event_stream_not_empty() {
  DumbEventStream<std::optional<int>> oddEventsHaveAValue;
  EventStreamNotEmpty<int> removeEmpties(&oddEventsHaveAValue);
  removeEmpties.registerSubscriber([](Event<int> e) { TEST_ASSERT_TRUE(e.value % 2); });
  for (int i = 0; i <= 10; i ++) {
    if (i % 2) {
      oddEventsHaveAValue.emit(i);
    } else {
      oddEventsHaveAValue.emit(std::nullopt);
    }
  }
}

void test_event_stream_debouncer() {
  Timekeeper::setNowSim(0);
  DumbEventStream<bool> noisyButton;
  EventStreamDebouncer<bool> cleanButton(&noisyButton, 10);
  int eventsCount = 0;
  bool receivedValue = false;
  cleanButton.registerSubscriber([&eventsCount, &receivedValue](Event<bool> e) {
    eventsCount ++;
    receivedValue = e.value;
  });

  Timekeeper::tick();
  noisyButton.emit(true);
  cleanButton.run();

  // No event emitted until the button settles.
  // Simulate noise for 7ms, ending in a stable value of true.
  for (int i = 1; i <= 7; i ++) {
    Timekeeper::tick();
    // Here's the noise -- true on odd, false on even.
    noisyButton.emit(i % 2);
    cleanButton.run();
    TEST_ASSERT_EQUAL(0, eventsCount);
    TEST_ASSERT_FALSE(receivedValue);
  }
  // Jump to just before the end of the debounce window; shouldn't have fired yet.
  Timekeeper::setNowSim(10);
  cleanButton.run();
  TEST_ASSERT_EQUAL(0, eventsCount);
  TEST_ASSERT_FALSE(receivedValue);

  // Now jump one more ms, to the end of the window.
  Timekeeper::tick();
  cleanButton.run();
  TEST_ASSERT_EQUAL(1, eventsCount);
  TEST_ASSERT_TRUE(receivedValue);

  // The debouncer should accept a change right away after emitting an event...
  Timekeeper::tick();
  noisyButton.emit(false);
  // ... But not actually emit an event for the change until after the period.
  Timekeeper::tick(10);
  cleanButton.run();
  TEST_ASSERT_EQUAL(2, eventsCount);
  TEST_ASSERT_FALSE(receivedValue);
}

void test_event_stream_switcher() {
  Timekeeper::setNowSim(0);
  auto alice = new DumbEventStream<const char*>();
  auto bob = new DumbEventStream<const char*>();
  auto carol = new DumbEventStream<const char*>();
  std::map<int, EventStream<const char*>*> streams = {
    { 0, alice },
    { 1, bob },
    { 2, carol }
  };
  StateInput switchy(0);
  EventStreamSwitcher<int, const char*> switcher(streams, &switchy);
  const char* message;
  switcher.registerSubscriber([&message](Event<const char*> e) {
    message = e.value;
  });
  alice->emit("Hello from Alice!");
  bob->emit("Hello from Bob!");
  carol->emit("Hello from Carol!");
  TEST_ASSERT_EQUAL_STRING("Hello from Alice!", message);
  switchy.write(1);
  alice->emit("Hello from Alice!");
  bob->emit("Hello from Bob!");
  carol->emit("Hello from Carol!");
  TEST_ASSERT_EQUAL_STRING("Hello from Bob!", message);
  switchy.write(2);
  alice->emit("Hello from Alice!");
  bob->emit("Hello from Bob!");
  carol->emit("Hello from Carol!");
  TEST_ASSERT_EQUAL_STRING("Hello from Carol!", message);
}

void test_event_stream_combiner() {
  Timekeeper::setNowSim(0);
  auto alice = new DumbEventStream<const char*>();
  auto bob = new DumbEventStream<const char*>();
  auto carol = new DumbEventStream<const char*>();
  std::vector<EventStream<const char*>*> streams = {
    alice,
    bob,
    carol
  };
  EventStreamCombiner<const char*> combiner(streams);
  const char* message;
  combiner.registerSubscriber([&message](Event<const char*> e) {
    message = e.value;
  });
  alice->emit("Hello from Alice!");
  TEST_ASSERT_EQUAL_STRING("Hello from Alice!", message);
  bob->emit("Hello from Bob!");
  TEST_ASSERT_EQUAL_STRING("Hello from Bob!", message);
  carol->emit("Hello from Carol!");
  TEST_ASSERT_EQUAL_STRING("Hello from Carol!", message);
}

void test_fancy_pushbutton() {
  TEST_FAIL_MESSAGE("write the test already");
}

int main(int argc, char **argv) {
  Timekeeper::setSource(TimekeeperSource::simTime);
  UNITY_BEGIN();
  RUN_TEST(test_input_to_event_stream);
  RUN_TEST(test_event_stream_filter);
  RUN_TEST(test_event_stream_translator);
  RUN_TEST(test_event_stream_not_empty);
  RUN_TEST(test_event_stream_debouncer);
  RUN_TEST(test_event_stream_switcher);
  RUN_TEST(test_event_stream_combiner);
  RUN_TEST(test_fancy_pushbutton);
  UNITY_END();
}