#include "K2Node/K2Node_SendEvent.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "GameEventTypes.h"
#include "GameEventNodeUtils.h"
#include "GameEventTypeManager.h"
#include "K2Node_EnumLiteral.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "UK2Node_SendEvent"

struct FK2Node_SendEventPinName
{
	static const FName PinnedName;
	static const FName ParamDataName;
};

const FName FK2Node_SendEventPinName::PinnedName(TEXT("Pinned"));
const FName FK2Node_SendEventPinName::ParamDataName(TEXT("ParamData"));

void UK2Node_SendEvent::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	//Input Pin & Output Pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Self pin
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("K2Node", "SendEvent_Self", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("K2Node", "SendEvent_Self_Tooltip", "Target object for sending event").ToString();

	// EventIdType pin
	// Create standard event identifier pins
	CreateEventIdentifierPins();

	// Pinned Pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, FK2Node_SendEventPinName::PinnedName);

	UpdatePinVisibility();
}

FText UK2Node_SendEvent::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("K2Node", "SendEvent_Tooltip", "Sends an event to the game event system. 【{0}】"), FText::FromName(GetFName()));
}

FText UK2Node_SendEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "SendEvent_Title", "Send Event");
}

FText UK2Node_SendEvent::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("K2Node", "SendEvent_Keywords", "{0} send event sendevent trigger fire emit dispatch broadcast"),
	                     Super::GetKeywords());
}

void UK2Node_SendEvent::PostReconstructNode()
{
	Super::PostReconstructNode();

	RefreshPinTypes();
}

void UK2Node_SendEvent::Serialize(FArchive& Ar)
{
	for (int32 Index = Pins.Num() - 1; Index >= 0; --Index)
	{
		const UEdGraphPin* Pin = Pins[Index];
		if (Pin && !Pin->GetOwningNodeUnchecked())
		{
			Pins.RemoveAt(Index);
		}
	}
	Super::Serialize(Ar);
}

void UK2Node_SendEvent::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	// Call base class method to handle event identifier related logic
	Super::PinDefaultValueChanged(Pin);

	// SendEvent specific logic: handle parameter type refresh when event name changes
	if (Pin && (Pin->PinName == FGameEventBasePinNames::EventTagPinName ||
	            Pin->PinName == FGameEventBasePinNames::EventStringPinName))
	{
		RefreshPinTypes();
	}
}

void UK2Node_SendEvent::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	//Super::ReallocatePinsDuringReconstruction(OldPins);
	AllocateDefaultPins();

	// Preserve default values and type information for important pins
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->PinName == FK2Node_SendEventPinName::ParamDataName)
		{
			UEdGraphPin* NewPin = FindPin(OldPin->PinName);
			if (!NewPin)
			{
				if (CheckParamDataPinTypeMatch(OldPin))
				{
					NewPin = CreateParamDataPin(OldPin->PinType.PinCategory);
					NewPin->PinType = OldPin->PinType;
				}
			}
		}
	}

	RestoreSplitPins(OldPins);
}

