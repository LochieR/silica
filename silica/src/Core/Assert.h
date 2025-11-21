#pragma once

#include "Log.h"

#ifdef SIL_PLATFORM_WINDOWS
#define SIL_DEBUGBREAK() __debugbreak()
#else
#include <signal.h>
#define SIL_DEBUGBREAK() raise(SIGTRAP)
#endif

#ifdef SIL_DEBUG
#define SIL_ASSERT(x, msg, ...) { if (!(x)) { SIL_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); SIL_DEBUGBREAK(); } }

#define SIL_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { SIL_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); SIL_DEBUGBREAK(); } }
#define SIL_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { SIL_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); SIL_DEBUGBREAK(); } }
#else
#define SIL_ASSERT(...)

#define SIL_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { SIL_WARN(msg, ##__VA_ARGS__); } }
#define SIL_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { SIL_ERROR(msg, ##__VA_ARGS__); } }
#endif
