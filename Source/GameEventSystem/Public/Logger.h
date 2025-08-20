#pragma once
#include "CoreMinimal.h"
#include "Misc/StringBuilder.h"

GAMEEVENTSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogGameEventSystem, VeryVerbose, All);

#if UE_BUILD_SHIPPING
#define WITH_GES_DEBUG_LOG 0
#else
#define WITH_GES_DEBUG_LOG 1
#endif

#if WITH_GES_DEBUG_LOG
struct GAMEEVENTSYSTEM_API FScopedCallTracker
{
	explicit FScopedCallTracker(FString InScopeName, const ELogVerbosity::Type InVerbosity = ELogVerbosity::Log);
	explicit FScopedCallTracker(const ANSICHAR* InScopeName, const ELogVerbosity::Type InVerbosity = ELogVerbosity::Log);
	~FScopedCallTracker();

	static bool bIsOn;

	static float PerfMarkLv1Time;
	static float PerfMarkLv2Time;

private:
	double RecordedStartTime;
	FString ScopeName;
	ELogVerbosity::Type Verbosity;
};

#define GAME_SCOPED_TRACK_LOG(ScopeName) \
	FScopedCallTracker _ScopedCallTrackerDummy(ScopeName)

#define GAME_SCOPED_TRACK_LOG_AUTO() \
	FScopedCallTracker _ScopedCallTrackerDummy(ANSI_TO_TCHAR(__FUNCTION__))

#define GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME() \
	FScopedCallTracker _ScopedCallTrackerDummy(FString::Printf(TEXT("[%s][%s] %hs"), *GetBlueprint()->GetName(), *GetNameSafe(this), __FUNCTION__), ELogVerbosity::VeryVerbose)
#else
#define GAME_SCOPED_TRACK_LOG(ScopeName)
#define GAME_SCOPED_TRACK_LOG_AUTO(ScopeName)
#define GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME(ScopeName)
#endif

#if WITH_GES_DEBUG_LOG
#define GAME_EVENT_SYSTEM_LOG(LogCategory, Verbosity, Format, ...) \
UE_LOG(LogCategory, Verbosity, TEXT("[%s] ") Format, ANSI_TO_TCHAR(__FUNCTION__), ##__VA_ARGS__)

#define GES_LOG(Format, ...) \
GAME_EVENT_SYSTEM_LOG(LogGameEventSystem, Log, Format, ##__VA_ARGS__)

#define GES_LOG_DISPLAY(Format, ...) \
GAME_EVENT_SYSTEM_LOG(LogGameEventSystem, Display, Format, ##__VA_ARGS__)

#define GES_LOG_WARNING(Format, ...) \
GAME_EVENT_SYSTEM_LOG(LogGameEventSystem, Warning, Format, ##__VA_ARGS__)

#define GES_LOG_ERROR(Format, ...) \
GAME_EVENT_SYSTEM_LOG(LogGameEventSystem, Error, Format, ##__VA_ARGS__)

#define GES_LOG_VERY_VERBOSE(Format, ...) \
GAME_EVENT_SYSTEM_LOG(LogGameEventSystem, VeryVerbose, Format, ##__VA_ARGS__)

#else
#define GAME_EVENT_SYSTEM_LOG(LogCategory, Verbosity, Format, ...) 
#define GES_LOG(Format, ...) 
#define GES_LOG_DISPLAY(Format, ...) 
#define GES_LOG_WARNING(Format, ...) 
#define GES_LOG_ERROR(Format, ...) 
#endif