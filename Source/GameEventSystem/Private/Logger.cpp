#include "Logger.h"
DEFINE_LOG_CATEGORY(LogGameEventSystem);

#if WITH_DEBUG_LOG

DEFINE_LOG_CATEGORY_STATIC(LogScopedCallTrack, Log, All)

bool FScopedCallTracker::bIsOn = true;
float FScopedCallTracker::PerfMarkLv1Time = 0.1f;
float FScopedCallTracker::PerfMarkLv2Time = 0.5f;

FScopedCallTracker::FScopedCallTracker(FString InScopeName)
{
	ScopeName = InScopeName;
	RecordedStartTime = FPlatformTime::Seconds();

	if (bIsOn)
	{
		UE_LOG(LogScopedCallTrack, Log, TEXT("%s Begin"), *ScopeName);
	}
}

FScopedCallTracker::FScopedCallTracker(const ANSICHAR* InScopeName) : RecordedStartTime(0)
{
	FScopedCallTracker(FString(InScopeName));
}

FScopedCallTracker::~FScopedCallTracker()
{
	if (bIsOn)
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
			UE_LOG(LogScopedCallTrack, Log, TEXT("%s"), *LogString);
		}
	}
}

#endif
