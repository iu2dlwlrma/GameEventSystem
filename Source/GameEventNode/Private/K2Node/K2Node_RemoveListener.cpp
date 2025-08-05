// Fill out your copyright notice in the Description page of Project Settings.

#include "K2Node/K2Node_RemoveListener.h"
#include "KismetCompiler.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "GameEventTypes.h"
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
	SelfPin->PinFriendlyName = NSLOCTEXT("K2Node", "RemoveListener_Self", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("K2Node", "RemoveListener_Self_Tooltip", "Target object for removing event").ToString();

	// Create standard event identifier pins
	CreateEventIdentifierPins();

	UpdatePinVisibility();
}

FText UK2Node_RemoveListener::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("K2Node", "RemoveListener_Tooltip", "Remove listener from game event system. 【{0}】"), FText::FromName(GetFName()));
}

FText UK2Node_RemoveListener::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "RemoveListener_Title", "Remove Listener");

}

FText UK2Node_RemoveListener::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("K2Node", "RemoveListener_Keywords", "{0} remove listener removelistener unbind unsubscribe unregister detach"),
	                     Super::GetKeywords());
}

void UK2Node_RemoveListener::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));

	static const UEnum* EventIdTypeEnum = StaticEnum<EEventIdType>();

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ThenPin = GetThenPin();
	UEdGraphPin* SelfPin = GetSelfPin();
	UEdGraphPin* EventTagPin = GetEventTagPin();
	UEdGraphPin* EventStringPin = GetEventStringPin();

	if (!ExecPin || !ThenPin || !SelfPin)
	{
		CompilerContext.MessageLog.Error(*FText::FromString(TEXT("Invalid pins in Remove Event node")).ToString(), this);
		return;
	}

	// Determine which remove function to use
	const UEdGraphPin* EventIdTypePin = GetEventIdTypePin();
	const FString EventIdTypeValue = EventIdTypePin->GetDefaultAsString();
	const int32 EventIdTypeIndex = EventIdTypeEnum->GetIndexByNameString(EventIdTypeValue);
	const bool bIsEventString = (EventIdTypeIndex != INDEX_NONE) && (EventIdTypeEnum->GetValueByIndex(EventIdTypeIndex) == static_cast<int64>(EEventIdType::StringBased));

	FName CallFuncName;
	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	if (bIsEventString)
	{
		CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, RemoveListener_StrKey);
	}
	else
	{
		CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, RemoveListener);
	}

	if (!CallFuncName.IsNone())
	{
		CallFuncNode->FunctionReference.SetExternalMember(CallFuncName, UGameEventNodeUtils::StaticClass());
		CallFuncNode->AllocateDefaultPins();

		// Connect WorldContextObject parameter
		UEdGraphPin* WorldContextObjectParam = CallFuncNode->FindPinChecked(WorldContextObjectParamName);
		UEdGraphPin* EventNameParam = CallFuncNode->FindPinChecked(EventNameParamName);

		
		if (bIsEventString)
		{
			CompilerContext.MovePinLinksToIntermediate(*EventStringPin, *EventNameParam);
		}
		else
		{
			CompilerContext.MovePinLinksToIntermediate(*EventTagPin, *EventNameParam);
		}

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
