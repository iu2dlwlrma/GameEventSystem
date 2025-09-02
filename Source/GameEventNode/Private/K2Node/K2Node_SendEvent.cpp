#include "K2Node/K2Node_SendEvent.h"

#include "GameEventNodeLog.h"
#include "KismetCompiler.h"
#include "GameEventNodeTypes.h"
#include "GameEventNodeUtils.h"
#include "GameEventTypeManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnumLiteral.h"

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
	GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME()

	Super::AllocateDefaultPins();

	//Input Pin & Output Pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Self pin
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("GameEventNode", "Target", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("GameEventNode", "Target_Tooltip", "Target object for listening").ToString();

	// Pinned Pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, FK2Node_SendEventPinName::PinnedName);

	CreateEventIdentifierPins(1);
}

FText UK2Node_SendEvent::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "SendEvent_Tooltip", "Sends an event to the game event system. 【{0}】"), FText::FromName(GetFName()));
}

FText UK2Node_SendEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("GameEventNode", "SendEvent_Title", "Send Event");
}

FText UK2Node_SendEvent::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "SendEvent_Keywords", "{0} send event sendevent trigger fire emit dispatch broadcast"), Super::GetKeywords());
}

void UK2Node_SendEvent::PostReconstructNode()
{
	GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME()

	Super::PostReconstructNode();

	RefreshPinTypes();

	const FString EventName = GetCurrentEventName();
	if (!EventName.IsEmpty() && !bIsBindSuccess)
	{
		bIsBindSuccess = FGameEventTypeManager::Get()->BindEventTypeNotify(EventName,
		                                                                   GetUniqueID(),
		                                                                   [this]
		                                                                   {
			                                                                   if (IsValid(this))
			                                                                   {
				                                                                   RefreshPinTypes();
			                                                                   }
		                                                                   });
	}

	const FString PrefixStr = FK2Node_SendEventPinName::ParamDataName.ToString();

	for (int32 Index = Pins.Num() - 1; Index >= 0; --Index)
	{
		if (const UEdGraphPin* Pin = Pins[Index])
		{
			if (!Pin->GetOwningNodeUnchecked())
			{
				Pins.RemoveAt(Index);
				continue;
			}
			const FString PinNameStr = Pin->PinName.ToString();
			if (PinNameStr.StartsWith(PrefixStr) && PinNameStr.Len() > PrefixStr.Len())
			{
				const FString NumberPart = PinNameStr.Mid(PrefixStr.Len());
				int32 IndexNumber;
				LexTryParseString<int32>(IndexNumber, *NumberPart);
				if (IndexNumber >= GlobalConfig.MaxParameterNum)
				{
					Pins.RemoveAt(Index);
				}
			}
		}
	}
}

void UK2Node_SendEvent::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME()

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
	GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME()

	// Super::ReallocatePinsDuringReconstruction(OldPins);
	AllocateDefaultPins();

	// Preserve default values and type information for important pins
	for (const UEdGraphPin* OldPin : OldPins)
	{
		// Check if it is a ParamData pin
		bool bIsParamDataPin = false;
		if (OldPin->PinName == FK2Node_SendEventPinName::ParamDataName)
		{
			bIsParamDataPin = true;
		}
		else
		{
			// Check if it is the numbered ParamData pin
			const FString PinNameStr = OldPin->PinName.ToString();
			const FString PrefixStr = FK2Node_SendEventPinName::ParamDataName.ToString();
			if (PinNameStr.StartsWith(PrefixStr) && PinNameStr.Len() > PrefixStr.Len())
			{
				const FString NumberPart = PinNameStr.Mid(PrefixStr.Len());
				if (NumberPart.IsNumeric())
				{
					bIsParamDataPin = true;
				}
			}
		}

		if (bIsParamDataPin)
		{
			UEdGraphPin* NewPin = FindPin(OldPin->PinName);
			if (!NewPin)
			{
				NewPin = CreatePin(EGPD_Input, OldPin->PinType.PinCategory, OldPin->PinName);
			}
			NewPin->PinType = OldPin->PinType;
			NewPin->DefaultObject = OldPin->DefaultObject;
			NewPin->DefaultTextValue = OldPin->DefaultTextValue;
			NewPin->DefaultValue = OldPin->DefaultValue;
			NewPin->bDefaultValueIsIgnored = NewPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object;
		}
	}

	RestoreSplitPins(OldPins);
}

