#include "GameEventNodeUtils.h"
#include "GameEventManager.h"

bool UGameEventNodeUtils::IsDelegateMode(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return false;
	}

	static const UEnum* EventBindTypeEnum = StaticEnum<EEventBindType>();
	const FString BindTypeValue = Pin->GetDefaultAsString();
	const int32 BindTypeEnumIndex = EventBindTypeEnum->GetIndexByNameString(BindTypeValue);
	return BindTypeEnumIndex != INDEX_NONE && EventBindTypeEnum->GetValueByIndex(BindTypeEnumIndex) == static_cast<int64>(EEventBindType::Delegate);
}

bool UGameEventNodeUtils::IsStringEventId(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return false;
	}
	static const UEnum* EventIdTypeEnum = StaticEnum<EEventIdType>();
	const FString EventIdTypeValue = Pin->GetDefaultAsString();
	const int32 EventIdTypeIndex = EventIdTypeEnum->GetIndexByNameString(EventIdTypeValue);
	return EventIdTypeIndex != INDEX_NONE && EventIdTypeEnum->GetValueByIndex(EventIdTypeIndex) == static_cast<int64>(EEventIdType::StringBased);
}

void UGameEventNodeUtils::ClearPinValue(UEdGraphPin* Pin)
{
	if (Pin)
	{
		Pin->DefaultValue = TEXT("");
		Pin->DefaultObject = nullptr;
		Pin->DefaultTextValue = FText::GetEmpty();
		Pin->bHidden = true;
		if (Pin->LinkedTo.Num() > 0)
		{
			Pin->BreakAllPinLinks();
		}
	}
}

FString UGameEventNodeUtils::GetEventName(const UEdGraphPin* Pin)
{
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
	{
		return Pin->GetDefaultAsString();
	}
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject == TBaseStructure<FGameplayTag>::Get())
	{
		FString TagValue = Pin->GetDefaultAsString();

		if (TagValue.IsEmpty() || TagValue == TEXT("None") || TagValue == TEXT("(TagName=\"\")"))
		{
			return FString();
		}

		// The format is usually : (TagName="YourTag.Name")
		const int32 StartQuote = TagValue.Find(TEXT("\""));
		const int32 EndQuote = TagValue.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		if (StartQuote != INDEX_NONE && EndQuote != INDEX_NONE && EndQuote > StartQuote)
		{
			FString ExtractedTag = TagValue.Mid(StartQuote + 1, EndQuote - StartQuote - 1);
			if (!ExtractedTag.IsEmpty())
			{
				const FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(FName(*ExtractedTag));
				if (GameplayTag.IsValid())
				{
					return FEventId::TagToEventName(GameplayTag);
				}
				return ExtractedTag;
			}
		}

		if (!TagValue.IsEmpty() && TagValue != TEXT("None"))
		{
			const FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(FName(*TagValue));
			if (GameplayTag.IsValid())
			{
				return FEventId::TagToEventName(GameplayTag);
			}
			return TagValue;
		}
	}
	return FString();
}

FString UGameEventNodeUtils::GetCurrentEventName(const UEdGraphPin* EventIdTypePin, const UEdGraphPin* EventTagPin, const UEdGraphPin* EventStringPin)
{
	if (!EventIdTypePin)
	{
		return FString();
	}

	if (IsStringEventId(EventIdTypePin))
	{
		if (EventStringPin)
		{
			return EventStringPin->GetDefaultAsString();
		}
	}
	else
	{
		if (EventTagPin)
		{
			return GetEventName(const_cast<UEdGraphPin*>(EventTagPin));
		}
	}

	return FString();
}

FName UGameEventNodeUtils::GetMultiParameterPinName(const FName PinName, const int32 Index)
{
	return Index == 0 ? PinName : FName(*FString::Printf(TEXT("%s%d"), *PinName.ToString(), Index));
}

UFunction* UGameEventNodeUtils::GetConvertFunction(const UEdGraphPin* DataTypePin)
{
	if (!DataTypePin)
	{
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
		}

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToStruct));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Object)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToObject));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_SoftObject)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToSoftObject));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_Class)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();

		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToClass));
	}
	if (DataTypeCategory == UEdGraphSchema_K2::PC_SoftClass)
	{
		const FString DataTypeSubCategory = DataTypePin->PinType.PinSubCategoryObject->GetName();
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertToSoftClass));
	}

	if (DataTypePin->PinType.IsArray())
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertArrayType));
	}
	if (DataTypePin->PinType.IsSet())
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertSetType));
	}
	if (DataTypePin->PinType.IsMap())
	{
		ConvertFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, ConvertMapType));
	}

	return ConvertFunction;
}

