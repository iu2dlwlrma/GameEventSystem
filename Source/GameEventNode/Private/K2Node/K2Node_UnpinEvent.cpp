#include "K2Node/K2Node_UnpinEvent.h"
#include "GameEventNodeUtils.h"
#include "KismetCompiler.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"

#define LOCTEXT_NAMESPACE "UK2Node_UnpinEvent"

struct FK2Node_UnpinEventPinName
{
	static const FName EventIdTypePinName;
	static const FName EventTagPinName;
	static const FName EventStringPinName;
};

const FName FK2Node_UnpinEventPinName::EventIdTypePinName(TEXT("EventIdType"));
const FName FK2Node_UnpinEventPinName::EventTagPinName(TEXT("EventTag"));
const FName FK2Node_UnpinEventPinName::EventStringPinName(TEXT("EventName"));

void UK2Node_UnpinEvent::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create input pins and output pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Self pin
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("GameEventNode", "Target", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("GameEventNode", "Target_Tooltip", "Target object for listening").ToString();

	// Create standard event identifier pins
	CreateEventIdentifierPins();
}

FText UK2Node_UnpinEvent::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "UnpinEvent_Tooltip", "Unpin the specified event and clear pinned data. [{0}]"), FText::FromName(GetFName()));
}

FText UK2Node_UnpinEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("GameEventNode", "UnpinEvent_Title", "Unpin Event");
}

FText UK2Node_UnpinEvent::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "UnpinEvent_Keywords", "{0} unpin event unpinevent clear reset release unlock"), Super::GetKeywords());
}

void UK2Node_UnpinEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));

	const UFunction* UnpinEventFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, UnpinEvent));

	if (!UnpinEventFunction)
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("GameEventNode", "UnpinEventFunctionNotFound", "UnpinEvent function not found").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFuncNode->FunctionReference.SetExternalMember(UnpinEventFunction->GetFName(), UGameEventNodeUtils::StaticClass());
	CallFuncNode->AllocateDefaultPins();

	// Connect execution pins
	UEdGraphPin* CallFuncExecPin = CallFuncNode->GetExecPin();
	UEdGraphPin* CallFuncThenPin = CallFuncNode->GetThenPin();

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallFuncExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *CallFuncThenPin);

	// Connect Self pin
	if (UEdGraphPin* CallFuncSelfPin = CallFuncNode->FindPin(WorldContextObjectParamName))
	{
		CompilerContext.MovePinLinksToIntermediate(*GetSelfPin(), *CallFuncSelfPin);
	}

	// Connect event name pin using base class method
	if (UEdGraphPin* CallFuncEventNamePin = CallFuncNode->FindPin(EventNameParamName))
	{
		ConnectEventNameWithTagConversion(CompilerContext, SourceGraph, CallFuncEventNamePin);
	}

	// Break down original node
	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_UnpinEvent::GetSelfPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Self);
}

#undef LOCTEXT_NAMESPACE
