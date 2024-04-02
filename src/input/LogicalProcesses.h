#ifndef RHEOSCAPE_LOGICAL_PROCESSES_H
#define RHEOSCAPE_LOGICAL_PROCESSES_H

#include <input/Input.h>

// Boolean processes.
// These all cast empty values to false for the purpose of comparison.

class AndProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInputA;
    Input<bool> _wrappedInputB;
  
  public:
    AndProcess(Input<bool> wrappedInputA, Input<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    bool read() {
      return _wrappedInputA.read() && _wrappedInputB.read();
    }
};

class OrProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInputA;
    Input<bool> _wrappedInputB;
  
  public:
    OrProcess(Input<bool> wrappedInputA, Input<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    bool read() {
      return _wrappedInputA.read() || _wrappedInputB.read();
    }
};

class XorProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInputA;
    Input<bool> _wrappedInputB;
  
  public:
    XorProcess(Input<bool> wrappedInputA, Input<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    bool read() {
      // Gotta cast to boolean, because otherwise an empty value and a false will be equal.
      return !_wrappedInputA.read() != !_wrappedInputB.read();
    }
};

class NotProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInput;
  
  public:
    NotProcess(Input<bool> wrappedInput) : _wrappedInput(wrappedInput) { }

    bool read() {
      return !_wrappedInput.read();
    }
};

#endif