#include "test.hh"

int TestAdapter::agn_function() {
    return test;
}

/* Include platform-specific implementation */
#include PLATFORM_C(test)
