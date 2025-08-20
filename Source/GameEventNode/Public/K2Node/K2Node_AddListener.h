#pragma once

#include "CoreMinimal.h"
#include "K2Node/K2Node_GameEventBase.h"
#include "Templates/SharedPointer.h"
#include "K2Node_AddListener.generated.h"

enum class EEventBindType : uint8;
enum class EEventIdType : uint8;
struct FEventTypeInfo;
class SGraphPin;
class UEdGraphPin;
class UK2Node_CreateDelegate;
class UK2Node_CallFunction;
class UK2Node_CustomEvent;

UCLASS(BlueprintType)
class GAMEEVENTNODE_API UK2Node_AddListener : public UK2Node_GameEventBase
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetKeywords() const override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void PostReconstructNode() override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

protected:
	virtual bool CheckUpdatePinCondition(const UEdGraphPin* Pin) const override;
	virtual void UpdatePinVisibility() override;

	void UpdateEventSignature() const;
	bool AnalyzeFunctionAndRegisterEventType() const;
	bool CheckFunctionParamCount() const;
	void UpdateCustomEventSignatureFromDataType() const;
	void HandleDelegateExpansion(const UK2Node_CallFunction* CallFuncNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);
	void CreateDataConversionNodes(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const UK2Node_CustomEvent* WrapperEventNode);

private:
	void AddDataTypePinAtIndex(const int32 Index = -1);
	void RemoveDataTypePinAtIndex(const int32 Index = -1);
	TArray<UEdGraphPin*> GetAllDataTypePins() const;
	UEdGraphPin* GetDataTypePinByIndex(const int32 Index) const;
	int32 GetDataTypePinCount() const;

#pragma region "GetPin"
	UEdGraphPin* GetDataTypePin() const;
	UEdGraphPin* GetSelfPin() const;
	UEdGraphPin* GetFunctionNamePin() const;
	UEdGraphPin* GetBindTypePin() const;
	UEdGraphPin* GetDelegatePin() const;
#pragma endregion

	friend class SGraphNodeAddListener;
};
