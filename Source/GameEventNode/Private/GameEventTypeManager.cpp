#include "GameEventTypeManager.h"
#include "GameEventNodeTypes.h"
#include "Engine/Engine.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "GameEventNodeLog.h"

TSharedPtr<FGameEventTypeManager> FGameEventTypeManager::TypeManagerPtr = nullptr;

FGameEventTypeManager::FGameEventTypeManager()
{
}

FGameEventTypeManager::~FGameEventTypeManager()
{
	TypeRegistry.ClearAll();
}

FGameEventTypeManager* FGameEventTypeManager::Get()
{
	if (!TypeManagerPtr.IsValid())
	{
		TypeManagerPtr = MakeShared<FGameEventTypeManager>();
		UE_LOG_GAS_INFO(TEXT("GameEventTypeManager instance created successfully, hash: 0x%08X"), GetTypeHash(TypeManagerPtr));
	}
	return TypeManagerPtr.Get();
}

#pragma region "Static"

void FGameEventTypeManager::GetPinTypeFromProperty(FProperty* Property, FName& OutPinCategory, FName& OutPinSubCategory, UObject*& OutSubCategoryObject, FEdGraphTerminalType& OutPinValueType, EPinContainerType& OutContainerType)
{
	OutPinCategory = UEdGraphSchema_K2::PC_Wildcard;
	OutPinSubCategory = NAME_None;
	OutSubCategoryObject = nullptr;
	OutContainerType = EPinContainerType::None;
	OutPinValueType = FEdGraphTerminalType();

	if (!Property)
	{
		return;
	}

	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		OutContainerType = EPinContainerType::Array;
		EPinContainerType TempContainer = EPinContainerType::None;
		GetPinTypeFromProperty(ArrayProperty->Inner, OutPinCategory, OutPinSubCategory, OutSubCategoryObject, OutPinValueType, TempContainer);
		return;
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		OutContainerType = EPinContainerType::Set;
		EPinContainerType TempContainer = EPinContainerType::None;
		GetPinTypeFromProperty(SetProperty->ElementProp, OutPinCategory, OutPinSubCategory, OutSubCategoryObject, OutPinValueType, TempContainer);
		return;
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		OutContainerType = EPinContainerType::Map;

		EPinContainerType TempContainer = EPinContainerType::None;
		FEdGraphTerminalType TempKeyPinValueType;
		GetPinTypeFromProperty(MapProperty->KeyProp, OutPinCategory, OutPinSubCategory, OutSubCategoryObject, TempKeyPinValueType, TempContainer);

		FName ValueCategory, ValueSubCategory;
		UObject* ValueSubCategoryObject = nullptr;
		FEdGraphTerminalType TempValuePinValueType;
		GetPinTypeFromProperty(MapProperty->ValueProp, ValueCategory, ValueSubCategory, ValueSubCategoryObject, TempValuePinValueType, TempContainer);

		OutPinValueType.TerminalCategory = ValueCategory;
		OutPinValueType.TerminalSubCategory = ValueSubCategory;
		OutPinValueType.TerminalSubCategoryObject = MakeWeakObjectPtr(ValueSubCategoryObject);
		return;
	}

	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Byte;
		OutSubCategoryObject = EnumProperty->GetEnum();
	}
	else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Byte;
		OutSubCategoryObject = ByteProperty->Enum;
	}
	else if (Property->IsA<FBoolProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (Property->IsA<FIntProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (Property->IsA<FInt64Property>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Int64;
	}
	else if (Property->IsA<FFloatProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Real;
		OutPinSubCategory = UEdGraphSchema_K2::PC_Float;
	}
	else if (Property->IsA<FDoubleProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Real;
		OutPinSubCategory = UEdGraphSchema_K2::PC_Double;
	}
	else if (Property->IsA<FStrProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (Property->IsA<FTextProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (Property->IsA<FNameProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Object;
		OutSubCategoryObject = ObjectProperty->PropertyClass;
	}
	else if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_SoftObject;
		OutSubCategoryObject = SoftObjectProperty->PropertyClass;
	}
	else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Class;
		OutSubCategoryObject = ClassProperty->MetaClass;
	}
	else if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_SoftClass;
		OutSubCategoryObject = SoftClassProperty->MetaClass;
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Struct;
		OutSubCategoryObject = StructProperty->Struct;
	}
	else if (const FDelegateProperty* DelegateProperty = CastField<FDelegateProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Delegate;
		OutSubCategoryObject = DelegateProperty->SignatureFunction;
	}
	else if (const FMulticastDelegateProperty* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_MCDelegate;
		OutSubCategoryObject = MulticastDelegateProperty->SignatureFunction;
	}
	else
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Wildcard;
	}
}

void FGameEventTypeManager::AddTypeInfoFromProperty(FProperty* Property, FEventTypeInfo& OutTypeInfo)
{
	if (!Property)
	{
		return;
	}

	FName PinCategory;
	FName PinSubCategory;
	UObject* SubCategoryObject = nullptr;
	FEdGraphTerminalType PinValueType;
	EPinContainerType PinContainerType = EPinContainerType::None;

	GetPinTypeFromProperty(Property, PinCategory, PinSubCategory, SubCategoryObject, PinValueType, PinContainerType);

	OutTypeInfo.Add(PinCategory, PinSubCategory, SubCategoryObject, PinValueType, PinContainerType);
}

