#include "Logger.h"

DEFINE_LOG_CATEGORY(LogGameEventSystem);

#if WITH_GES_DEBUG_LOG
static std::atomic<bool> GES_DebugEnabled(true);

bool GetDebugLogEnabled()
{
	return GES_DebugEnabled.load();
}

void SetDebugLogEnabled(const bool bEnabled)
{
	GES_DebugEnabled.store(bEnabled);
}

static std::atomic<bool> GEN_DebugEnabled(false);

bool GetNodeDebugLogEnabled()
{
	return GEN_DebugEnabled.load();
}

void SetNodeDebugLogEnabled(const bool bEnabled)
{
	GEN_DebugEnabled.store(bEnabled);
}

#endif

#if WITH_GES_DEBUG_LOG
DEFINE_LOG_CATEGORY_STATIC(LogScopedCallTrack, All, All)

bool FScopedCallTracker::bIsOn = true;
float FScopedCallTracker::PerfMarkLv1Time = 0.1f;
float FScopedCallTracker::PerfMarkLv2Time = 0.5f;

FScopedCallTracker::FScopedCallTracker(FString InScopeName, const ELogVerbosity::Type InVerbosity)
{
	ScopeName = InScopeName;
	RecordedStartTime = FPlatformTime::Seconds();
	Verbosity = InVerbosity;

	if (bIsOn && GetDebugLogEnabled())
	{
		FMsg::Logf(__FILE__,__LINE__, LogScopedCallTrack.GetCategoryName(), Verbosity,TEXT("%s Begin"), *ScopeName);
	}
}

FScopedCallTracker::FScopedCallTracker(const ANSICHAR* InScopeName, const ELogVerbosity::Type InVerbosity) : RecordedStartTime(0),
                                                                                                             Verbosity(ELogVerbosity::Log)
{
	FScopedCallTracker(FString(InScopeName), InVerbosity);
}

FScopedCallTracker::~FScopedCallTracker()
{
	if (bIsOn && GetDebugLogEnabled())
	{
		const double TimeTaken = FPlatformTime::Seconds() - RecordedStartTime;

		const bool bConsiderAsWarningLv1 = (TimeTaken >= PerfMarkLv1Time);
		const bool bConsiderAsWarningLv2 = (TimeTaken >= PerfMarkLv2Time);

		const FString LogString = FString::Printf(TEXT("%s End (Execution Time : %f ms)"), *ScopeName, TimeTaken * 1000.0);
		if (bConsiderAsWarningLv1)
		{
			UE_LOG(LogScopedCallTrack, Display, TEXT("%s [PerfMark_Lv1]"), *LogString);
		}
		else if (bConsiderAsWarningLv2)
		{
			UE_LOG(LogScopedCallTrack, Warning, TEXT("%s [PerfMark_Lv2]"), *LogString);
		}
		else
		{
			FMsg::Logf(__FILE__,__LINE__, LogScopedCallTrack.GetCategoryName(), Verbosity,TEXT("%s"), *LogString);
		}
	}
}

#endif
