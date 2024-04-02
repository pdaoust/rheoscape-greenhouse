#ifndef RHEOSCAPE_OUTPUT_FACTORIES_H
#define RHEOSCAPE_OUTPUT_FACTORIES_H

#include <input/Input.h>
#include <input/TimeProcesses.h>
#include <input/TranslatingProcesses.h>
#include <output/DigitalPinOutput.h>
#include <output/MotorDriver.h>
#include <output/Output.h>

// Wrap the input in a throttling process, to be gentle on hardware.
// Mechanical relays have coils that can't take more than e.g. a 2 second cycling time,
// zero-crossing solid-state relays can create weird dirty power problems
// if their cycle time doesn't line up perfectly with the mains cycle,
// and some loads (e.g., inductive loads) can't take fast cycling on an AC supply either.
Output makeRelay(
  uint8_t pin,
  bool onState,
  unsigned long minCycleTime,
  Input<bool> input,
  Runner runner
) {
  Input<bool> throttle = ThrottlingProcess<bool>(
    input,
    minCycleTime
  );
  return DigitalPinOutput(
    pin,
    onState,
    throttle,
    runner
  );
}

// Wrap a motor controller in a throttling process that also converts booleans to floats.
// The excursion time should allow enough time for the cover to fully open or close.
// TODO: Maybe limit switches could be used for a more advanced cover in the future.
Output makeCover(
  uint8_t forwardPin,
  uint8_t backwardPin,
  uint8_t pwmPin,
  bool controlPinActiveState,
  unsigned long excursionTime,
  Input<bool> input,
  Runner runner
) {
  Input<bool> throttle = ThrottlingProcess<bool>(
    input,
    excursionTime
  );
  Input<float> translator = TranslatingProcess<bool, float>(
    throttle,
    [](bool value) {
      return (float)(value ? 1.0 : -1.0);
    }
  );
  return MotorDriver(
    forwardPin,
    backwardPin,
    pwmPin,
    controlPinActiveState,
    translator,
    runner
  );
}

#endif