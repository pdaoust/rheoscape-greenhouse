#ifndef RHEOSCAPE_TWILIO_MESSAGE_OUTPUT_H
#define RHEOSCAPE_TWILIO_MESSAGE_OUTPUT_H

#include <input/Input.h>
#include <output/Output.h>
#include <twilio.hpp>

struct TwilioConfig {
  std::string accountId;
  std::string authToken;
  std::string sender;
  std::string recipient;
};

// Send a message via Twilio every n milliseconds,
// unless the message is blank.
// TODO: make the config an input?
// WARNING: There's no error checking whatsoever in here.
// And in fact there's no way in the entire framework to catch errors.
// TODO: make a logger.
class TwilioMessageOutput : public Output {
  private:
    std::optional<std::string> _lastMessageSent;
    Input<std::string> _input;
    Throttle _throttle;
    TwilioConfig _config;
    Twilio _client;

    void _sendMessage(std::optional<std::string> message) {
      if (message == _lastMessageSent) {
        return;
      }

      if (!message.has_value()) {
        _lastMessageSent = message;
        return;
      }

      String response;
      _client.send_message(
        _config.recipient.c_str(),
        _config.sender.c_str(),
        message.value().c_str(),
        response
      );
      _lastMessageSent = message;
    }
  
  public:
    TwilioMessageOutput(Input<std::string> input, unsigned long sendEvery, TwilioConfig config, Runner runner)
    :
      _input(input),
      _throttle(Throttle(
        sendEvery,
        [this]() {
          _sendMessage(_input.read());
        }
      )),
      _config(config),
      _client(config.accountId.c_str(), config.authToken.c_str()),
      Output(runner)
    { }

    void run() {
      _throttle.tryRun();
    }
};

#endif