#include "K2Node/K2Node_RemoveListener.h"
#include "KismetCompiler.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "GameEventNodeUtils.h"

#define LOCTEXT_NAMESPACE "UK2Node_RemoveListener"

struct FK2Node_RemoveListenerPinName
{
	static const FName EventIdTypePinName;
	static const FName EventTagPinName;
	static const FName EventStringPinName;
};

const FName FK2Node_RemoveListenerPinName::EventIdTypePinName(TEXT("EventIdType"));
const FName FK2Node_RemoveListenerPinName::EventTagPinName(TEXT("EventTag"));
const FName FK2Node_RemoveListenerPinName::EventStringPinName(TEXT("EventName"));

void UK2Node_RemoveListener::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create input pins and output pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Input pin
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("GameEventNode", "Target", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("GameEventNode", "Target_Tooltip", "Target object for listening").ToString();

	// Create standard event identifier pins
	CreateEventIdentifierPins();
}

FText UK2Node_RemoveListener::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "RemoveListener_Tooltip", "Remove listener from game event system. 【{0}】"), FText::FromName(GetFName()));
}

FText UK2Node_RemoveListener::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("GameEventNode", "RemoveListener_Title", "Remove Listener");
}

FText UK2Node_RemoveListener::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "RemoveListener_Keywords", "{0} removeevent remove listener removelistener unbind unsubscribe unregister detach"), Super::GetKeywords());
}

void UK2Node_RemoveListener::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ThenPin = GetThenPin();
	UEdGraphPin* SelfPin = GetSelfPin();

	if (!ExecPin || !ThenPin || !SelfPin)
	{
		CompilerContext.MessageLog.Error(*FText::FromString(TEXT("Invalid pins in Remove Event node")).ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	const FName CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, RemoveListener);

	if (!CallFuncName.IsNone())
	{
		CallFuncNode->FunctionReference.SetExternalMember(CallFuncName, UGameEventNodeUtils::StaticClass());
		CallFuncNode->AllocateDefaultPins();

		UEdGraphPin* WorldContextObjectParam = CallFuncNode->FindPinChecked(WorldContextObjectParamName);
		UEdGraphPin* EventNameParam = CallFuncNode->FindPinChecked(EventNameParamName);

		ConnectEventNameWithTagConversion(CompilerContext, SourceGraph, EventNameParam);

		CompilerContext.MovePinLinksToIntermediate(*SelfPin, *WorldContextObjectParam);

		CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallFuncNode->GetExecPin());
		CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFuncNode->GetThenPin());
	}

	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_RemoveListener::GetSelfPin() const
{
	UEdGraphPin* Pin = FindPin(UEdGraphSchema_K2::PN_Self);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

#undef LOCTEXT_NAMESPACE
