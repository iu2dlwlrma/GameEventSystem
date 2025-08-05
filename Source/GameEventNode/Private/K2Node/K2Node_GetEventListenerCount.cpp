// Copyright LetsGo. All Rights Reserved.

#include "K2Node/K2Node_GetEventListenerCount.h"
#include "ToolMenu.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "GameEventNodeUtils.h"
#include "K2Node_CallFunction.h"

#define LOCTEXT_NAMESPACE "UK2Node_GetEventListenerCount"

struct FK2Node_GetEventListenerCountPinName
{
	static const FName ReturnValuePinName;
};

const FName FK2Node_GetEventListenerCountPinName::ReturnValuePinName(TEXT("ReturnValue"));

void UK2Node_GetEventListenerCount::AllocateDefaultPins()
{
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("K2Node", "GetEventListenerCount_Self", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("K2Node", "GetEventListenerCount_Self_Tooltip", "World context object").ToString();

	CreateEventIdentifierPins();

	if (UEdGraphPin* EventIdTypePin = GetEventIdTypePin())
	{
		EventIdTypePin->PinToolTip = NSLOCTEXT("K2Node", "GetEventListenerCount_EventIdType_Tooltip", "Event ID type: String or GameplayTag").ToString();
	}
	if (UEdGraphPin* EventTagPin = GetEventTagPin())
	{
		EventTagPin->PinToolTip = NSLOCTEXT("K2Node", "GetEventListenerCount_EventTag_Tooltip", "Event name to check").ToString();
	}
	if (UEdGraphPin* EventStringPin = GetEventStringPin())
	{
		EventStringPin->PinToolTip = NSLOCTEXT("K2Node", "GetEventListenerCount_EventString_Tooltip", "Event name to check").ToString();
	}

	UEdGraphPin* ReturnValuePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, FK2Node_GetEventListenerCountPinName::ReturnValuePinName);
	ReturnValuePin->PinFriendlyName = NSLOCTEXT("K2Node", "GetEventListenerCount_ReturnValue", "Listener Count");
	ReturnValuePin->PinToolTip = NSLOCTEXT("K2Node", "GetEventListenerCount_ReturnValue_Tooltip", "Number of listeners for this event").ToString();

	UpdatePinVisibility();
}

FText UK2Node_GetEventListenerCount::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("K2Node", "GetEventListenerCount_Tooltip", "Get the number of listeners for the specified event. [{0}]"), FText::FromName(GetFName()));
}

FText UK2Node_GetEventListenerCount::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Get Event Listener Count"));
}

FText UK2Node_GetEventListenerCount::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("K2Node", "GetEventListenerCount_Keywords", "{0} get event listener count geteventlistenercount number amount size num"),
	                     Super::GetKeywords());
}

void UK2Node_GetEventListenerCount::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));

	UFunction* GetListenerCountFunction;
	const bool bIsEventString = UGameEventNodeUtils::IsStringEventId(GetEventIdTypePin());
	if (bIsEventString)
	{
		GetListenerCountFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, GetEventListenerCount_StrKey));
	}
	else
	{
		GetListenerCountFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, GetEventListenerCount));
	}

	if (!GetListenerCountFunction)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("GetEventListenerCountFunctionNotFound", "GetEventListenerCount function not found").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFuncNode->FunctionReference.SetExternalMember(GetListenerCountFunction->GetFName(), UGameEventNodeUtils::StaticClass());
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

UEdGraphPin* UK2Node_GetEventListenerCount::GetSelfPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Self);
}

UEdGraphPin* UK2Node_GetEventListenerCount::GetReturnValuePin() const
{
	return FindPinChecked(FK2Node_GetEventListenerCountPinName::ReturnValuePinName);
}

#undef LOCTEXT_NAMESPACE
