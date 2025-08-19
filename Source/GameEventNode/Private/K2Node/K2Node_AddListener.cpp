#include "K2Node/K2Node_AddListener.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "GameEventNodeTypes.h"
#include "K2Node_Variable.h"
#include "GameEventNodeLog.h"
#include "GameEventNodeUtils.h"
#include "GameEventTypeManager.h"
#include "K2Node_CreateDelegate.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallArrayFunction.h"
#include "Kismet/KismetArrayLibrary.h"

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
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

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

	// Create delegate pin
	UEdGraphPin* DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, FK2Node_AddListenerPinName::DelegatePinName);
	DelegatePin->PinToolTip = NSLOCTEXT("K2Node", "AddListener_Delegate_Tooltip", "Delegate to bind").ToString();
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
	return FText::Format(NSLOCTEXT("K2Node", "AddListener_Keywords", "{0} addevent add listener addlistener bind subscribe register hook attach"),
	                     Super::GetKeywords());
}

void UK2Node_AddListener::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	//Super::ReallocatePinsDuringReconstruction(OldPins);
	AllocateDefaultPins();

	for (UEdGraphPin* OldPin : OldPins)
	{
		bool bIsDataTypePin = false;

		const FString PinNameStr = OldPin->PinName.ToString();
		const FString PrefixStr = FK2Node_AddListenerPinName::DataTypePinName.ToString();
		if (PinNameStr == PrefixStr || PinNameStr.StartsWith(PrefixStr))
		{
			bIsDataTypePin = true;
		}
		if (bIsDataTypePin)
		{
			UEdGraphPin* NewPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, OldPin->PinName);
			NewPin->PinType = OldPin->PinType;
			NewPin->DefaultObject = OldPin->DefaultObject;
			NewPin->DefaultTextValue = OldPin->DefaultTextValue;
			NewPin->DefaultValue = OldPin->DefaultValue;
			NewPin->bNotConnectable = true;
		}
	}

	RestoreSplitPins(OldPins);
}

void UK2Node_AddListener::PostReconstructNode()
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	Super::PostReconstructNode();

	UpdateEventSignature();
}

void UK2Node_AddListener::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName WorldContextObjectParamName(TEXT("WorldContextObject"));
	static const FName EventNameParamName(TEXT("EventName"));
	static const FName FunctionNameParamName(TEXT("FunctionName"));

	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ThenPin = GetThenPin();

	UEdGraphPin* SelfPin = GetSelfPin();
	UEdGraphPin* FunctionNamePin = GetFunctionNamePin();

	if (!FunctionNamePin)
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("K2Node", "InvalidPins", "Invalid pins in @@").ToString(), this);
		return;
	}

	const bool bIsDelegate = UGameEventNodeUtils::IsDelegateMode(GetBindTypePin());

	FName CallFuncName;
	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	if (bIsDelegate)
	{
		CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, AddListener_ByDelegate);
	}
	else
	{
		CallFuncName = GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, AddListener_ByFuncName);
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

		ConnectEventNameWithTagConversion(CompilerContext, SourceGraph, EventNameParam);

		CompilerContext.MovePinLinksToIntermediate(*SelfPin, *WorldContextObjectParam);

		CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallFuncNode->GetExecPin());
		CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFuncNode->GetThenPin());
	}

	BreakAllNodeLinks();
}

void UK2Node_AddListener::PinConnectionListChanged(UEdGraphPin* Pin)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

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
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	// Base class handles event identifier related logic
	Super::PinDefaultValueChanged(Pin);

	bool bIsDataTypePin = false;
	if (Pin)
	{
		if (Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
		{
			bIsDataTypePin = true;
		}
		else
		{
			const FString PinNameStr = Pin->PinName.ToString();
			const FString PrefixStr = FK2Node_AddListenerPinName::DataTypePinName.ToString();
			if (PinNameStr.StartsWith(PrefixStr) && PinNameStr.Len() > PrefixStr.Len())
			{
				const FString NumberPart = PinNameStr.Mid(PrefixStr.Len());
				if (NumberPart.IsNumeric())
				{
					bIsDataTypePin = true;
				}
			}
		}
	}

	if (Pin->PinName == FGameEventBasePinNames::EventTagPinName ||
	    Pin->PinName == FGameEventBasePinNames::EventStringPinName ||
	    Pin->PinName == FK2Node_AddListenerPinName::FunctionNamePinName ||
	    bIsDataTypePin)
	{
		UpdateEventSignature();
	}
}

