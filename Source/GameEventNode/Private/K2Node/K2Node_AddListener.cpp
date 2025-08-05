#include "K2Node/K2Node_AddListener.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "GameEventTypes.h"
#include "K2Node_Variable.h"
#include "GameEventNodeLog.h"
#include "GameEventNodeUtils.h"
#include "GameEventTypeManager.h"
#include "K2Node_CreateDelegate.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Runtime/Engine/Internal/Kismet/BlueprintTypeConversions.h"

#define LOCTEXT_NAMESPACE "UK2Node_AddListener"

struct FK2Node_AddListenerPinName
{
	static const FName FunctionNamePinName;
	static const FName EventBindTypePinName;
	static const FName DelegatePinName;
	static const FName DataTypePinName;
};

const FName FK2Node_AddListenerPinName::FunctionNamePinName(TEXT("FunctionName"));
const FName FK2Node_AddListenerPinName::EventBindTypePinName(TEXT("EventBindType"));
const FName FK2Node_AddListenerPinName::DelegatePinName(TEXT("Delegate"));
const FName FK2Node_AddListenerPinName::DataTypePinName(TEXT("DataType"));

#pragma region Overrides

void UK2Node_AddListener::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Create Self pin
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("K2Node", "AddListener_Self", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_Self_Tooltip", "Target object for listening").ToString();

	// Create standard event identifier pins
	CreateEventIdentifierPins();

	// Set AddListener specific tooltips
	if (UEdGraphPin* EventIdTypePin = GetEventIdTypePin())
	{
		EventIdTypePin->DefaultValue = StaticEnum<EEventIdType>()->GetNameStringByValue(static_cast<int64>(EEventIdType::TagBased));
		EventIdTypePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_EventIdType_Tooltip", "Event ID type: String or GameplayTag").ToString();
	}

	if (UEdGraphPin* EventTagPin = GetEventTagPin())
	{
		EventTagPin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_EventTag_Tooltip", "Event name to register").ToString();
	}

	if (UEdGraphPin* EventStringPin = GetEventStringPin())
	{
		EventStringPin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_EventString_Tooltip", "Event name to register").ToString();
	}

	// Create binding type selection pin
	UEdGraphPin* EventBindTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, StaticEnum<EEventBindType>(), FK2Node_AddListenerPinName::EventBindTypePinName);
	EventBindTypePin->DefaultValue = StaticEnum<EEventBindType>()->GetNameStringByValue(static_cast<int64>(EEventBindType::FunctionName));
	EventBindTypePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_BindType_Tooltip", "Binding type: Function name or Delegate").ToString();

	// Create function name pin
	UEdGraphPin* FunctionNamePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, FK2Node_AddListenerPinName::FunctionNamePinName);
	FunctionNamePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_FunctionName_Tooltip", "Target object's function name").ToString();

	// // Create data type pin (only shown in delegate mode)
	// UEdGraphPin* DataTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FK2Node_AddListenerPinName::DataTypePinName);
	// DataTypePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_DataType_Tooltip", "Parameter type for delegate binding").ToString();

	// Create delegate pin
	UEdGraphPin* DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, FK2Node_AddListenerPinName::DelegatePinName);
	DelegatePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_Delegate_Tooltip", "Delegate to bind").ToString();

	// Set initial visibility
	UpdatePinVisibility();
}

FText UK2Node_AddListener::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("K2Node", "AddListener_Tooltip", "Add a listener to the game event system. 【{0}】"), FText::FromName(GetFName()));
}

FText UK2Node_AddListener::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "AddListener_Title", "Add Listener");
}

FText UK2Node_AddListener::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("K2Node", "AddListener_Keywords", "{0} add listener addlistener bind subscribe register hook attach"),
	                     Super::GetKeywords());
}

void UK2Node_AddListener::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	// First call parent class method
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// Preserve default values for important pins
	for (UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->PinName == FGameEventBasePinNames::EventIdTypePinName ||
		    OldPin->PinName == FK2Node_AddListenerPinName::EventBindTypePinName ||
		    OldPin->PinName == FGameEventBasePinNames::EventTagPinName ||
		    OldPin->PinName == FGameEventBasePinNames::EventStringPinName ||
		    OldPin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
		{
			if (OldPin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
			{
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FK2Node_AddListenerPinName::DataTypePinName);
				NewPin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_DataType_Tooltip", "Parameter type for delegate binding").ToString();
				NewPin->PinType = OldPin->PinType;
				NewPin->DefaultObject = OldPin->DefaultObject;
				NewPin->DefaultTextValue = OldPin->DefaultTextValue;
				NewPin->DefaultValue = OldPin->DefaultValue;
				NewPin->bNotConnectable = true;
			}
		}
	}
}

