#pragma once

#include "CoreMinimal.h"
#include "K2Node/K2Node_GameEventBase.h"
#include "K2Node_SendEvent.generated.h"

UCLASS()
class GAMEEVENTNODE_API UK2Node_SendEvent : public UK2Node_GameEventBase
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetKeywords() const override;
	virtual void PostReconstructNode() override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:
	void RefreshPinTypes();

	bool CheckParamDataPinTypeMatch(const UEdGraphPin* Pin = nullptr) const;

private:
	UEdGraphPin* CreateParamDataPin(const FName PinCategory);
	UEdGraphPin* GetSelfPin() const;
	UEdGraphPin* GetPinnedPin() const;
	UEdGraphPin* GetParamDataPin() const;
};