void UK2Node_SendEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME()

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

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ThenPin = GetThenPin();
	UEdGraphPin* SelfPin = GetSelfPin();
	UEdGraphPin* PinnedPin = GetPinnedPin();

	if (!ExecPin || !ThenPin || !SelfPin)
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("GameEventNode", "InvalidPins", "Invalid pins in @@").ToString(), this);
		return;
	}
	if (!CheckParamDataPinsTypeMatch())
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("GameEventNode", "ParamDataMatch", "Type mismatch, please refresh the node! @@").ToString(), this);
		return;
	}

	const TArray<UEdGraphPin*> ParamDataPins = GetAllParamDataPins();
	const int32 ParamCount = ParamDataPins.Num();
	for (const UEdGraphPin* ParamDataPin : ParamDataPins)
	{
		const bool bValid = ParamDataPin != nullptr || !ParamDataPin->DefaultValue.IsEmpty() || ParamDataPin->DefaultObject || !ParamDataPin->DefaultTextValue.IsEmpty();
		if (!bValid)
		{
			const FText MessageLog = FText::Format(NSLOCTEXT("GameEventNode", "ParamDataValid", "Parameter '{0}' Invalid parameters! @@"), FText::FromString(ParamDataPin->PinName.ToString()));
			CompilerContext.MessageLog.Error(*MessageLog.ToString(), this);
			return;
		}
		const bool bHasConnection = ParamDataPin->LinkedTo.Num() > 0;
		const bool bHasDefaultValue = !ParamDataPin->DefaultValue.IsEmpty() || ParamDataPin->DefaultObject != nullptr || !ParamDataPin->DefaultTextValue.IsEmpty();

		// The pin is valid if it's connected OR if it has a valid default value.
		if (!bHasConnection && !bHasDefaultValue)
		{
			// Special case: Booleans can have a default value of "false" which is an empty string. We assume they are always valid if unconnected.
			if (ParamDataPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Boolean)
			{
				const FText MessageLog = FText::Format(NSLOCTEXT("GameEventNode", "ParamDataValid", "Parameter '{0}' is not connected and has no default value! @@"), FText::FromString(ParamDataPin->PinName.ToString()));
				CompilerContext.MessageLog.Error(*MessageLog.ToString(), this);
				return;
			}
		}
	}

	FName CallFuncName;
	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	const bool bHasParam = ParamCount > 0;
	if (bHasParam)
	{
		if (ParamCount == 1)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEvent);
		}
		else if (ParamCount == 2)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventTwoParam);
		}
		else if (ParamCount == 3)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventThreeParam);
		}
		else if (ParamCount == 4)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventFourParam);
		}
		else if (ParamCount == 5)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventFiveParam);
		}
		else if (ParamCount == 6)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventSixParam);
		}
		else if (ParamCount == 7)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventSevenParam);
		}
		else if (ParamCount == 8)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEventEightParam);
		}
	}
	else
	{
		CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, SendEvent_NoParam);
	}

	if (!CallFuncName.IsNone())
	{
		CallFuncNode->FunctionReference.SetExternalMember(CallFuncName, UGameEventNodeUtils::StaticClass());
		CallFuncNode->AllocateDefaultPins();

		UEdGraphPin* WorldContextObjectParam = CallFuncNode->FindPinChecked(WorldContextObjectParamName);
		UEdGraphPin* EventNameParam = CallFuncNode->FindPinChecked(EventNameParamName);
		UEdGraphPin* PinnedParam = CallFuncNode->FindPinChecked(PinnedParamName);

		ConnectEventNameWithTagConversion(CompilerContext, SourceGraph, EventNameParam);

		CompilerContext.MovePinLinksToIntermediate(*SelfPin, *WorldContextObjectParam);
		CompilerContext.MovePinLinksToIntermediate(*PinnedPin, *PinnedParam);

		if (bHasParam)
		{
			for (int i = 0; i < ParamCount; ++i)
			{
				if (UEdGraphPin* ParamDataPin = ParamDataPins[i])
				{
					const FName ParamDataParamName = i == 0 ? FName(TEXT("ParamData")) : FName(*FString::Printf(TEXT("ParamData%d"), i));
					UEdGraphPin* ParamDataParam = CallFuncNode->FindPinChecked(ParamDataParamName);
					ParamDataParam->PinType = ParamDataPin->PinType;

					UE_LOG_GAS_EDITOR(TEXT("PinCategory '%s'  -- PinSubCategoryObject '%s'"), *ParamDataPin->PinType.PinCategory.ToString(), *GetNameSafe(ParamDataPin->PinType.PinSubCategoryObject.Get()));

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

								UEdGraphPin* EnumInputPin = EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
								CompilerContext.MovePinLinksToIntermediate(*ParamDataPin, *EnumInputPin);

								UEdGraphPin* EnumOutputPin = EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
								ParamDataParam->MakeLinkTo(EnumOutputPin);
							}
							else
							{
								const UFunction* MakeLiteralFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(*MakeLiteralFunctionName);
								UK2Node_CallFunction* MakeLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
								MakeLiteralNode->SetFromFunction(MakeLiteralFunction);
								MakeLiteralNode->AllocateDefaultPins();

								UEdGraphPin* MakeLiteralValuePin = MakeLiteralNode->FindPinChecked(TEXT("Value"));
								CompilerContext.MovePinLinksToIntermediate(*ParamDataPin, *MakeLiteralValuePin);
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
			}
		}

		CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallFuncNode->GetExecPin());
		CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFuncNode->GetThenPin());
	}

	BreakAllNodeLinks();
}

