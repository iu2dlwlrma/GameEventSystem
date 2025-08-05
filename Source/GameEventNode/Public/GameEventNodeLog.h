#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameEventNode, VeryVerbose, All);

#if !UE_BUILD_SHIPPING
#define SHOW_DEBUG_LOG 1
#else
#define SHOW_DEBUG_LOG 0
#endif

#if SHOW_DEBUG_LOG
#define UE_LOG_GAS(Verbosity, Format, ...) \
    UE_LOG(LogGameEventNode, Verbosity, TEXT("[GameEventNode] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_INFO(Format, ...) \
    UE_LOG(LogGameEventNode, Log, TEXT("[GameEventNode] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_VERBOSE(Format, ...) \
    UE_LOG(LogGameEventNode, VeryVerbose, TEXT("[GameEventNode] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_EDITOR(Format, ...) \
    UE_LOG(LogGameEventNode, VeryVerbose, TEXT("[Editor] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_WARNING(Format, ...) \
    UE_LOG(LogGameEventNode, Warning, TEXT("[GameEventNode][WARNING] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_ERROR(Format, ...) \
    UE_LOG(LogGameEventNode, Error, TEXT("[GameEventNode][ERROR] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_COMPILE(Format, ...) \
    UE_LOG(LogGameEventNode, VeryVerbose, TEXT("[Compile] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_PIN(Format, ...) \
    UE_LOG(LogGameEventNode, VeryVerbose, TEXT("[Pin] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_UI(Format, ...) \
    UE_LOG(LogGameEventNode, VeryVerbose, TEXT("[UI] ") Format, ##__VA_ARGS__)

#define UE_LOG_GAS_DEBUG(Format, ...) \
    UE_LOG(LogGameEventNode, VeryVerbose, TEXT("[DEBUG] ") Format, ##__VA_ARGS__)

#else
#define UE_LOG_GAS(Format, ...)
#define UE_LOG_GAS_INFO(Format, ...)
#define UE_LOG_GAS_VERBOSE(Format, ...)
#define UE_LOG_GAS_EDITOR(Format, ...)
#define UE_LOG_GAS_WARNING(Format, ...)
#define UE_LOG_GAS_ERROR(Format, ...)
#define UE_LOG_GAS_COMPILE(Format, ...)
#define UE_LOG_GAS_PIN(Format, ...)
#define UE_LOG_GAS_UI(Format, ...)
#define UE_LOG_GAS_DEBUG(Format, ...)
#endif

#define UE_LOG_GAS_FUNC_ENTER() \
UE_LOG_GAS_DEBUG(TEXT("Entering function: %s"), ANSI_TO_TCHAR(__FUNCTION__))

#define UE_LOG_GAS_FUNC_EXIT() \
UE_LOG_GAS_DEBUG(TEXT("Exiting function: %s"), ANSI_TO_TCHAR(__FUNCTION__))
