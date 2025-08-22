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
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void DestroyNode() override;
	
protected:
	void RefreshPinTypes();
	
private:
	void SafeRemovePin(UEdGraphPin* PinToRemove);
	UEdGraphPin* CreateParamDataPinAtIndex(const int32 Index, const FName PinCategory);
	TArray<UEdGraphPin*> GetAllParamDataPins() const;
	UEdGraphPin* GetParamDataPinByIndex(const int32 Index) const;
	bool CheckParamDataPinsTypeMatch() const;
#pragma region "GetPin"
	UEdGraphPin* GetSelfPin() const;
	UEdGraphPin* GetPinnedPin() const;
	UEdGraphPin* GetParamDataPin() const;
#pragma endregion

	uint8 bIsBindSuccess:1;
};
