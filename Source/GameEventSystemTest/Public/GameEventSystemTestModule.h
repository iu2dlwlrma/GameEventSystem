#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/**
 * GameEventSystemTest模块
 * 提供GameEventSystem插件的全面自动化测试
 */
class FGameEventSystemTestModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
