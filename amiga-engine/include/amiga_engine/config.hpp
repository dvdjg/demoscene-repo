#pragma once

/**
 * Build profiles: host unit tests vs Amiga target (see Makefile).
 */
#if defined(AMIGA_ENGINE_HOST_BUILD)
#define AMIGA_ENGINE_HOST 1
#else
#define AMIGA_ENGINE_HOST 0
#endif

namespace amiga::engine {

inline constexpr unsigned k_engine_version_major = 0;
inline constexpr unsigned k_engine_version_minor = 1;
inline constexpr unsigned k_engine_version_patch = 0;

} // namespace amiga::engine
