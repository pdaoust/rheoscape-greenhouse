#ifndef RHEOSCAPE_OUTPUT_H
#define RHEOSCAPE_OUTPUT_H

#include <Runnable.h>

// An output is not a special type of thing with its own class;
// it's just a pattern of taking an input and implementing a run() function,
// then reacting to the input when appropriate within run().
// In other words, it's just a runnable.

// Zero-crossing SSRs, while fast, can create weird bias voltages if you switch them too fast.
#define SOLID_STATE_RELAY_CYCLE_TIME 100
// Don't thrash the poor mechanical relays.
#define MECHANICAL_RELAY_CYCLE_TIME 2000
// Be even gentler with things that have inductive loads in them.
#define INDUCTIVE_LOAD_CYCLE_TIME 1000 * 60

class Output : public Runnable { };

#endif