void UK2Node_SendEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static TMap<FName, FName> MakeLiteralFunctionTable =
	{
		{ UEdGraphSchema_K2::PC_Boolean, TEXT("MakeLiteralBool") },
		{ UEdGraphSchema_K2::PC_Byte, TEXT("MakeLiteralByte") },
		{ UEdGraphSchema_K2::PC_Int, TEXT("MakeLiteralInt") },
		{ UEdGraphSchema_K2::PC_Int64, TEXT("MakeLiteralInt64") },
		{ UEdGraphSchema_K2::PC_Name, TEXT("MakeLiteralName") },
		{ UEdGraphSchema_K2::PC_Real, TEXT("MakeLiteralDouble") },
		{ UEdGraphSchema_K2::PC_String, TEXT("MakeLiteralString") },
		{ UEdGraphSchema_K2::PC_Text, TEXT("MakeLiteralText") },
	};

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));
	static const FName PinnedParamName(TEXT("bPinned"));
	static const FName ParamDataParamName(TEXT("ParamData"));

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ThenPin = GetThenPin();
	UEdGraphPin* SelfPin = GetSelfPin();
	UEdGraphPin* EventTagPin = GetEventTagPin();
	UEdGraphPin* EventStringPin = GetEventStringPin();
	UEdGraphPin* PinnedPin = GetPinnedPin();
	UEdGraphPin* ParamDataPin = GetParamDataPin();

	if (!ExecPin || !ThenPin || !SelfPin)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidPins", "Invalid pins in @@").ToString(), this);
		return;
	}
	if (!CheckParamDataPinTypeMatch())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("ParamData", "Type mismatch, please refresh the node! @@").ToString(), this);
		return;
	}

	const bool bIsEventString = UGameEventNodeUtils::IsStringEventId(GetEventIdTypePin());

	FName CallFuncName;
	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	// Check if there's parameter data - ParamDataPin may not exist
	bool bHasValue = false;
	if (ParamDataPin)
	{
		const bool bHasDefaultValue = !ParamDataPin->DefaultValue.IsEmpty() || ParamDataPin->DefaultObject || !ParamDataPin->DefaultTextValue.IsEmpty();
		bHasValue = ParamDataPin->LinkedTo.Num() > 0 || bHasDefaultValue;
	}

	if (bIsEventString)
	{
		if (bHasValue)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEvent_StrKey);
		}
		else
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEvent_NoParam_StrKey);
		}
	}
	else
	{
		if (bHasValue)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEvent);
		}
		else
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEvent_NoParam);
		}
	}

	if (!CallFuncName.IsNone())
	{
		CallFuncNode->FunctionReference.SetExternalMember(CallFuncName, UGameEventNodeUtils::StaticClass());
		CallFuncNode->AllocateDefaultPins();

		// Connect WorldContextObject parameter
		UEdGraphPin* WorldContextObjectParam = CallFuncNode->FindPinChecked(WorldContextObjectParamName);
		UEdGraphPin* EventNameParam = CallFuncNode->FindPinChecked(EventNameParamName);
		UEdGraphPin* PinnedParam = CallFuncNode->FindPinChecked(PinnedParamName);

		// Connect event name parameter
		if (bIsEventString)
		{
			CompilerContext.MovePinLinksToIntermediate(*EventStringPin, *EventNameParam);
		}
		else
		{
			CompilerContext.MovePinLinksToIntermediate(*EventTagPin, *EventNameParam);
		}

		CompilerContext.MovePinLinksToIntermediate(*SelfPin, *WorldContextObjectParam);
		CompilerContext.MovePinLinksToIntermediate(*PinnedPin, *PinnedParam);

		// If there's parameter data, connect parameter data
		if (bHasValue && ParamDataPin)
		{
			UEdGraphPin* ParamDataParam = CallFuncNode->FindPinChecked(ParamDataParamName);
			ParamDataParam->PinType = ParamDataPin->PinType;
			// Handle connected pins, unconnected pins need to be linked, otherwise reflection won't get them and cause crashes
			if (ParamDataPin->LinkedTo.Num() == 0)
			{
				// Basic types
				if (const FName* MakeLiteralFunctionName = MakeLiteralFunctionTable.Find(ParamDataPin->PinType.PinCategory))
				{
					// Enum types
					if (ParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && ParamDataPin->PinType.PinSubCategoryObject.Get())
					{
						UK2Node_EnumLiteral* EnumLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(this, SourceGraph);
						EnumLiteralNode->Enum = CastChecked<UEnum>(ParamDataPin->PinType.PinSubCategoryObject.Get());
						EnumLiteralNode->AllocateDefaultPins();

						UEdGraphPin* EnumBytePin = EnumLiteralNode->FindPinChecked(FName("Enum"));
						CompilerContext.MovePinLinksToIntermediate(*ParamDataPin, *EnumBytePin);

						UEdGraphPin* OutputPin = EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
						ParamDataParam->MakeLinkTo(OutputPin);
					}
					else
					{
						const UFunction* MakeLiteralFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(*MakeLiteralFunctionName);
						UK2Node_CallFunction* MakeLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
						MakeLiteralNode->SetFromFunction(MakeLiteralFunction);
						MakeLiteralNode->AllocateDefaultPins();

						UEdGraphPin* MakeLiteralPin = MakeLiteralNode->FindPinChecked(TEXT("Value"));
						CompilerContext.MovePinLinksToIntermediate(*ParamDataPin, *MakeLiteralPin);

						UEdGraphPin* OutputPin = MakeLiteralNode->GetReturnValuePin();
						ParamDataParam->MakeLinkTo(OutputPin);
					}
				}
				else
				{
					CompilerContext.MovePinLinksToIntermediate(*ParamDataPin, *ParamDataParam);
				}
			}
			else
			{
				CompilerContext.MovePinLinksToIntermediate(*ParamDataPin, *ParamDataParam);
			}
		}

		// Connect execution pins
		CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallFuncNode->GetExecPin());
		CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFuncNode->GetThenPin());
	}

	BreakAllNodeLinks();
}

