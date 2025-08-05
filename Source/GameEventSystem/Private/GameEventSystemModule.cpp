#include "GameEventSystemModule.h"
#include "GameEventManager.h"
#include "GameEventSystemSettings.h"
#include "Logger.h"

#if WITH_EDITOR
#include "Editor.h"
#include "ISettingsModule.h"
#endif

#define LOCTEXT_NAMESPACE "FGameEventSystemModule"

void FGameEventSystemModule::StartupModule()
{
	FLogger::Get().Log(TEXT("GameEventSystem module is starting..."));

	const TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	if (EventManager.IsValid())
	{
		FLogger::Get().Log(TEXT("GameEventManager instance initialized successfully"));
	}
	else
	{
		FLogger::Get().LogError(TEXT("Failed to initialize GameEventManager instance"));
	}

#if WITH_EDITOR
	RegisterSettings();

	BeginPieDelegate = FEditorDelegates::BeginPIE.AddLambda([](bool bIsSimulating)
	{
		FLogger::Get().LogVeryVerbose(TEXT("PIE startup detected, cleaning up..."));

		const TSharedPtr<FGameEventManager> Manager = FGameEventManager::Get();
		if (Manager.IsValid())
		{
			Manager->Clear();
			FLogger::Get().LogVeryVerbose(TEXT("Cleanup completed"));
		}
		else
		{
			FLogger::Get().LogWarning(TEXT("Unable to get GameEventManager instance, skipping cleanup"));
		}
	});

	FLogger::Get().LogVeryVerbose(TEXT("PIE begin delegate registered"));
#endif

	FLogger::Get().LogVeryVerbose(TEXT("GameEventSystem module startup completed"));
}

void FGameEventSystemModule::ShutdownModule()
{
	FLogger::Get().LogVeryVerbose(TEXT("GameEventSystem module is shutting down..."));

	const TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	if (EventManager.IsValid())
	{
		EventManager->Clear();
		FLogger::Get().LogVeryVerbose(TEXT("Cleanup completed"));
	}

#if WITH_EDITOR
	UnregisterSettings();

	if (BeginPieDelegate.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(BeginPieDelegate);
		FLogger::Get().LogVeryVerbose(TEXT("PIE begin delegate removed"));
	}
#endif

	FLogger::Get().LogVeryVerbose(TEXT("GameEventSystem module shutdown completed"));
}

#if WITH_EDITOR
void FGameEventSystemModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project",
		                                 "Plugins",
		                                 "GameEventSystem",
		                                 LOCTEXT("RuntimeSettingsName", "Game Event System"),
		                                 LOCTEXT("RuntimeSettingsDescription", "Configure Game Event System plugin settings"),
		                                 GetMutableDefault<UGameEventSystemSettings>()
		                                );

		FLogger::Get().LogVeryVerbose(TEXT("GameEventSystem settings page registered"));
	}
	else
	{
		FLogger::Get().LogWarning(TEXT("Failed to register GameEventSystem settings: Settings module not available"));
	}
}

void FGameEventSystemModule::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "GameEventSystem");

		FLogger::Get().LogVeryVerbose(TEXT("GameEventSystem settings page unregistered"));
	}
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameEventSystemModule, GameEventSystem);
