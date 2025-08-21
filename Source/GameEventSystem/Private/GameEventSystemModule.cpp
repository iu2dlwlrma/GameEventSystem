#include "GameEventSystemModule.h"
#include "GameEventManager.h"
#include "GameEventSystemSettings.h"
#include "Logger.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#define LOCTEXT_NAMESPACE "FGameEventSystemModule"

void FGameEventSystemModule::StartupModule()
{
#if WITH_GES_DEBUG_LOG
	if (const UGameEventSystemSettings* Settings = UGameEventSystemSettings::Get())
	{
		SetDebugLogEnabled(Settings->bEnableDebug);
		SetNodeDebugLogEnabled(Settings->bEnableNodeDebug);
	}
#endif

	GES_LOG_DISPLAY(TEXT("GameEventSystem module is starting..."));

	const TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	if (EventManager.IsValid())
	{
		GES_LOG_DISPLAY(TEXT("GameEventManager instance initialized successfully"));
	}
	else
	{
		GES_LOG_ERROR(TEXT("Failed to initialize GameEventManager instance"));
	}

#if WITH_EDITOR
	BeginPieDelegate = FEditorDelegates::BeginPIE.AddLambda([](bool bIsSimulating)
	{
		GES_LOG_VERY_VERBOSE(TEXT("PIE startup detected, cleaning up..."));

		const TSharedPtr<FGameEventManager> Manager = FGameEventManager::Get();
		if (Manager.IsValid())
		{
			Manager->Clear();
			GES_LOG_VERY_VERBOSE(TEXT("Cleanup completed"));
		}
		else
		{
			GES_LOG_WARNING(TEXT("Unable to get GameEventManager instance, skipping cleanup"));
		}
	});

	GES_LOG_VERY_VERBOSE(TEXT("PIE begin delegate registered"));
#endif

	GES_LOG_VERY_VERBOSE(TEXT("GameEventSystem module startup completed"));
}

void FGameEventSystemModule::ShutdownModule()
{
	GES_LOG_VERY_VERBOSE(TEXT("GameEventSystem module is shutting down..."));

	const TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	if (EventManager.IsValid())
	{
		EventManager->Clear();
		GES_LOG_VERY_VERBOSE(TEXT("Cleanup completed"));
	}

#if WITH_EDITOR
	if (BeginPieDelegate.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(BeginPieDelegate);
		GES_LOG_VERY_VERBOSE(TEXT("PIE begin delegate removed"));
	}
#endif

	GES_LOG_VERY_VERBOSE(TEXT("GameEventSystem module shutdown completed"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameEventSystemModule, GameEventSystem);
