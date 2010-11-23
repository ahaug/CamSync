#include <assert.h>
#include <stdio.h>
#include <tr1/memory>

/* This file documents how base and derived frames and sensors work by
 * providing a small model of it. */

class _BaseFrame {
public:
    _BaseFrame() {foo = 1;}
    int foo;
};


class BaseFrame {
public:
    BaseFrame(_BaseFrame *f) : ptr(f) {}
    // Access to the base class functionality
    int foo() {return get()->foo;}
protected:
    std::tr1::shared_ptr<_BaseFrame> ptr;
private:
    _BaseFrame *get() {return ptr.get();}
};

class BaseSensor {
public:
    BaseFrame getFrame() {return getBaseFrame();}
protected:
    virtual BaseFrame getBaseFrame() = 0;
};

class _DerivedFrame : public _BaseFrame {
public:
    _DerivedFrame() {foo=2; panda=17;}

    // something completely new
    unsigned panda;
};

class DerivedFrame : public BaseFrame {
public:
    DerivedFrame(_DerivedFrame *f) : BaseFrame(f) {}
    // Access to the derived class functionality
    unsigned panda() {return get()->panda;}
private:
    _DerivedFrame *get() {return (_DerivedFrame *)ptr.get();}
};

class DerivedSensor : public BaseSensor {
public:
    DerivedFrame getFrame() {
        return DerivedFrame(new _DerivedFrame());
    }
protected:
    virtual BaseFrame getBaseFrame() {
        return BaseFrame(getFrame());
    }
};


int main(int argc, char **argv) {
    printf("%d %d\n", sizeof(BaseFrame), sizeof(DerivedFrame));

    DerivedSensor ds;
    DerivedFrame df = ds.getFrame();
    printf("%d\n", df.foo());

    printf("%d\n", df.panda());

    BaseFrame bf = ds.getFrame();
    printf("%d\n", bf.foo());

    BaseSensor &bs = ds;
    bf = bs.getFrame();
    printf("%d\n", bf.foo());
    
    return 0;
};