void UK2Node_AddListener::PostReconstructNode()
{
	Super::PostReconstructNode();

	AnalyzeFunctionAndRegisterEventType();
}

void UK2Node_AddListener::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));
	static const FName FunctionNameParamName(TEXT("FunctionName"));

	AnalyzeFunctionAndRegisterEventType();

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ThenPin = GetThenPin();

	UEdGraphPin* SelfPin = GetSelfPin();
	UEdGraphPin* EventTagPin = GetEventTagPin();
	UEdGraphPin* EventStringPin = GetEventStringPin();
	UEdGraphPin* FunctionNamePin = GetFunctionNamePin();

	if (!FunctionNamePin)
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("K2Node", "InvalidPins", "Invalid pins in @@").ToString(), this);
		return;
	}

	const bool bIsDelegate = UGameEventNodeUtils::IsDelegateMode(GetBindTypePin());
	const bool bIsEventString = UGameEventNodeUtils::IsStringEventId(GetEventIdTypePin());

	FName CallFuncName;
	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	if (bIsDelegate)
	{
		if (bIsEventString)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, AddListener_StrKey_ByDelegate);
		}
		else
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, AddListener_ByDelegate);
		}
	}
	else
	{
		if (bIsEventString)
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, AddListener_StrKey_ByFuncName);
		}
		else
		{
			CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, AddListener_ByFuncName);
		}
	}

	if (!CallFuncName.IsNone())
	{
		CallFuncNode->FunctionReference.SetExternalMember(CallFuncName, UGameEventNodeUtils::StaticClass());
		CallFuncNode->AllocateDefaultPins();

		UEdGraphPin* WorldContextObjectParam = CallFuncNode->FindPinChecked(WorldContextObjectParamName);
		UEdGraphPin* EventNameParam = CallFuncNode->FindPinChecked(EventNameParamName);

		if (bIsDelegate)
		{
			HandleDelegateExpansion(CallFuncNode, CompilerContext, SourceGraph);
		}
		else
		{
			UEdGraphPin* FunctionNameParam = CallFuncNode->FindPinChecked(FunctionNameParamName);
			CompilerContext.MovePinLinksToIntermediate(*FunctionNamePin, *FunctionNameParam);
		}

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

void UK2Node_AddListener::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (!Pin || !IsValid(this))
	{
		return;
	}
	// If delegate pin connection changed
	const UEdGraphPin* DelegatePin = GetDelegatePin();
	if (Pin != DelegatePin)
	{
		return;
	}

	UpdateCustomEventSignatureFromDataType();
}

void UK2Node_AddListener::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	// Base class handles event identifier related logic
	Super::PinDefaultValueChanged(Pin);

	// AddListener specific logic: handle binding type and function parameter related pin changes
	if (Pin && (Pin->PinName == FK2Node_AddListenerPinName::EventBindTypePinName ||
	            Pin->PinName == FK2Node_AddListenerPinName::FunctionNamePinName ||
	            Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName ||
	            Pin->PinName == FGameEventBasePinNames::EventTagPinName ||
	            Pin->PinName == FGameEventBasePinNames::EventStringPinName))
	{
		UpdatePinVisibility();

		if (Pin->PinName == FGameEventBasePinNames::EventTagPinName ||
		    Pin->PinName == FGameEventBasePinNames::EventStringPinName ||
		    Pin->PinName == FK2Node_AddListenerPinName::FunctionNamePinName ||
		    Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
		{
			AnalyzeFunctionAndRegisterEventType();
			UpdateCustomEventSignatureFromDataType();
		}
	}
}

