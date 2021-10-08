#include "spdlog/spdlog.h"

#define S_TRACE(msg, ...) spdlog::info(msg, __VA_ARGS__);
#define S_WARN(msg, ...) spdlog::warn(msg, __VA_ARGS__);
#define S_ERR(msg, ...) spdlog::error(msg, __VA_ARGS__);
