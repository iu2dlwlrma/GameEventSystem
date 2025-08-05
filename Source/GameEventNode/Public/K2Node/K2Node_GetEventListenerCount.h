#pragma once

#include "CoreMinimal.h"
#include "K2Node_GameEventBase.h"
#include "K2Node_GetEventListenerCount.generated.h"

UCLASS()
class GAMEEVENTNODE_API UK2Node_GetEventListenerCount : public UK2Node_GameEventBase
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;

	virtual bool IsNodePure() const override
	{
		return true;
	}

	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetKeywords() const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

private:
	UEdGraphPin* GetSelfPin() const;
	UEdGraphPin* GetReturnValuePin() const;
};