void UK2Node_AddListener::PinTypeChanged(UEdGraphPin* Pin)
{
	Super::PinTypeChanged(Pin);

	// When DataType pin type changes, dynamically update Delegate pin's delegate signature
	if (Pin)
	{
		if (Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
		{
			AnalyzeFunctionAndRegisterEventType();
			UpdateCustomEventSignatureFromDataType();
		}

		GetGraph()->NotifyGraphChanged();
	}
}

void UK2Node_AddListener::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	if (!Context || !Context->Node || !UGameEventNodeUtils::IsDelegateMode(GetBindTypePin()))
	{
		return;
	}

	const FName SectionName = TEXT("EdGraphSchemaPinActions");
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);

	if (Context->Pin && Context->Pin->PinName == FK2Node_AddListenerPinName::DelegatePinName)
	{
		if (!FindPin(FK2Node_AddListenerPinName::DataTypePinName))
		{
			Section.AddMenuEntry(
			                     TEXT("AddDataTypePin"),
			                     NSLOCTEXT("K2Node", "AddDataTypePin", "Add Data Type Pin"),
			                     NSLOCTEXT("K2Node", "AddDataTypePinTooltip", "Adds a Data Type pin to specify the delegate parameter type"),
			                     FSlateIcon(),
			                     FExecuteAction::CreateUObject(const_cast<UK2Node_AddListener*>(this), &UK2Node_AddListener::AddDataTypePin)
			                    );
		}
	}
	if (Context->Pin && Context->Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
	{
		Section.AddMenuEntry(TEXT("RemoveDataTypePin"),
		                     NSLOCTEXT("K2Node", "RemoveDataTypePin", "Remove Data Type Pin"),
		                     NSLOCTEXT("K2Node", "RemoveDataTypePinTooltip", "Removes the Data Type pin"),
		                     FSlateIcon(),
		                     FExecuteAction::CreateUObject(const_cast<UK2Node_AddListener*>(this), &UK2Node_AddListener::RemoveDataTypePin)
		                    );
	}
}

#pragma endregion Overrides

void UK2Node_AddListener::UpdatePinVisibility()
{
	Super::UpdatePinVisibility();

	UEdGraphPin* FunctionNamePin = GetFunctionNamePin();
	UEdGraphPin* DelegatePin = GetDelegatePin();
	UEdGraphPin* DataTypePin = GetDataTypePin();

	if (UGameEventNodeUtils::IsDelegateMode(GetBindTypePin()))
	{
		UGameEventNodeUtils::ClearPinValue(FunctionNamePin);
		if (FunctionNamePin)
		{
			FunctionNamePin->bHidden = true;
		}
		if (DelegatePin)
		{
			DelegatePin->bHidden = false;
		}
		if (DataTypePin)
		{
			DataTypePin->bHidden = false;
		}
	}
	else
	{
		UGameEventNodeUtils::ClearPinValue(DataTypePin);
		UGameEventNodeUtils::ClearPinValue(DelegatePin);
		if (FunctionNamePin)
		{
			FunctionNamePin->bHidden = false;
		}
		if (DelegatePin)
		{
			DelegatePin->bHidden = true;
		}
		if (DataTypePin)
		{
			DataTypePin->bHidden = true;
		}
	}
}

