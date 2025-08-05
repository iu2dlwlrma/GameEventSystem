#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGameEventSystemModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	FDelegateHandle BeginPieDelegate;

	void RegisterSettings();
	void UnregisterSettings();
#endif
};
