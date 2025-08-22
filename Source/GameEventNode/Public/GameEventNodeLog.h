#pragma once
#include "Logger.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameEventNode, VeryVerbose, All);

#if !UE_BUILD_SHIPPING
#define WITH_GEN_DEBUG_LOG 1
#else
#define WITH_GEN_DEBUG_LOG 0
#endif

#if WITH_GES_DEBUG_LOG && WITH_GEN_DEBUG_LOG
#define UE_LOG_GAS_INFO(Format, ...) \
if (GetNodeDebugLogEnabled()) \
{ \
GAME_EVENT_SYSTEM_LOG(LogGameEventNode, Log, Format, ##__VA_ARGS__) \
}

#define UE_LOG_GAS_EDITOR(Format, ...) \
if (GetNodeDebugLogEnabled()) \
{ \
GAME_EVENT_SYSTEM_LOG(LogGameEventNode, VeryVerbose, Format, ##__VA_ARGS__) \
}

#define UE_LOG_GAS_WARNING(Format, ...) \
if (GetNodeDebugLogEnabled()) \
{ \
GAME_EVENT_SYSTEM_LOG(LogGameEventNode, Warning, Format, ##__VA_ARGS__) \
}

#define UE_LOG_GAS_ERROR(Format, ...) \
if (GetNodeDebugLogEnabled()) \
{ \
GAME_EVENT_SYSTEM_LOG(LogGameEventNode, Error, Format, ##__VA_ARGS__) \
}

#else
#define UE_LOG_GAS_INFO(Format, ...)
#define UE_LOG_GAS_EDITOR(Format, ...)
#define UE_LOG_GAS_WARNING(Format, ...)
#define UE_LOG_GAS_ERROR(Format, ...)
#endif