void UK2Node_AddListener::PinTypeChanged(UEdGraphPin* Pin)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	Super::PinTypeChanged(Pin);

	bool bIsDataTypePin = false;
	if (Pin)
	{
		if (Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
		{
			bIsDataTypePin = true;
		}
		else
		{
			const FString PinNameStr = Pin->PinName.ToString();
			const FString PrefixStr = FK2Node_AddListenerPinName::DataTypePinName.ToString();
			if (PinNameStr.StartsWith(PrefixStr) && PinNameStr.Len() > PrefixStr.Len())
			{
				const FString NumberPart = PinNameStr.Mid(PrefixStr.Len());
				if (NumberPart.IsNumeric())
				{
					bIsDataTypePin = true;
				}
			}
		}
	}

	// When DataType pin type changes, dynamically update Delegate pin's delegate signature
	if (Pin && bIsDataTypePin)
	{
		UpdateEventSignature();
	}

	if (Pin)
	{
		GetGraph()->NotifyNodeChanged(this);
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

	bool bIsShowMenu = false;

	if (Context->Pin)
	{
		if (Context->Pin->PinName == FK2Node_AddListenerPinName::DelegatePinName || Context->Pin->PinName == FK2Node_AddListenerPinName::DataTypePinName)
		{
			bIsShowMenu = true;
		}
		else
		{
			const FString PinNameStr = Context->Pin->PinName.ToString();
			const FString PrefixStr = FK2Node_AddListenerPinName::DataTypePinName.ToString();
			if (PinNameStr.StartsWith(PrefixStr) && PinNameStr.Len() > PrefixStr.Len())
			{
				const FString NumberPart = PinNameStr.Mid(PrefixStr.Len());
				if (NumberPart.IsNumeric())
				{
					bIsShowMenu = true;
				}
			}
		}
	}

	if (bIsShowMenu)
	{
		if (GetDataTypePinCount() < GlobalConfig.MaxParameterNum)
		{
			Section.AddMenuEntry(
			                     TEXT("AddDataTypePin"),
			                     NSLOCTEXT("K2Node", "AddDataTypePin", "Add Data Type Pin"),
			                     NSLOCTEXT("K2Node", "AddDataTypePinTooltip", "Adds a Data Type pin to specify the delegate parameter type"),
			                     FSlateIcon(),
			                     FExecuteAction::CreateUObject(const_cast<UK2Node_AddListener*>(this), &UK2Node_AddListener::AddDataTypePinAtIndex, -1)
			                    );
		}
		if (GetDataTypePinCount() >= 1)
		{
			Section.AddMenuEntry(
			                     TEXT("RemoveDataTypePin"),
			                     NSLOCTEXT("K2Node", "RemoveDataTypePin", "Remove Last Data Type Pin"),
			                     NSLOCTEXT("K2Node", "RemoveDataTypePinTooltip", "Removes the last Data Type pin"),
			                     FSlateIcon(),
			                     FExecuteAction::CreateUObject(const_cast<UK2Node_AddListener*>(this), &UK2Node_AddListener::RemoveDataTypePinAtIndex, -1)
			                    );
		}
	}
}

#pragma endregion Overrides

bool UK2Node_AddListener::CheckUpdatePinCondition(const UEdGraphPin* Pin) const
{
	return Super::CheckUpdatePinCondition(Pin) || Pin->PinName == FK2Node_AddListenerPinName::EventBindTypePinName;
}

void UK2Node_AddListener::UpdatePinVisibility()
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	const bool bIsDelegate = UGameEventNodeUtils::IsDelegateMode(GetBindTypePin());
	if (UEdGraphPin* FunctionNamePin = GetFunctionNamePin())
	{
		UGameEventNodeUtils::ClearPinValue(FunctionNamePin);
		FunctionNamePin->bHidden = bIsDelegate;
	}
	if (UEdGraphPin* DelegatePin = GetDelegatePin())
	{
		DelegatePin->bHidden = !bIsDelegate;
	}

	if (!bIsDelegate)
	{
		TArray<UEdGraphPin*> DataTypePins = GetAllDataTypePins();
		for (UEdGraphPin* DataTypePin : DataTypePins)
		{
			if (DataTypePin)
			{
				DataTypePin->bHidden = !bIsDelegate;
				Pins.Remove(DataTypePin);
				DestroyPin(DataTypePin);
			}
		}
	}

	Super::UpdatePinVisibility();
}

