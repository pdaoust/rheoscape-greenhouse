#ifndef RHEOSCAPE_TWILIO_MESSAGE_OUTPUT_H
#define RHEOSCAPE_TWILIO_MESSAGE_OUTPUT_H
#ifdef PLATFORM_ARDUINO

#include <functional>

#include <twilio.hpp>
#include <input/Input.h>
#include <event_stream/EventStream.h>

struct TwilioConfig {
  std::string accountId;
  std::string authToken;
  std::string sender;
  std::string recipient;
};

// Send a message via Twilio.
// TODO: make the config an input?
// WARNING: There's no error checking whatsoever in here.
// And in fact there's no way in the entire framework to catch errors.
// TODO: make a logger.
class TwilioMessageNotifier {
  private:
    Input<TwilioConfig>* _configInput;
    TwilioConfig _lastSeenConfig;
    Twilio _client;
  
  public:
    TwilioMessageNotifier(EventStream<std::string>* eventStream, Input<TwilioConfig>* configInput)
    :
      _configInput(configInput),
      _lastSeenConfig(configInput->read()),
      _client(_lastSeenConfig.accountId.c_str(), _lastSeenConfig.authToken.c_str())
    {
      eventStream->registerSubscriber([this](Event<std::string> e) { this->sendMessage(e.value); });
    }

    void sendMessage(std::string message) {
      TwilioConfig config = _configInput->read();
      if (config.accountId != _lastSeenConfig.accountId || config.authToken != _lastSeenConfig.authToken) {
        _client = Twilio(_lastSeenConfig.accountId.c_str(), _lastSeenConfig.authToken.c_str());
      }
      _lastSeenConfig = config;

      String response;
      _client.send_message(
        _lastSeenConfig.recipient.c_str(),
        _lastSeenConfig.sender.c_str(),
        message.c_str(),
        response
      );
    }
};

#endif
#endif