#include "GameEventTypeManager.h"
#include "GameEventNodeTypes.h"
#include "Engine/Engine.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Logger.h"

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
		FLogger::Get().Log(TEXT("GameEventTypeManager instance created successfully, hash: 0x%08X"), GetTypeHash(TypeManagerPtr));
	}
	return TypeManagerPtr.Get();
}

#pragma region "Static"

void FGameEventTypeManager::GetPinTypeFromProperty(FProperty* Property, FName& OutPinCategory, FName& OutPinSubCategory, UObject*& OutSubCategoryObject, FEdGraphTerminalType& OutPinValueType, EPinContainerType& OutContainerType)
{
	if (!Property)
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Wildcard;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
		OutContainerType = EPinContainerType::None;
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
		FEdGraphTerminalType TempValueType;
		GetPinTypeFromProperty(MapProperty->KeyProp, OutPinCategory, OutPinSubCategory, OutSubCategoryObject, TempValueType, TempContainer);

		FName ValueCategory, ValueSubCategory;
		UObject* ValueSubCategoryObject = nullptr;
		GetPinTypeFromProperty(MapProperty->ValueProp, ValueCategory, ValueSubCategory, ValueSubCategoryObject, TempValueType, TempContainer);

		OutPinValueType.TerminalCategory = ValueCategory;
		OutPinValueType.TerminalSubCategory = ValueSubCategory;
		OutPinValueType.TerminalSubCategoryObject = MakeWeakObjectPtr(ValueSubCategoryObject);
		return;
	}

	OutContainerType = EPinContainerType::None;

	if (Property->IsA<FBoolProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Boolean;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Byte;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Byte;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = EnumProperty->GetEnum();
	}
	else if (Property->IsA<FIntProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Int;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (Property->IsA<FInt64Property>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Int64;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (Property->IsA<FFloatProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Real;
		OutPinSubCategory = UEdGraphSchema_K2::PC_Float;
		OutSubCategoryObject = nullptr;
	}
	else if (Property->IsA<FDoubleProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Real;
		OutPinSubCategory = UEdGraphSchema_K2::PC_Double;
		OutSubCategoryObject = nullptr;
	}
	else if (Property->IsA<FStrProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_String;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (Property->IsA<FTextProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Text;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (Property->IsA<FNameProperty>())
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Name;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
	else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Object;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = ObjectProperty->PropertyClass;
	}
	else if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_SoftObject;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = SoftObjectProperty->PropertyClass;
	}
	else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Class;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = ClassProperty->MetaClass;
	}
	else if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_SoftClass;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = SoftClassProperty->MetaClass;
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = StructProperty->Struct;
	}
	else if (const FDelegateProperty* DelegateProperty = CastField<FDelegateProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Delegate;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = DelegateProperty->SignatureFunction;
	}
	else if (const FMulticastDelegateProperty* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Property))
	{
		OutPinCategory = UEdGraphSchema_K2::PC_MCDelegate;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = MulticastDelegateProperty->SignatureFunction;
	}
	else
	{
		OutPinCategory = UEdGraphSchema_K2::PC_Wildcard;
		OutPinSubCategory = NAME_None;
		OutSubCategoryObject = nullptr;
	}
}

FEventTypeInfo FGameEventTypeManager::CreateTypeInfoFromProperty(FProperty* Property)
{
	if (!Property)
	{
		return FEventTypeInfo();
	}

	FName PinCategory;
	FName PinSubCategory;
	UObject* SubCategoryObject = nullptr;
	FEdGraphTerminalType PinValueType;
	EPinContainerType PinContainerType = EPinContainerType::None;

	GetPinTypeFromProperty(Property, PinCategory, PinSubCategory, SubCategoryObject, PinValueType, PinContainerType);

	FEventTypeInfo TypeInfo(PinCategory, PinSubCategory, SubCategoryObject, PinValueType, PinContainerType);

	return TypeInfo;
}

bool FGameEventTypeManager::AnalyzeUFunctionParameters(const UFunction* Function, FEventTypeInfo& OutTypeInfo)
{
	if (!Function)
	{
		return false;
	}

	for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (Property && !(Property->PropertyFlags & CPF_ReturnParm))
		{
			OutTypeInfo = CreateTypeInfoFromProperty(Property);
			return true;
		}
	}

	OutTypeInfo = FEventTypeInfo();
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
		FLogger::Get().LogDisplay(TEXT("Event[%s] - Auto-registered function type [%s::%s] -> [%s]"),
		                          *EventName,
		                          *Receiver->GetName(),
		                          *FunctionName,
		                          *TypeInfo.ToString());
		return true;
	}

	FLogger::Get().LogWarning(TEXT("Event[%s] - Cannot analyze function signature [%s::%s]"),
	                          *EventName,
	                          *Receiver->GetName(),
	                          *FunctionName);
	return false;
}
