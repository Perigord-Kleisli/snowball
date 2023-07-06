
#ifndef _SNOWBALL_RUNTIME_H_
#define _SNOWBALL_RUNTIME_H_

namespace snowball {
    void initialize_exceptions();
}

__attribute__((always_inline)) void initialize_snowball() __asm__("sn.runtime.initialize");

#endif // _SNOWBALL_RUNTIME_H_