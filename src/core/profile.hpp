#pragma once
#include "../third_party/tracy/public/tracy/Tracy.hpp"

#ifndef DW_PROFILE
#define PROFILE_SCOPE
#define PROFILE_SCOPE_NAME(NAME)
#else
#define PROFILE_SCOPE ZoneScoped
#define PROFILE_SCOPE_NAME(NAME) ZoneScopedN(NAME)
#endif