void UK2Node_AddListener::AnalyzeFunctionAndRegisterEventType() const
{
	UE_LOG_GAS_EDITOR(TEXT("AddListener: AnalyzeFunctionAndRegisterEventType"));

	const FString EventName = GetCurrentEventName();
	if (EventName.IsEmpty())
	{
		UE_LOG_GAS_WARNING(TEXT("AddListener: Event name is empty, skipping type analysis"));
		return;
	}

	if (UGameEventNodeUtils::IsDelegateMode(GetBindTypePin()))
	{
		if (const UEdGraphPin* DataTypePin = GetDataTypePin())
		{
			const FEventTypeInfo TypeInfo(DataTypePin->PinType);
			FGameEventTypeManager::Get()->RegisterEventType(EventName, TypeInfo);
			return;
		}
		const FEventTypeInfo TypeInfo;
		FGameEventTypeManager::Get()->RegisterEventType(EventName, TypeInfo);
		return;
	}

	const UEdGraphPin* FunctionNamePin = GetFunctionNamePin();
	if (!FunctionNamePin || FunctionNamePin->GetDefaultAsString().IsEmpty())
	{
		UE_LOG_GAS_WARNING(TEXT("AddListener: Function name is empty, skipping type analysis"));
		return;
	}

	const FString FunctionName = FunctionNamePin->GetDefaultAsString();
	UE_LOG_GAS_EDITOR(TEXT("AddListener: Analyzing function '%s' for event '%s'"), *FunctionName, *EventName);

	UClass* TargetClass = nullptr;
	UEdGraphPin* SelfPin = GetSelfPin();

	if (SelfPin && SelfPin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* ConnectedPin = SelfPin->LinkedTo[0];
		if (ConnectedPin && ConnectedPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
		{
			TargetClass = Cast<UClass>(ConnectedPin->PinType.PinSubCategoryObject.Get());
		}

		const UEdGraphPin* UsePin = FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(ConnectedPin);
		if (!TargetClass && UsePin)
		{
			if (const UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(UsePin->GetOwningNode()))
			{
				if (FProperty* Property = VarNode->GetPropertyForVariable())
				{
					if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
					{
						TargetClass = ObjProp->PropertyClass;
					}
				}
			}
		}
	}
	else if (SelfPin && SelfPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		TargetClass = GetBlueprintClassFromNode();
	}

	UE_LOG_GAS_EDITOR(TEXT("AddListener: Final target class: %s, looking for function: %s"), *TargetClass->GetName(), *FunctionName);

	FGameEventTypeManager::Get()->AnalyzeAndRegisterFunctionType(EventName, TargetClass, FunctionName);
}

void UK2Node_AddListener::UpdateCustomEventSignatureFromDataType() const
{
	UE_LOG_GAS_EDITOR(TEXT("AddListener: UpdateCustomEventSignatureFromDataType"));

	if (GEditor && UGameEventNodeUtils::IsDelegateMode(GetBindTypePin()))
	{
		GEditor->GetTimerManager()->SetTimerForNextTick([this]
		{
			const UEdGraphPin* DelegatePin = GetDelegatePin();
			const UEdGraphPin* DataTypePin = GetDataTypePin();
			if (!IsValid(this) || !DelegatePin)
			{
				return;
			}

			for (UEdGraphPin* LinkedPin : DelegatePin->LinkedTo)
			{
				if (!LinkedPin)
				{
					continue;
				}
				const UEdGraphPin* UsePin = FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(LinkedPin);
				if (UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(UsePin->GetOwningNode()))
				{
					if (IsValid(CustomEventNode))
					{
						if (!CustomEventNode)
						{
							return;
						}

						CustomEventNode->UserDefinedPins.Empty();

						if (DataTypePin)
						{
							const TSharedPtr<FUserPinInfo> NewParamPin = MakeShareable(new FUserPinInfo());
							NewParamPin->DesiredPinDirection = EGPD_Output;
							NewParamPin->PinName = TEXT("Value");
							NewParamPin->PinType = DataTypePin->PinType;

							if (DataTypePin->PinType.IsContainer() ||
							    DataTypePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct ||
							    DataTypePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
							    DataTypePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String ||
							    DataTypePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
							{
								NewParamPin->PinType.bIsReference = true;
								NewParamPin->PinType.bIsConst = true;
								NewParamPin->PinType.bIsWeakPointer = false;
							}

							CustomEventNode->UserDefinedPins.Add(NewParamPin);
						}

						CustomEventNode->ReconstructNode();
					}
				}
			}
		});
	}
}

void UK2Node_AddListener::HandleDelegateExpansion(const UK2Node_CallFunction* CallFuncNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	if (!CallFuncNode || !SourceGraph)
	{
		UE_LOG_GAS_WARNING(TEXT("AddListener: HandleDelegateExpansion - CallFuncNode or SourceGraph is null"));
		return;
	}

	static const FName PropertyDelegateParamName(TEXT("PropertyDelegate"));
	UEdGraphPin* DelegateParam = CallFuncNode->FindPinChecked(PropertyDelegateParamName);
	if (!DelegateParam)
	{
		UE_LOG_GAS_WARNING(TEXT("AddListener: HandleDelegateExpansion - DelegateParam is null"));
		return;
	}

	const UEdGraphPin* DelegatePin = GetDelegatePin();
	if (!DelegatePin || DelegatePin->LinkedTo.Num() == 0)
	{
		UE_LOG_GAS_WARNING(TEXT("AddListener: HandleDelegateExpansion - DelegatePin is null or not connected"));
		return;
	}

	static const FName PropertyName = TEXT("Property");

	UK2Node_CustomEvent* WrapperEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);
	WrapperEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *FString("WrapperEvent"), *CompilerContext.GetGuid(WrapperEventNode));
	WrapperEventNode->EventReference.SetExternalMember(WrapperEventNode->CustomFunctionName, GetBlueprintClassFromNode());

	TSharedPtr<FUserPinInfo> WrapperParamPin = MakeShareable(new FUserPinInfo());
	WrapperParamPin->PinName = PropertyName;
	WrapperParamPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
	WrapperParamPin->PinType.PinSubCategoryObject = FEventProperty::StaticStruct();
	WrapperParamPin->PinType.ContainerType = EPinContainerType::None;
	WrapperParamPin->PinType.bIsReference = true;
	WrapperParamPin->PinType.bIsConst = true;
	WrapperParamPin->PinType.bIsWeakPointer = false;
	WrapperParamPin->DesiredPinDirection = EGPD_Output;

	WrapperEventNode->UserDefinedPins.Add(WrapperParamPin);
	WrapperEventNode->AllocateDefaultPins();

	CreateDataConversionNodes(CompilerContext, SourceGraph, WrapperEventNode);

	UEdGraphPin* WrapperDelegatePin = WrapperEventNode->FindPin(UK2Node_Event::DelegateOutputName);
	DelegateParam->MakeLinkTo(WrapperDelegatePin);
}