void UK2Node_AddListener::UpdateEventSignature() const
{
	AnalyzeFunctionAndRegisterEventType();
	UpdateCustomEventSignatureFromDataType();
}

void UK2Node_AddListener::AnalyzeFunctionAndRegisterEventType() const
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	const FString EventName = GetCurrentEventName();
	if (EventName.IsEmpty())
	{
		UE_LOG_GAS_WARNING(TEXT("AddListener: Event name is empty, skipping type analysis"));
		return;
	}

	if (UGameEventNodeUtils::IsDelegateMode(GetBindTypePin()))
	{
		TArray<UEdGraphPin*> DataTypePins = GetAllDataTypePins();
		if (DataTypePins.Num() > 0)
		{
			TArray<FEventParameterInfo> Parameters;
			for (const UEdGraphPin* DataTypePin : DataTypePins)
			{
				if (DataTypePin && DataTypePin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					Parameters.Add(FEventParameterInfo(DataTypePin->PinType));
				}
			}

			if (Parameters.Num() > 0)
			{
				const FEventTypeInfo TypeInfo(Parameters);
				FGameEventTypeManager::Get()->RegisterEventType(EventName, TypeInfo);
				return;
			}
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
	if (UE::GetIsEditorLoadingPackage() || !GIsEditor || IsTemplate() || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	if (GEditor && UGameEventNodeUtils::IsDelegateMode(GetBindTypePin()))
	{
		GEditor->GetTimerManager()->SetTimerForNextTick([this]
		{
			const UEdGraphPin* DelegatePin = GetDelegatePin();
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
				if (UsePin == nullptr)
				{
					continue;
				}
				if (UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(UsePin->GetOwningNode()))
				{
					if (IsValid(CustomEventNode))
					{
						if (!CustomEventNode)
						{
							return;
						}

						TArray<UEdGraphPin*> DataTypePins = GetAllDataTypePins();

						bool bNeedsUpdate = false;
						const int32 DataTypePinCount = DataTypePins.Num();

						if (CustomEventNode->UserDefinedPins.Num() != DataTypePinCount)
						{
							bNeedsUpdate = true;
						}
						else
						{
							for (int32 i = 0; i < CustomEventNode->UserDefinedPins.Num(); ++i)
							{
								if (i < DataTypePinCount)
								{
									const UEdGraphPin* DataTypePin = DataTypePins[i];
									const TSharedPtr<FUserPinInfo>& ExistingPin = CustomEventNode->UserDefinedPins[i];
									if (ExistingPin->PinType != DataTypePin->PinType)
									{
										bNeedsUpdate = true;
										break;
									}
								}
							}
						}

						if (bNeedsUpdate)
						{
							CustomEventNode->UserDefinedPins.Empty();
							for (int32 i = 0; i < DataTypePins.Num(); ++i)
							{
								const UEdGraphPin* DataTypePin = DataTypePins[i];
								if (DataTypePin && DataTypePin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
								{
									const TSharedPtr<FUserPinInfo> NewParamPin = MakeShareable(new FUserPinInfo());
									NewParamPin->DesiredPinDirection = EGPD_Output;

									NewParamPin->PinName = i == 0 ? TEXT("Value") : *FString::Printf(TEXT("Value%d"), i);
									NewParamPin->PinType = DataTypePin->PinType;
									CustomEventNode->UserDefinedPins.Add(NewParamPin);
								}
							}

							CustomEventNode->ReconstructNode();
						}
					}
				}
			}
		});
	}
}

void UK2Node_AddListener::HandleDelegateExpansion(const UK2Node_CallFunction* CallFuncNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

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
	WrapperParamPin->PinType.PinSubCategoryObject = FPropertyContext::StaticStruct();
	WrapperParamPin->PinType.ContainerType = EPinContainerType::Array;
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
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

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

		TArray<UEdGraphPin*> DataTypePins = GetAllDataTypePins();
		for (int32 i = 0; i < DataTypePins.Num(); ++i)
		{
			const UEdGraphPin* DataTypePin = DataTypePins[i];
			if (const UFunction* ConvertFunction = UGameEventNodeUtils::GetConvertFunction(DataTypePin))
			{
				UK2Node_CallFunction* ConvertFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				ConvertFunctionNode->SetFromFunction(ConvertFunction);
				ConvertFunctionNode->AllocateDefaultPins();

				UEdGraphPin* WrapperEventPropertyPin = WrapperEventNode->FindPinChecked(TEXT("Property"));

				if (WrapperEventPropertyPin->PinType.ContainerType == EPinContainerType::Array)
				{
					UK2Node_CallArrayFunction* ArrayGetNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);

					ArrayGetNode->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get)));
					ArrayGetNode->AllocateDefaultPins();

					UEdGraphPin* ArrayPin = ArrayGetNode->FindPinChecked(TEXT("TargetArray"));
					WrapperEventPropertyPin->MakeLinkTo(ArrayPin);
					ArrayPin->PinType = WrapperEventPropertyPin->PinType;

					UEdGraphPin* ArrayIndexPin = ArrayGetNode->FindPinChecked(TEXT("Index"));
					ArrayIndexPin->DefaultValue = FString::Printf(TEXT("%d"), i);

					UEdGraphPin* ArrayItemPin = ArrayGetNode->FindPinChecked(TEXT("Item"));
					ArrayItemPin->PinType = WrapperEventPropertyPin->PinType;
					ArrayItemPin->PinType.ContainerType = EPinContainerType::None;

					UEdGraphPin* ConvertFunctionInPropPin = ConvertFunctionNode->FindPinChecked(TEXT("InProp"));
					ArrayItemPin->MakeLinkTo(ConvertFunctionInPropPin);

					FString ParamName = i == 0 ? TEXT("Value") : FString::Printf(TEXT("Value%d"), i);
					UEdGraphPin* DelegateFunctionValuePin = DelegateFunctionNode->FindPinChecked(ParamName);

					UEdGraphPin* ConvertFunctionOutValuePin = ConvertFunctionNode->FindPinChecked(TEXT("OutValue"));
					ConvertFunctionOutValuePin->PinType = DelegateFunctionValuePin->PinType;
					ConvertFunctionOutValuePin->MakeLinkTo(DelegateFunctionValuePin);
				}
			}
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

