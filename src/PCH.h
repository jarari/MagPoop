#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

#pragma warning(push)
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <REX/REX.h>
#pragma warning(pop)

#include <Windows.h>
#undef ERROR
#undef max
#undef min
#include <DirectXPackedVector.h>
#include <cmath>
#include <cstdarg>
#include <functional>
#include <unordered_map>

using namespace std::literals;
