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

void UGameEventNodeUtils::AddListener_ByFuncName(UObject* WorldContextObject, const FGameplayTag EventName, const FString& FunctionName)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return;
	}
	AddListener_StrKey_ByFuncName(WorldContextObject, FEventId::TagToEventName(EventName), FunctionName);
}

void UGameEventNodeUtils::AddListener_StrKey_ByFuncName(UObject* WorldContextObject, const FString EventName, const FString& FunctionName)
{
	FListenerContext Listener;
	Listener.Receiver = WorldContextObject;
	Listener.FunctionName = FunctionName;

	if (FGameEventManager::Get().IsValid())
	{
		FGameEventManager::Get()->AddListener(FEventId(EventName), Listener);
	}
}

void UGameEventNodeUtils::AddListener_ByDelegate(UObject* WorldContextObject, const FGameplayTag EventName, const FEventPropertyDelegate& PropertyDelegate)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return;
	}

	AddListener_StrKey_ByDelegate(WorldContextObject, FEventId::TagToEventName(EventName), PropertyDelegate);
}

void UGameEventNodeUtils::AddListener_StrKey_ByDelegate(UObject* WorldContextObject, const FString EventName, const FEventPropertyDelegate& PropertyDelegate)
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

void UGameEventNodeUtils::RemoveListener(UObject* WorldContextObject, const FGameplayTag EventName)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return;
	}

	RemoveListener_StrKey(WorldContextObject, FEventId::TagToEventName(EventName));
}

void UGameEventNodeUtils::RemoveListener_StrKey(UObject* WorldContextObject, const FString EventName)
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

void UGameEventNodeUtils::SendEvent(UObject* WorldContextObject, const FGameplayTag EventName, const bool bPinned, const int32& ParamData)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEvent)
{
	P_GET_OBJECT(UObject, WorldContextObject);
	P_GET_STRUCT(FGameplayTag, EventName);
	P_GET_UBOOL(bPinned);

	Stack.Step(Stack.Object, nullptr);

	FProperty* Property = nullptr;
	void* ValuePtr = nullptr;
	if (Stack.MostRecentProperty != nullptr)
	{
		Property = CastField<FProperty>(Stack.MostRecentProperty);
		ValuePtr = Stack.MostRecentPropertyAddress;
	}

	P_FINISH;
	P_NATIVE_BEGIN;
		if (!IsValid(WorldContextObject) || !EventName.IsValid())
		{
			return;
		}

		FString Key = FEventId::TagToEventName(EventName);

		FEventContext EventContext;
		EventContext.WorldContext = WorldContextObject;
		EventContext.EventId.Key = Key;
		EventContext.bPinned = bPinned;
		EventContext.AddPropertyContext(Property, ValuePtr);
		FGameEventManager::Get()->SendEvent(EventContext);
	P_NATIVE_END;
}

void UGameEventNodeUtils::SendEvent_StrKey(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UGameEventNodeUtils::execSendEvent_StrKey)
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

void UGameEventNodeUtils::SendEvent_NoParam(UObject* WorldContextObject, const FGameplayTag EventName, const bool bPinned)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return;
	}

	SendEvent_NoParam_StrKey(WorldContextObject, FEventId::TagToEventName(EventName), bPinned);
}

void UGameEventNodeUtils::SendEvent_NoParam_StrKey(UObject* WorldContextObject, const FString EventName, const bool bPinned)
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

void UGameEventNodeUtils::UnpinEvent(UObject* WorldContextObject, const FGameplayTag EventName)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return;
	}

	UnpinEvent_StrKey(WorldContextObject, FEventId::TagToEventName(EventName));
}

void UGameEventNodeUtils::UnpinEvent_StrKey(UObject* WorldContextObject, const FString EventName)
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

bool UGameEventNodeUtils::HasEvent(UObject* WorldContextObject, const FGameplayTag EventName)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return false;
	}

	return HasEvent_StrKey(WorldContextObject, FEventId::TagToEventName(EventName));
}

bool UGameEventNodeUtils::HasEvent_StrKey(UObject* WorldContextObject, const FString EventName)
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

