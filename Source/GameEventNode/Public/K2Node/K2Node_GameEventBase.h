#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_GameEventBase.generated.h"

UCLASS(Abstract)
class GAMEEVENTNODE_API UK2Node_GameEventBase : public UK2Node
{
	GENERATED_BODY()

public:
	UK2Node_GameEventBase();

	// UEdGraphNode interface
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetKeywords() const override;
	virtual void PostReconstructNode() override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

protected:
	struct FGameEventBasePinNames
	{
		static const FName EventIdTypePinName;
		static const FName EventTagPinName;
		static const FName EventStringPinName;
	};

	virtual void UpdatePinVisibility();

	virtual FString GetCurrentEventName() const;

	virtual void CreateEventIdentifierPins();

	virtual UEdGraphPin* GetEventIdTypePin() const;
	virtual UEdGraphPin* GetEventTagPin() const;
	virtual UEdGraphPin* GetEventStringPin() const;

	virtual bool IsEventIdentifierPin(const UEdGraphPin* Pin) const;
};