void UK2Node_AddListener::CreateDataConversionNodes(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const UK2Node_CustomEvent* WrapperEventNode)
{
	const UEdGraphPin* DelegatePin = GetDelegatePin();
	UEdGraphPin* LinkedPin = DelegatePin->LinkedTo[0];
	if (!LinkedPin)
	{
		return;
	}
	const UFunction* FoundFunction = nullptr;
	const UEdGraphPin* UsePin = FBlueprintEditorUtils::FindFirstCompilerRelevantLinkedPin(LinkedPin);
	if (const UK2Node_CustomEvent* CreateCustomEvent = Cast<UK2Node_CustomEvent>(UsePin->GetOwningNode()))
	{
		FoundFunction = CreateCustomEvent->EventReference.ResolveMember<UFunction>();
	}
	else if (const UK2Node_CreateDelegate* CreateDelegateNode = Cast<UK2Node_CreateDelegate>(UsePin->GetOwningNode()))
	{
		FMemberReference MemberReference;
		MemberReference.SetDirect(CreateDelegateNode->SelectedFunctionName, CreateDelegateNode->SelectedFunctionGuid, CreateDelegateNode->GetScopeClass(), false);
		FoundFunction = MemberReference.ResolveMember<UFunction>();
	}

	if (IsValid(FoundFunction))
	{
		UK2Node_CallFunction* DelegateFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		DelegateFunctionNode->SetFromFunction(FoundFunction);
		DelegateFunctionNode->AllocateDefaultPins();

		if (const UFunction* ConvertFunction = GetConvertFunction())
		{
			UK2Node_CallFunction* ConvertFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			ConvertFunctionNode->SetFromFunction(ConvertFunction);
			ConvertFunctionNode->AllocateDefaultPins();

			UEdGraphPin* WrapperEventPropertyPin = WrapperEventNode->FindPinChecked(TEXT("Property"));
			UEdGraphPin* ConvertFunctionInPropPin = ConvertFunctionNode->FindPinChecked(TEXT("InProp"));
			WrapperEventPropertyPin->MakeLinkTo(ConvertFunctionInPropPin);

			UEdGraphPin* DelegateFunctionValuePin = DelegateFunctionNode->FindPinChecked(TEXT("Value"));
			UEdGraphPin* ConvertFunctionOutValuePin = ConvertFunctionNode->FindPinChecked(TEXT("OutValue"));

			ConvertFunctionOutValuePin->PinType = DelegateFunctionValuePin->PinType;
			ConvertFunctionOutValuePin->MakeLinkTo(DelegateFunctionValuePin);
		}

		UEdGraphPin* WrapperEventThenPin = WrapperEventNode->GetThenPin();
		UEdGraphPin* DelegateFunctionExecPin = DelegateFunctionNode->GetExecPin();
		WrapperEventThenPin->MakeLinkTo(DelegateFunctionExecPin);
	}
	else
	{
		CompilerContext.MessageLog.Warning(*NSLOCTEXT("K2Node", "AddListener_DelegateInvalid", "Delegate node is invalid, please recreate! @@").ToString(), this);
	}
}

