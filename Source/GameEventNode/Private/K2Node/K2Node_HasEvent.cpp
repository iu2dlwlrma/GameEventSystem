// Copyright LetsGo. All Rights Reserved.

#include "K2Node/K2Node_HasEvent.h"
#include "GameEventNodeUtils.h"
#include "KismetCompiler.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"

#define LOCTEXT_NAMESPACE "UK2Node_HasEvent"

struct FK2Node_HasEventPinName
{
	static const FName ReturnValuePinName;
};

const FName FK2Node_HasEventPinName::ReturnValuePinName(TEXT("ReturnValue"));

void UK2Node_HasEvent::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("K2Node", "HasEvent_Self", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("K2Node", "HasEvent_Self_Tooltip", "World context object").ToString();

	// Create standard event identifier pins
	CreateEventIdentifierPins();

	// Return value pin
	UEdGraphPin* ReturnValuePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FK2Node_HasEventPinName::ReturnValuePinName);
	ReturnValuePin->PinFriendlyName = NSLOCTEXT("K2Node", "HasEvent_ReturnValue", "Event Exists");
	ReturnValuePin->PinToolTip = NSLOCTEXT("K2Node", "HasEvent_ReturnValue_Tooltip", "Whether the event exists").ToString();

	UpdatePinVisibility();
}

FText UK2Node_HasEvent::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("K2Node", "HasEvent_Tooltip", "Check if the specified event exists in the game event system. [{0}]"), FText::FromName(GetFName()));
}

FText UK2Node_HasEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "HasEvent_Title", "Has Even");
}

FText UK2Node_HasEvent::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("K2Node", "HasEvent_Keywords", "{0} has event hasevent check exist contains valid"),
	                     Super::GetKeywords());
}

void UK2Node_HasEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));

	const UEdGraphPin* EventIdTypePin = GetEventIdTypePin();

	// Determine which event identifier type to use
	const bool bIsEventString = UGameEventNodeUtils::IsStringEventId(EventIdTypePin);

	// Create function call node
	UFunction* HasEventFunction;
	if (bIsEventString)
	{
		HasEventFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, HasEvent_StrKey));
	}
	else
	{
		HasEventFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, HasEvent));
	}

	if (!HasEventFunction)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("HasEventFunctionNotFound", "HasEvent function not found").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFuncNode->FunctionReference.SetExternalMember(HasEventFunction->GetFName(), UGameEventNodeUtils::StaticClass());
	CallFuncNode->AllocateDefaultPins();

	if (UEdGraphPin* CallFuncSelfPin = CallFuncNode->FindPin(WorldContextObjectParamName))
	{
		CompilerContext.MovePinLinksToIntermediate(*GetSelfPin(), *CallFuncSelfPin);
	}

	if (UEdGraphPin* CallFuncEventNamePin = CallFuncNode->FindPin(EventNameParamName))
	{
		if (bIsEventString)
		{
			CompilerContext.MovePinLinksToIntermediate(*GetEventStringPin(), *CallFuncEventNamePin);
		}
		else
		{
			CompilerContext.MovePinLinksToIntermediate(*GetEventTagPin(), *CallFuncEventNamePin);
		}
	}

	UEdGraphPin* ReturnValuePin = GetReturnValuePin();
	UEdGraphPin* CallFuncReturnPin = CallFuncNode->GetReturnValuePin();
	if (ReturnValuePin && CallFuncReturnPin)
	{
		CompilerContext.MovePinLinksToIntermediate(*ReturnValuePin, *CallFuncReturnPin);
	}

	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_HasEvent::GetSelfPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Self);
}

UEdGraphPin* UK2Node_HasEvent::GetReturnValuePin() const
{
	return FindPinChecked(FK2Node_HasEventPinName::ReturnValuePinName);
}

#undef LOCTEXT_NAMESPACE