void UGameEventNodeUtils::AddListener_ByFuncName(UObject* WorldContextObject, const FString EventName, const FString& FunctionName)
{
	FListenerContext Listener;
	Listener.Receiver = WorldContextObject;
	Listener.FunctionName = FunctionName;

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->AddListener(FEventId(EventName), Listener);
	}
}

void UGameEventNodeUtils::AddListener_ByDelegate(UObject* WorldContextObject, const FString EventName, const FEventPropertyDelegate& PropertyDelegate)
{
	if (!IsValid(WorldContextObject) || EventName.IsEmpty())
	{
		return;
	}

	FListenerContext Listener;
	Listener.Receiver = WorldContextObject;
	Listener.PropertyDelegate = PropertyDelegate;

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->AddListener(FEventId(EventName), Listener);
	}
}

void UGameEventNodeUtils::RemoveListener(UObject* WorldContextObject, const FString EventName)
{
	if (!IsValid(WorldContextObject) || EventName.IsEmpty())
	{
		return;
	}

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->RemoveAllListenersForReceiver(FEventId(EventName), WorldContextObject);
	}
}

void UGameEventNodeUtils::SendEvent(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEvent)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	Stack.Step(Stack.Object, nullptr);

	if (Stack.MostRecentProperty != nullptr)
	{
		EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
	}
	else
	{
		EventContext.AddPropertyContext(nullptr, nullptr);
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventTwoParam(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventTwoParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 2; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventThreeParam(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventThreeParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 3; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventFourParam(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventFourParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 4; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventFiveParam(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3, const int32& ParamData4)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventFiveParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 5; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventSixParam(UObject* WorldContextObject,
                                            const FString EventName,
                                            const bool bPinned,
                                            const int32& ParamData,
                                            const int32& ParamData1,
                                            const int32& ParamData2,
                                            const int32& ParamData3,
                                            const int32& ParamData4,
                                            const int32& ParamData5)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventSixParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 6; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventSevenParam(UObject* WorldContextObject,
                                              const FString EventName,
                                              const bool bPinned,
                                              const int32& ParamData,
                                              const int32& ParamData1,
                                              const int32& ParamData2,
                                              const int32& ParamData3,
                                              const int32& ParamData4,
                                              const int32& ParamData5,
                                              const int32& ParamData6)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventSevenParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 7; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEventEightParam(UObject* WorldContextObject,
                                              const FString EventName,
                                              const bool bPinned,
                                              const int32& ParamData,
                                              const int32& ParamData1,
                                              const int32& ParamData2,
                                              const int32& ParamData3,
                                              const int32& ParamData4,
                                              const int32& ParamData5,
                                              const int32& ParamData6,
                                              const int32& ParamData7)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEventEightParam)
{
	Stack.MostRecentProperty = nullptr;
	FEventContext EventContext;

	Stack.StepCompiledIn<FObjectProperty>(&EventContext.WorldContext);
	Stack.StepCompiledIn<FProperty>(&EventContext.EventId.Key);
	Stack.StepCompiledIn<FBoolProperty>(&EventContext.bPinned);

	for (int32 i = 0; i < 8; ++i)
	{
		Stack.Step(Stack.Object, nullptr);
		if (Stack.MostRecentProperty != nullptr)
		{
			EventContext.AddPropertyContext(CastField<FProperty>(Stack.MostRecentProperty), Stack.MostRecentPropertyAddress);
		}
		else
		{
			EventContext.AddPropertyContext(nullptr, nullptr);
		}
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEvent_NoParam(UObject* WorldContextObject, const FString EventName, const bool bPinned)
{
	if (!IsValid(WorldContextObject) || EventName.IsEmpty())
	{
		return;
	}
	const FEventContextBase EventContext(EventName, WorldContextObject, bPinned);

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->SendEvent(EventContext);
	}
}

void UGameEventNodeUtils::UnpinEvent(UObject* WorldContextObject, const FString EventName)
{
	if (!IsValid(WorldContextObject) || EventName.IsEmpty())
	{
		return;
	}

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->UnpinEvent(FEventId(EventName));
	}
}

bool UGameEventNodeUtils::HasEvent(UObject* WorldContextObject, const FString EventName)
{
	if (!IsValid(WorldContextObject) || EventName.IsEmpty())
	{
		return false;
	}

	if (FGameEventManager::Get().IsValid())
	{
		return FGameEventManager::Get()->HasEvent(FEventId(EventName));
	}

	return false;
}

int32 UGameEventNodeUtils::GetEventListenerCount(UObject* WorldContextObject, const FString EventName)
{
	if (!IsValid(WorldContextObject) || EventName.IsEmpty())
	{
		return 0;
	}

	if (FGameEventManager::Get().IsValid())
	{
		return FGameEventManager::Get()->GetEventListenerCount(FEventId(EventName));
	}

	return 0;
}

void UGameEventNodeUtils::RemoveAllListenersForReceiver(UObject* Receiver)
{
	if (!IsValid(Receiver))
	{
		return;
	}

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->RemoveAllListenersForReceiver(Receiver);
	}
}

#pragma region "Convert"

FString UGameEventNodeUtils::TagToEventName(const FGameplayTag InTag)
{
	return FEventId::TagToEventName(InTag);
}

bool UGameEventNodeUtils::ConvertToBoolean(const FPropertyContext& InProp, bool& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FBoolProperty* Property = CastField<FBoolProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToInt(const FPropertyContext& InProp, int32& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FIntProperty* Property = CastField<FIntProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToInt64(const FPropertyContext& InProp, int64& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FInt64Property* Property = CastField<FInt64Property>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToFloat(const FPropertyContext& InProp, float& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FFloatProperty* Property = CastField<FFloatProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToDouble(const FPropertyContext& InProp, double& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FDoubleProperty* Property = CastField<FDoubleProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToByte(const FPropertyContext& InProp, uint8& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FByteProperty* Property = CastField<FByteProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToString(const FPropertyContext& InProp, FString& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FStrProperty* Property = CastField<FStrProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToText(const FPropertyContext& InProp, FText& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FTextProperty* Property = CastField<FTextProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToName(const FPropertyContext& InProp, FName& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FNameProperty* Property = CastField<FNameProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToStruct(const FPropertyContext& InProp, TFieldPath<FProperty>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertToStruct)
{
	P_GET_STRUCT_REF(FPropertyContext, InProp);

	Stack.StepCompiledIn<FProperty>(nullptr);
	void* OutStructPtr = Stack.MostRecentPropertyAddress;
	FProperty* OutStructProperty = Stack.MostRecentProperty;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!OutStructProperty || !OutStructPtr || !InProp.IsValid())
	{
		return;
	}

	const FStructProperty* OutStructProp = CastField<FStructProperty>(OutStructProperty);
	if (!OutStructProp)
	{
		return;
	}

	const FStructProperty* InStructProp = CastField<FStructProperty>(InProp.Property.Get());
	if (InStructProp && InStructProp->Struct == OutStructProp->Struct)
	{
		OutStructProp->Struct->CopyScriptStruct(OutStructPtr, InProp.PropertyPtr);
		*static_cast<bool*>(RESULT_PARAM) = true;
	}
}

bool UGameEventNodeUtils::ConvertToObject(const FPropertyContext& InProp, UObject*& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FObjectProperty* Property = CastField<FObjectProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetObjectPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToSoftObject(const FPropertyContext& InProp, TSoftObjectPtr<>& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FSoftObjectProperty* Property = CastField<FSoftObjectProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetObjectPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToClass(const FPropertyContext& InProp, UClass*& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FClassProperty* Property = CastField<FClassProperty>(InProp.Property.Get()))
		{
			OutValue = Cast<UClass>(Property->GetObjectPropertyValue(InProp.PropertyPtr));
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToSoftClass(const FPropertyContext& InProp, TSoftClassPtr<>& OutValue)
{
	if (InProp.IsValid())
	{
		if (const FSoftClassProperty* Property = CastField<FSoftClassProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetObjectPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertArrayType(const FPropertyContext& InProp, TArray<uint8>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertArrayType)
{
	P_GET_STRUCT_REF(FPropertyContext, InProp);

	Stack.StepCompiledIn<FProperty>(nullptr);
	void* OutArrayPtr = Stack.MostRecentPropertyAddress;
	FProperty* OutArrayProperty = Stack.MostRecentProperty;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!InProp.IsValid() || !OutArrayProperty || !OutArrayPtr)
	{
		return;
	}

	const FArrayProperty* InArrayProp = CastField<FArrayProperty>(InProp.Property.Get());
	const FArrayProperty* OutArrayProp = CastField<FArrayProperty>(OutArrayProperty);

	if (!InArrayProp || !OutArrayProp)
	{
		return;
	}

	FProperty* InInnerProp = InArrayProp->Inner;
	FProperty* OutInnerProp = OutArrayProp->Inner;

	if (!InInnerProp->SameType(OutInnerProp))
	{
		return;
	}

	FScriptArrayHelper InHelper(InArrayProp, InProp.PropertyPtr);
	FScriptArrayHelper OutHelper(OutArrayProp, OutArrayPtr);

	int32 NumElements = InHelper.Num();
	OutHelper.Resize(NumElements);

	for (int32 i = 0; i < NumElements; ++i)
	{
		void* InElementPtr = InHelper.GetRawPtr(i);
		void* OutElementPtr = OutHelper.GetRawPtr(i);
		OutInnerProp->CopySingleValue(OutElementPtr, InElementPtr);
	}

	*static_cast<bool*>(RESULT_PARAM) = true;
}

bool UGameEventNodeUtils::ConvertSetType(const FPropertyContext& InProp, TSet<uint8>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertSetType)
{
	P_GET_STRUCT_REF(FPropertyContext, InProp);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* OutProperty = Stack.MostRecentProperty;
	void* OutPropertyPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!InProp.IsValid() || !OutProperty)
	{
		return;
	}

	const FSetProperty* InSetProp = CastField<FSetProperty>(InProp.Property.Get());
	const FSetProperty* OutSetProp = CastField<FSetProperty>(OutProperty);

	if (!InSetProp || !OutSetProp)
	{
		return;
	}

	FProperty* InElementProp = InSetProp->ElementProp;
	FProperty* OutElementProp = OutSetProp->ElementProp;

	if (!InElementProp->SameType(OutElementProp))
	{
		return;
	}

	FScriptSetHelper InHelper(InSetProp, InProp.PropertyPtr);
	FScriptSetHelper OutHelper(OutSetProp, OutPropertyPtr);

	OutHelper.EmptyElements();

	for (FScriptSetHelper::FIterator It(InHelper); It; ++It)
	{
		void* ElementPtr = InHelper.GetElementPtr(It);
		OutHelper.AddDefaultValue_Invalid_NeedsRehash();
		int32 NewIndex = OutHelper.Num() - 1;
		void* NewElementPtr = OutHelper.GetElementPtr(NewIndex);
		OutElementProp->CopySingleValue(NewElementPtr, ElementPtr);
	}

	OutHelper.Rehash();

	*static_cast<bool*>(RESULT_PARAM) = true;
}

bool UGameEventNodeUtils::ConvertMapType(const FPropertyContext& InProp, TMap<uint8, uint8>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertMapType)
{
	P_GET_STRUCT_REF(FPropertyContext, InProp);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* OutProperty = Stack.MostRecentProperty;
	void* OutPropertyPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!InProp.IsValid() || !OutProperty)
	{
		return;
	}

	const FMapProperty* InMapProp = CastField<FMapProperty>(InProp.Property.Get());
	const FMapProperty* OutMapProp = CastField<FMapProperty>(OutProperty);

	if (!InMapProp || !OutMapProp)
	{
		return;
	}

	FProperty* InKeyProp = InMapProp->KeyProp;
	FProperty* InValueProp = InMapProp->ValueProp;
	FProperty* OutKeyProp = OutMapProp->KeyProp;
	FProperty* OutValueProp = OutMapProp->ValueProp;

	if (!InKeyProp->SameType(OutKeyProp) || !InValueProp->SameType(OutValueProp))
	{
		return;
	}

	FScriptMapHelper InHelper(InMapProp, InProp.PropertyPtr);
	FScriptMapHelper OutHelper(OutMapProp, OutPropertyPtr);

	OutHelper.EmptyValues();

	for (FScriptMapHelper::FIterator It(InHelper); It; ++It)
	{
		void* InKeyPtr = InHelper.GetKeyPtr(It);
		void* InValuePtr = InHelper.GetValuePtr(It);

		OutHelper.AddDefaultValue_Invalid_NeedsRehash();
		int32 NewIndex = OutHelper.Num() - 1;

		void* OutKeyPtr = OutHelper.GetKeyPtr(NewIndex);
		void* OutValuePtr = OutHelper.GetValuePtr(NewIndex);

		OutKeyProp->CopySingleValue(OutKeyPtr, InKeyPtr);
		OutValueProp->CopySingleValue(OutValuePtr, InValuePtr);
	}

	OutHelper.Rehash();

	*static_cast<bool*>(RESULT_PARAM) = true;
}

#pragma endregion "Convert"