UFunction* UK2Node_AddListener::GetConvertFunction() const
{
	const UEdGraphPin* DataTypePin = GetDataTypePin();
	if (!DataTypePin)
	{
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - No Parameters !"));
		return nullptr;
	}
	const FName DataTypeCategory = DataTypePin->PinType.PinCategory;

	UFunction* ConvertFunction = nullptr;

	if (DataTypeCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToBoolean));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Byte)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToByte));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Int)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToInt));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Int64)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToInt64));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Real)
	{
		if (DataTypePin->PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
		{
			ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToFloat));
		}
		if (DataTypePin->PinType.PinSubCategory == UEdGraphSchema_K2::PC_Double)
		{
			ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToDouble));
		}
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Name)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToName));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_String)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToString));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Text)
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToText));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Struct)
	{
		if (const UScriptStruct* Struct = Cast<UScriptStruct>(DataTypePin->PinType.PinSubCategoryObject.Get()))
		{
			const FString DataTypeSubCategory = Struct->GetName();
			UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is struct: %s"), *DataTypeSubCategory);
		}

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToStruct));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Object)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is object: %s"), *DataTypeSubCategory);

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToObject));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_SoftObject)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is object: %s"), *DataTypeSubCategory);

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToSoftObject));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Class)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is object: %s"), *DataTypeSubCategory);

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToClass));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_SoftClass)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is object: %s"), *DataTypeSubCategory);

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToSoftClass));
	}

	const bool bIsArray = DataTypePin->PinType.IsArray();
	const bool bIsSet = DataTypePin->PinType.IsSet();
	const bool bIsMap = DataTypePin->PinType.IsMap();

	UClass* BlueprintTypeConversionsClass = UBlueprintTypeConversions::StaticClass();

	UFunction* ArrayConversionFunction = BlueprintTypeConversionsClass->FindFunctionByName(TEXT("ConvertArrayType"));
	check(ArrayConversionFunction);

	UFunction* SetConversionFunction = BlueprintTypeConversionsClass->FindFunctionByName(TEXT("ConvertSetType"));
	check(SetConversionFunction);

	UFunction* MapConversionFunction = BlueprintTypeConversionsClass->FindFunctionByName(TEXT("ConvertMapType"));
	check(MapConversionFunction);

	UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType: %s"), *DataTypeCategory.ToString());

	if (bIsArray)
	{
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is Array"));
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertArrayType));
	}
	if (bIsSet)
	{
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is Set"));
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertSetType));
	}
	if (bIsMap)
	{
		UE_LOG_GAS_EDITOR(TEXT("AddListener: GetConvertFunction - DataType is Map"));
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertMapType));
	}

	return ConvertFunction;
}

#pragma region PinAccessors

void UK2Node_AddListener::AddDataTypePin()
{
	UEdGraphPin* DataTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FK2Node_AddListenerPinName::DataTypePinName);
	DataTypePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_DataType_Tooltip", "Parameter type for delegate binding").ToString();
	DataTypePin->bNotConnectable = true;
	DataTypePin->PinType.bIsReference = true;

	UpdatePinVisibility();
	GetGraph()->NotifyGraphChanged();
}

void UK2Node_AddListener::RemoveDataTypePin()
{
	if (UEdGraphPin* DataTypePin = FindPin(FK2Node_AddListenerPinName::DataTypePinName))
	{
		DataTypePin->BreakAllPinLinks();
		Pins.Remove(DataTypePin);
		DestroyPin(DataTypePin);
	}

	UpdatePinVisibility();
	GetGraph()->NotifyGraphChanged();
}

UEdGraphPin* UK2Node_AddListener::GetDataTypePin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_AddListenerPinName::DataTypePinName);
	return Pin;
}

UEdGraphPin* UK2Node_AddListener::GetSelfPin() const
{
	UEdGraphPin* Pin = FindPin(UEdGraphSchema_K2::PN_Self);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_AddListener::GetFunctionNamePin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_AddListenerPinName::FunctionNamePinName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_AddListener::GetBindTypePin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_AddListenerPinName::EventBindTypePinName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_AddListener::GetDelegatePin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_AddListenerPinName::DelegatePinName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}
#pragma endregion PinAccessors

#undef LOCTEXT_NAMESPACE