int32 UGameEventNodeUtils::GetEventListenerCount(UObject* WorldContextObject, const FGameplayTag EventName)
{
	if (!IsValid(WorldContextObject) || !EventName.IsValid())
	{
		return 0;
	}

	return GetEventListenerCount_StrKey(WorldContextObject, FEventId::TagToEventName(EventName));
}

int32 UGameEventNodeUtils::GetEventListenerCount_StrKey(UObject* WorldContextObject, const FString EventName)
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
bool UGameEventNodeUtils::ConvertToBoolean(const FEventProperty& InProp, bool& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FBoolProperty* Property = CastField<FBoolProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToInt(const FEventProperty& InProp, int32& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FIntProperty* Property = CastField<FIntProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToInt64(const FEventProperty& InProp, int64& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FInt64Property* Property = CastField<FInt64Property>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToFloat(const FEventProperty& InProp, float& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FFloatProperty* Property = CastField<FFloatProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToDouble(const FEventProperty& InProp, double& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FDoubleProperty* Property = CastField<FDoubleProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToByte(const FEventProperty& InProp, uint8& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FByteProperty* Property = CastField<FByteProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToString(const FEventProperty& InProp, FString& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FStrProperty* Property = CastField<FStrProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToText(const FEventProperty& InProp, FText& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FTextProperty* Property = CastField<FTextProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToName(const FEventProperty& InProp, FName& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FNameProperty* Property = CastField<FNameProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToStruct(const FEventProperty& InProp, TFieldPath<FProperty>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertToStruct)
{
	P_GET_STRUCT_REF(FEventProperty, InProp);

	Stack.StepCompiledIn<FProperty>(nullptr);
	void* OutStructPtr = Stack.MostRecentPropertyAddress;
	FProperty* OutStructProperty = Stack.MostRecentProperty;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!OutStructProperty || !OutStructPtr || !InProp.IsPropertyValid())
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

bool UGameEventNodeUtils::ConvertToObject(const FEventProperty& InProp, UObject*& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FObjectProperty* Property = CastField<FObjectProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetObjectPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToSoftObject(const FEventProperty& InProp, TSoftObjectPtr<>& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FSoftObjectProperty* Property = CastField<FSoftObjectProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetObjectPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToClass(const FEventProperty& InProp, UClass*& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FClassProperty* Property = CastField<FClassProperty>(InProp.Property.Get()))
		{
			OutValue = Cast<UClass>(Property->GetObjectPropertyValue(InProp.PropertyPtr));
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertToSoftClass(const FEventProperty& InProp, TSoftClassPtr<>& OutValue)
{
	if (InProp.IsPropertyValid())
	{
		if (const FSoftClassProperty* Property = CastField<FSoftClassProperty>(InProp.Property.Get()))
		{
			OutValue = Property->GetObjectPropertyValue(InProp.PropertyPtr);
			return true;
		}
	}
	return false;
}

bool UGameEventNodeUtils::ConvertArrayType(const FEventProperty& InProp, TArray<uint8>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertArrayType)
{
	P_GET_STRUCT_REF(FEventProperty, InProp);

	Stack.StepCompiledIn<FProperty>(nullptr);
	void* OutArrayPtr = Stack.MostRecentPropertyAddress;
	FProperty* OutArrayProperty = Stack.MostRecentProperty;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!InProp.IsPropertyValid() || !OutArrayProperty || !OutArrayPtr)
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

bool UGameEventNodeUtils::ConvertSetType(const FEventProperty& InProp, TSet<uint8>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertSetType)
{
	P_GET_STRUCT_REF(FEventProperty, InProp);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* OutProperty = Stack.MostRecentProperty;
	void* OutPropertyPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!InProp.IsPropertyValid() || !OutProperty)
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

bool UGameEventNodeUtils::ConvertMapType(const FEventProperty& InProp, TMap<uint8, uint8>& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UGameEventNodeUtils::execConvertMapType)
{
	P_GET_STRUCT_REF(FEventProperty, InProp);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* OutProperty = Stack.MostRecentProperty;
	void* OutPropertyPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	*static_cast<bool*>(RESULT_PARAM) = false;

	if (!InProp.IsPropertyValid() || !OutProperty)
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