void UK2Node_SendEvent::DestroyNode()
{
	Super::DestroyNode();

	const FString EventName = GetCurrentEventName();
	if (!EventName.IsEmpty() && bIsBindSuccess)
	{
		FGameEventTypeManager::Get()->UnBindEventTypeNotify(EventName, GetUniqueID());
	}
}

void UK2Node_SendEvent::RefreshPinTypes()
{
	if (UE::GetIsEditorLoadingPackage() || !GIsEditor || IsTemplate() || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	GAME_SCOPED_TRACK_LOG_AUTO_BLUEPRINT_NAME();

	const FString EventName = GetCurrentEventName();
	FEventTypeInfo TypeInfo;
	const bool bHasTypeInfo = FGameEventTypeManager::Get()->GetEventTypeInfo(EventName, TypeInfo);
	const bool bIsNoParams = EventName.IsEmpty() || !bHasTypeInfo || !TypeInfo.IsValid();

	const TArray<UEdGraphPin*> CurrentParamDataPins = GetAllParamDataPins();

	if (bIsNoParams)
	{
		// If there are no parameters, remove all existing parameter pins.
		for (UEdGraphPin* ParamDataPin : CurrentParamDataPins)
		{
			if (ParamDataPin)
			{
				ParamDataPin->BreakAllPinLinks();
				SafeRemovePin(ParamDataPin);
			}
		}
	}
	else
	{
		// This block handles the case where the event should have parameters.
		const int32 ExpectedParamCount = TypeInfo.GetParameterCount();
		const int32 CurrentParamCount = CurrentParamDataPins.Num();

		// 1. Remove extra pins from the end if the new parameter count is smaller.
		// Iterate backwards to safely remove elements.
		if (CurrentParamCount > ExpectedParamCount)
		{
			for (int32 i = CurrentParamCount - 1; i >= ExpectedParamCount; --i)
			{
				if (UEdGraphPin* PinToRemove = CurrentParamDataPins[i])
				{
					PinToRemove->BreakAllPinLinks();
					SafeRemovePin(PinToRemove);
				}
			}
		}

		// 2. Iterate through all expected parameters to update existing pins or create new ones.
		for (int32 i = 0; i < ExpectedParamCount; ++i)
		{
			const FEventParameterInfo* ParamInfo = TypeInfo.GetParameterInfo(i);
			if (!ParamInfo)
			{
				// Skip if parameter info is somehow invalid for this index.
				continue;
			}

			// Get the current pin at this index, if it exists.
			UEdGraphPin* CurrentPin = i < CurrentParamDataPins.Num() ? CurrentParamDataPins[i] : nullptr;

			// Check if the current pin is compatible with the expected parameter type.
			const bool bIsCompatible = CurrentPin &&
			                           CurrentPin->PinType.PinCategory == ParamInfo->PinCategory &&
			                           CurrentPin->PinType.PinSubCategory == ParamInfo->PinSubCategory &&
			                           CurrentPin->PinType.PinSubCategoryObject == ParamInfo->PinSubCategoryObject.Get() &&
			                           CurrentPin->PinType.ContainerType == ParamInfo->ContainerType;

			// If the pin is already correct, do nothing and continue to the next one. This preserves connections.
			if (bIsCompatible)
			{
				continue;
			}

			// If the pin is not compatible (or doesn't exist), we need to recreate it.
			// First, remove the old, incompatible pin if it exists.
			if (CurrentPin)
			{
				CurrentPin->BreakAllPinLinks();
				SafeRemovePin(CurrentPin);
			}

			// Create the new, correct pin for the current parameter.
			// Assumes CreateParamDataPinAtIndex correctly creates a pin at the specified index.
			if (UEdGraphPin* NewParamDataPin = CreateParamDataPinAtIndex(i, ParamInfo->PinCategory))
			{
				// Configure the new pin's type information based on ParamInfo.
				NewParamDataPin->PinType.PinCategory = ParamInfo->PinCategory;
				NewParamDataPin->PinType.PinSubCategory = ParamInfo->PinSubCategory;
				NewParamDataPin->PinType.PinSubCategoryObject = ParamInfo->PinSubCategoryObject;
				NewParamDataPin->PinType.PinValueType = ParamInfo->PinValueType;
				NewParamDataPin->PinType.ContainerType = ParamInfo->ContainerType;

				// Set default value behavior based on the pin category
				if (NewParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
				    NewParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
				    NewParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
				    NewParamDataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
				{
					NewParamDataPin->bDefaultValueIsIgnored = true;
				}
				else
				{
					NewParamDataPin->bDefaultValueIsIgnored = false;
				}

				GetDefault<UEdGraphSchema_K2>()->SetPinAutogeneratedDefaultValueBasedOnType(NewParamDataPin);
			}
		}
	}

	// Notify the graph that the node has been modified to trigger a visual update.
	GetGraph()->NotifyNodeChanged(this);
}

void UK2Node_SendEvent::SafeRemovePin(UEdGraphPin* PinToRemove)
{
	if (!PinToRemove)
	{
		return;
	}

	// If the pin is split, the correct engine-level way to clean it up is to recombine it first.
	// This will handle breaking links on sub-pins and cleaning them up properly.
	if (PinToRemove->SubPins.Num() > 0)
	{
		if (const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>())
		{
			// Recombining the pin is the canonical way to "un-split" it.
			K2Schema->RecombinePin(PinToRemove);
		}
	}

	// Now that the pin is guaranteed to be a single entity, break its links and remove it from the node.
	PinToRemove->BreakAllPinLinks(true); // 'true' notifies the other end of the link
	RemovePin(PinToRemove);
}

UEdGraphPin* UK2Node_SendEvent::CreateParamDataPinAtIndex(const int32 Index, const FName PinCategory)
{
	const FName PinName = UGameEventNodeUtils::GetMultiParameterPinName(FK2Node_SendEventPinName::ParamDataName, Index);
	UEdGraphPin* Pin = CreatePin(EGPD_Input, PinCategory, PinName);
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		Pin->bDefaultValueIsIgnored = true;
	}
	else
	{
		Pin->bDefaultValueIsIgnored = false;
	}

	return Pin;
}

TArray<UEdGraphPin*> UK2Node_SendEvent::GetAllParamDataPins() const
{
	TArray<UEdGraphPin*> ParamDataPins;

	int32 Index = 0;
	while (UEdGraphPin* Pin = GetParamDataPinByIndex(Index))
	{
		ParamDataPins.Add(Pin);
		Index++;
	}
	return ParamDataPins;
}

UEdGraphPin* UK2Node_SendEvent::GetParamDataPinByIndex(const int32 Index) const
{
	const FName PinName = UGameEventNodeUtils::GetMultiParameterPinName(FK2Node_SendEventPinName::ParamDataName, Index);
	return FindPin(PinName);
}

bool UK2Node_SendEvent::CheckParamDataPinsTypeMatch() const
{
	const FString EventName = GetCurrentEventName();
	FEventTypeInfo TypeInfo;
	if (EventName.IsEmpty() || !FGameEventTypeManager::Get()->GetEventTypeInfo(EventName, TypeInfo))
	{
		return true;
	}

	const TArray<UEdGraphPin*> ParamDataPins = GetAllParamDataPins();
	const int32 ExpectedParamCount = TypeInfo.GetParameterCount();

	if (ParamDataPins.Num() != ExpectedParamCount)
	{
		return false;
	}

	for (int32 i = 0; i < ParamDataPins.Num(); ++i)
	{
		const UEdGraphPin* ParamDataPin = ParamDataPins[i];
		const FEventParameterInfo* ExpectedParam = TypeInfo.GetParameterInfo(i);

		if (!ParamDataPin || !ExpectedParam)
		{
			return false;
		}

		const FEdGraphPinType& PinType = ParamDataPin->PinType;

		if (PinType.PinCategory != ExpectedParam->PinCategory ||
		    PinType.PinSubCategory != ExpectedParam->PinSubCategory ||
		    PinType.PinSubCategoryObject != ExpectedParam->PinSubCategoryObject)
		{
			return false;
		}

		if (PinType.ContainerType != ExpectedParam->ContainerType)
		{
			return false;
		}

		if (ExpectedParam->IsMap())
		{
			if (PinType.PinValueType.TerminalCategory != ExpectedParam->PinValueType.TerminalCategory ||
			    PinType.PinValueType.TerminalSubCategory != ExpectedParam->PinValueType.TerminalSubCategory ||
			    PinType.PinValueType.TerminalSubCategoryObject != ExpectedParam->PinValueType.TerminalSubCategoryObject)
			{
				return false;
			}
		}
	}

	return true;
}

#pragma region "GetPin"
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
