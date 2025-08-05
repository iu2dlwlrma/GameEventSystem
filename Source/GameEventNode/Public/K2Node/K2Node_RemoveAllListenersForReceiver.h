#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_RemoveAllListenersForReceiver.generated.h"

UCLASS()
class GAMEEVENTNODE_API UK2Node_RemoveAllListenersForReceiver : public UK2Node
{
	GENERATED_BODY()

public:
	//~ Begin UK2Node Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetKeywords() const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	//~ End UK2Node Interface

private:
	UEdGraphPin* GetReceiverPin() const;
};
