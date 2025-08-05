#pragma once

#include "CoreMinimal.h"
#include "K2Node/K2Node_GameEventBase.h"
#include "K2Node_RemoveListener.generated.h"

enum class EEventBindType : uint8;
enum class EEventIdType : uint8;

UCLASS(BlueprintType)
class GAMEEVENTNODE_API UK2Node_RemoveListener : public UK2Node_GameEventBase
{
	GENERATED_BODY()

public:
	//~ Begin UK2Node Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetKeywords() const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	//~ End UK2Node Interface

private:
	UEdGraphPin* GetSelfPin() const;
};