void UK2Node_SendEvent::RefreshPinTypes()
{
	const FString EventName = GetCurrentEventName();
	FEventTypeInfo TypeInfo;
	FGameEventTypeManager::Get()->GetEventTypeInfo(EventName, TypeInfo);
	const bool bIsNoParams = EventName.IsEmpty();
	UEdGraphPin* ParamDataPin = GetParamDataPin();
	if (bIsNoParams)
	{
		if (ParamDataPin)
		{
			ParamDataPin->BreakAllPinLinks();
			RemovePin(ParamDataPin);
		}
	}
	else
	{
		if (!ParamDataPin && !TypeInfo.bIsNoParams)
		{
			// If pin doesn't exist, create it
			ParamDataPin = CreateParamDataPin(UEdGraphSchema_K2::PC_Wildcard);
		}
		if (!CheckParamDataPinTypeMatch(ParamDataPin))
		{
			ParamDataPin->PinType.PinCategory = TypeInfo.PinCategory;
			ParamDataPin->PinType.PinSubCategory = TypeInfo.PinSubCategory;
			ParamDataPin->PinType.PinSubCategoryObject = TypeInfo.PinSubCategoryObject;
			ParamDataPin->PinType.PinValueType = TypeInfo.PinValueType;
			ParamDataPin->PinType.ContainerType = TypeInfo.ContainerType;
			if (ParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
			    ParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
			    ParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
			    ParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass ||
			    ParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
			{
				ParamDataPin->bDefaultValueIsIgnored = true;
			}
			else
			{
				ParamDataPin->bDefaultValueIsIgnored = false;
			}
		}
	}

	GetGraph()->NotifyGraphChanged();
}

bool UK2Node_SendEvent::CheckParamDataPinTypeMatch(const UEdGraphPin* Pin) const
{
	const UEdGraphPin* ParamDataPin = Pin == nullptr ? GetParamDataPin() : Pin;
	const FString EventName = GetCurrentEventName();
	FEventTypeInfo TypeInfo;
	if (ParamDataPin && !EventName.IsEmpty() && FGameEventTypeManager::Get()->GetEventTypeInfo(EventName, TypeInfo))
	{
		const FEdGraphPinType& PinType = ParamDataPin->PinType;

		// Check basic type information
		if (PinType.PinCategory != TypeInfo.PinCategory ||
		    PinType.PinSubCategory != TypeInfo.PinSubCategory ||
		    PinType.PinSubCategoryObject != TypeInfo.PinSubCategoryObject)
		{
			return false;
		}

		// Check container types
		if (PinType.ContainerType != TypeInfo.ContainerType)
		{
			return false;
		}

		// For Map type, also need to check PinValueType (Value type information)
		if (TypeInfo.IsMap())
		{
			if (PinType.PinValueType.TerminalCategory != TypeInfo.PinValueType.TerminalCategory ||
			    PinType.PinValueType.TerminalSubCategory != TypeInfo.PinValueType.TerminalSubCategory ||
			    PinType.PinValueType.TerminalSubCategoryObject != TypeInfo.PinValueType.TerminalSubCategoryObject)
			{
				return false;
			}
		}
	}
	return true;
}

UEdGraphPin* UK2Node_SendEvent::CreateParamDataPin(const FName PinCategory)
{
	UEdGraphPin* Pin = CreatePin(EGPD_Input, PinCategory, FK2Node_SendEventPinName::ParamDataName);
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		Pin->bDefaultValueIsIgnored = true;
	}
	else
	{
		Pin->bDefaultValueIsIgnored = false;
	}
	return Pin;
}

#pragma region GetPin
UEdGraphPin* UK2Node_SendEvent::GetSelfPin() const
{
	UEdGraphPin* Pin = FindPin(UEdGraphSchema_K2::PN_Self);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SendEvent::GetPinnedPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_SendEventPinName::PinnedName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SendEvent::GetParamDataPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_SendEventPinName::ParamDataName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}
#pragma endregion

#undef LOCTEXT_NAMESPACE