bool FGameEventTypeManager::AnalyzeUFunctionParameters(const UFunction* Function, FEventTypeInfo& OutTypeInfo)
{
	if (!Function)
	{
		return false;
	}
	OutTypeInfo = FEventTypeInfo();

	for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (!(Property->PropertyFlags & CPF_ReturnParm))
		{
			AddTypeInfoFromProperty(Property, OutTypeInfo);
		}
	}

	return true;
}

bool FGameEventTypeManager::AnalyzeFunctionSignature(const UClass* ObjectClass, const FString& FunctionName, FEventTypeInfo& OutTypeInfo)
{
	if (!ObjectClass || FunctionName.IsEmpty())
	{
		return false;
	}

	const UFunction* Function = ObjectClass->FindFunctionByName(*FunctionName);
	if (!Function)
	{
		return false;
	}

	return AnalyzeUFunctionParameters(Function, OutTypeInfo);
}
#pragma endregion

void FGameEventTypeManager::RegisterEventType(const FString& EventName, const FEventTypeInfo& TypeInfo)
{
	if (EventName.IsEmpty())
	{
		return;
	}
	TypeRegistry.RegisterEventType(EventName, TypeInfo);
	StartEventTypeNotify(EventName);
}

bool FGameEventTypeManager::GetEventTypeInfo(const FString& EventName, FEventTypeInfo& OutTypeInfo) const
{
	const FEventTypeInfo* TypeInfo = TypeRegistry.GetEventTypeInfo(EventName);
	if (TypeInfo && TypeInfo->IsValid())
	{
		OutTypeInfo = *TypeInfo;
		return true;
	}
	return false;
}

bool FGameEventTypeManager::IsEventRegistered(const FString& EventName) const
{
	return TypeRegistry.IsEventRegistered(EventName);
}

void FGameEventTypeManager::UnregisterEventType(const FString& EventName)
{
	TypeRegistry.UnregisterEventType(EventName);
}

void FGameEventTypeManager::ClearAllEventTypes()
{
	TypeRegistry.ClearAll();
}

TArray<FString> FGameEventTypeManager::GetAllRegisteredEventNames() const
{
	TArray<FString> EventNames;
	TypeRegistry.EventTypes.GetKeys(EventNames);
	return EventNames;
}

bool FGameEventTypeManager::AnalyzeAndRegisterFunctionType(const FString& EventName, const UClass* Receiver, const FString& FunctionName)
{
	FGameEventTypeManager* TypeManager = Get();
	if (!TypeManager || !Receiver || FunctionName.IsEmpty())
	{
		return false;
	}

	FEventTypeInfo TypeInfo;
	if (TypeManager->AnalyzeFunctionSignature(Receiver, FunctionName, TypeInfo))
	{
		TypeManager->RegisterEventType(EventName, TypeInfo);
		UE_LOG_GAS_INFO(TEXT("Event[%s] - Auto-registered function type [%s::%s] -> [%s]"),
		                *EventName,
		                *Receiver->GetName(),
		                *FunctionName,
		                *TypeInfo.ToString());
		return true;
	}

	UE_LOG_GAS_WARNING(TEXT("Event[%s] - Cannot analyze function signature [%s::%s]"),
	                *EventName,
	                *Receiver->GetName(),
	                *FunctionName);
	return false;
}

bool FGameEventTypeManager::BindEventTypeNotify(const FString& EventName, const int32 UniqueID, const TFunction<void()>& Function)
{
	if (EventName.IsEmpty() || !Function)
	{
		return false;
	}
	TMap<int32, TFunction<void()>>& NotifyGroup = EventTypeNotifyGroup.FindOrAdd(EventName);
	if (NotifyGroup.Contains(UniqueID))
	{
		return true;
	}
	NotifyGroup.Add(UniqueID, Function);
	return true;
}

bool FGameEventTypeManager::UnBindEventTypeNotify(const FString& EventName, const int32 UniqueID)
{
	if (EventName.IsEmpty())
	{
		return false;
	}
	TMap<int32, TFunction<void()>>* NotifyGroup = EventTypeNotifyGroup.Find(EventName);
	if (NotifyGroup->Contains(UniqueID))
	{
		NotifyGroup->Remove(UniqueID);
		return true;
	}
	return false;
}

void FGameEventTypeManager::StartEventTypeNotify(const FString& EventName)
{
	if (const TMap<int32, TFunction<void()>>* NotifyGroup = EventTypeNotifyGroup.Find(EventName))
	{
		// The auto keyword will deduce the type of each element in the TMap as a TPair
		// In this case, the type is TPair<const int32, TFunction<void()>>
		for (const auto& Pair : *NotifyGroup)
		{
			if (const TFunction<void()>& Function = Pair.Value)
			{
				Function();
			}
		}
	}
}