void UK2Node_AddListener::AddDataTypePinAtIndex(const int32 Index)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());

	const int32 CurrentCount = GetDataTypePinCount();
	const int32 TargetIndex = (Index < 0) ? CurrentCount : FMath::Clamp(Index, 0, CurrentCount);

	const FName PinName = UGameEventNodeUtils::GetMultiParameterPinName(FK2Node_AddListenerPinName::DataTypePinName, TargetIndex);
	UEdGraphPin* DataTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, PinName);
	DataTypePin->bNotConnectable = true;

	UpdateEventSignature();
	GetGraph()->NotifyNodeChanged(this);
}

void UK2Node_AddListener::RemoveDataTypePinAtIndex(const int32 Index)
{
	GAME_SCOPED_TRACK_LOG_AUTO_BY_NAME(GetBlueprint()->GetName());
	TArray<UEdGraphPin*> DataTypePins = GetAllDataTypePins();
	if (DataTypePins.Num() > 0 && Index < DataTypePins.Num())
	{
		UEdGraphPin* LastPin = (Index < 0) ? DataTypePins.Last() : DataTypePins[Index];
		Pins.Remove(LastPin);
		DestroyPin(LastPin);
	}

	UpdateEventSignature();
	GetGraph()->NotifyNodeChanged(this);
}

TArray<UEdGraphPin*> UK2Node_AddListener::GetAllDataTypePins() const
{
	TArray<UEdGraphPin*> DataTypePins;
	for (int32 i = 0; i < GlobalConfig.MaxParameterNum; ++i)
	{
		const FName PinName = UGameEventNodeUtils::GetMultiParameterPinName(FK2Node_AddListenerPinName::DataTypePinName, i);
		if (UEdGraphPin* Pin = FindPin(PinName))
		{
			DataTypePins.Add(Pin);
		}
		else
		{
			break;
		}
	}
	return DataTypePins;
}

UEdGraphPin* UK2Node_AddListener::GetDataTypePinByIndex(const int32 Index) const
{
	const FName PinName = UGameEventNodeUtils::GetMultiParameterPinName(FK2Node_AddListenerPinName::DataTypePinName, Index);
	return FindPin(PinName);
}

int32 UK2Node_AddListener::GetDataTypePinCount() const
{
	return GetAllDataTypePins().Num();
}

#pragma region PinAccessors

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

#pragma endregion

#undef LOCTEXT_NAMESPACE
