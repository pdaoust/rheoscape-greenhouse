#ifndef RHEOSCAPE_LOGICAL_PROCESSES_H
#define RHEOSCAPE_LOGICAL_PROCESSES_H

#include <input/Input.h>

// Boolean processes.
// These all cast empty values to false for the purpose of comparison.

class AndProcess : public BasicInput<bool> {
  private:
    BasicInput<bool> _wrappedInputA;
    BasicInput<bool> _wrappedInputB;
  
  public:
    AndProcess(BasicInput<bool> wrappedInputA, BasicInput<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    bool read() {
      return _wrappedInputA.read() && _wrappedInputB.read();
    }
};

class OrProcess : public BasicInput<bool> {
  private:
    BasicInput<bool> _wrappedInputA;
    BasicInput<bool> _wrappedInputB;
  
  public:
    OrProcess(BasicInput<bool> wrappedInputA, BasicInput<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    bool read() {
      return _wrappedInputA.read() || _wrappedInputB.read();
    }
};

class XorProcess : public BasicInput<bool> {
  private:
    BasicInput<bool> _wrappedInputA;
    BasicInput<bool> _wrappedInputB;
  
  public:
    XorProcess(BasicInput<bool> wrappedInputA, BasicInput<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    bool read() {
      // Gotta cast to boolean, because otherwise an empty value and a false will be equal.
      return !_wrappedInputA.read() != !_wrappedInputB.read();
    }
};

class NotProcess : public BasicInput<bool> {
  private:
    BasicInput<bool> _wrappedInput;
  
  public:
    NotProcess(BasicInput<bool> wrappedInput) : _wrappedInput(wrappedInput) { }

    bool read() {
      return !_wrappedInput.read();
    }
};

#endif