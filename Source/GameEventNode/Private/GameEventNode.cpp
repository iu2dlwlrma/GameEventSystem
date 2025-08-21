#include "Modules/ModuleManager.h"
#include "GameEventNodeLog.h"
#include "GraphEditor/GraphNodeFactory.h"

DEFINE_LOG_CATEGORY(LogGameEventNode);

class FGameEventNodeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG_GAS_INFO(TEXT("GameEventNode module is starting..."));

		GraphNodeFactory = MakeShareable(new FGameEventGraphNodeFactory());

		if (GraphNodeFactory.IsValid())
		{
			FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeFactory);
			UE_LOG_GAS_INFO(TEXT("GameEventNodeGraphNodeFactory registered successfully"));
		}
		else
		{
			UE_LOG_GAS_ERROR(TEXT("Failed to create GameEventNodeGraphNodeFactory"));
		}

		UE_LOG_GAS_INFO(TEXT("GameEventNode module startup completed"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG_GAS_INFO(TEXT("GameEventNode module is shutting down..."));

		if (GraphNodeFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeFactory);
			GraphNodeFactory.Reset();
			UE_LOG_GAS_INFO(TEXT("GameEventNodeGraphNodeFactory unregistered successfully"));
		}
		else
		{
			UE_LOG_GAS_WARNING(TEXT("GameEventNodeGraphNodeFactory is already null, skipping unregister operation"));
		}

		UE_LOG_GAS_INFO(TEXT("GameEventNode module shutdown completed"));
	}

private:
	TSharedPtr<FGameEventGraphNodeFactory> GraphNodeFactory;
};

IMPLEMENT_MODULE(FGameEventNodeModule, GameEventNode);
