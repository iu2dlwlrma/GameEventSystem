#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/WeakObjectPtr.h"
#include "GameEventTypes.generated.h"

#pragma region Enum

UENUM(BlueprintType)
enum class EEventIdType : uint8
{
	TagBased UMETA(DisplayName = "Tag"),
	StringBased UMETA(DisplayName = "String"),
};

UENUM(BlueprintType)
enum class EEventBindType : uint8
{
	FunctionName UMETA(DisplayName = "FunctionName"),
	Delegate UMETA(DisplayName = "Delegate")
};

#pragma endregion Enum

#pragma region Property

USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEM_API FEventProperty
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GameEventSystem|Property")
	TFieldPath<FProperty> Property;
	void* PropertyPtr;

	FEventProperty() : PropertyPtr(nullptr)
	{
	}

	bool IsPropertyValid() const
	{
		return Property.Get() != nullptr && PropertyPtr != nullptr;
	}
};

struct GAMEEVENTSYSTEM_API FPropertyContext
{
	FProperty* Property;
	void* PropertyPtr;

	FPropertyContext() : Property(nullptr),
	                     PropertyPtr(nullptr)
	{
	}

	FPropertyContext(FProperty* Property, void* PropertyPtr) : Property(Property),
	                                                           PropertyPtr(PropertyPtr)
	{
	}

	bool IsValid() const
	{
		return Property != nullptr && PropertyPtr != nullptr;
	}

	void Clean()
	{
		Property = nullptr;
		PropertyPtr = nullptr;
	}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FEventPropertyDelegate, const FEventProperty&, Property);

USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEM_API FEventTypeInfo
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsNoParams = true;

	UPROPERTY()
	FName PinCategory;

	UPROPERTY()
	FName PinSubCategory;

	UPROPERTY()
	TWeakObjectPtr<UObject> PinSubCategoryObject;

	UPROPERTY()
	FEdGraphTerminalType PinValueType;

	UPROPERTY()
	EPinContainerType ContainerType;

	FEventTypeInfo() : ContainerType(EPinContainerType::None)
	{
	}

	FEventTypeInfo(const FName& InPinCategory, const FName& InPinSubCategory, const TWeakObjectPtr<UObject>& InPinSubCategoryObject, const FEdGraphTerminalType& InPinValueType, const EPinContainerType InContainerType) : bIsNoParams(false),
	                                                                                                                                                                                                                        PinCategory(InPinCategory),
	                                                                                                                                                                                                                        PinSubCategory(InPinSubCategory),
	                                                                                                                                                                                                                        PinSubCategoryObject(InPinSubCategoryObject),
	                                                                                                                                                                                                                        PinValueType(InPinValueType),
	                                                                                                                                                                                                                        ContainerType(InContainerType)
	{
	}

	FEventTypeInfo(const FEdGraphPinType& InPinType) : bIsNoParams(false),
	                                                   PinCategory(InPinType.PinCategory),
	                                                   PinSubCategory(InPinType.PinSubCategory),
	                                                   PinSubCategoryObject(InPinType.PinSubCategoryObject),
	                                                   PinValueType(InPinType.PinValueType),
	                                                   ContainerType(InPinType.ContainerType)
	{
	}

	FString ToString() const
	{
		if (bIsNoParams)
		{
			return FString("NoParams");
		}
		FString Result = PinCategory.ToString() + TEXT(".") + PinSubCategory.ToString();
		if (PinSubCategoryObject.Get())
		{
			Result += TEXT(".") + PinSubCategoryObject->GetName();
		}
		if (IsArray())
		{
			Result += TEXT(" Array");
		}
		if (IsSet())
		{
			Result += TEXT(" Set");
		}
		if (IsMap())
		{
			Result += +TEXT(".") + PinValueType.TerminalCategory.ToString() + TEXT(" Map");
		}
		return Result;
	}

	FORCEINLINE bool IsContainer() const
	{
		return ContainerType != EPinContainerType::None;
	}

	FORCEINLINE bool IsArray() const
	{
		return ContainerType == EPinContainerType::Array;
	}

	FORCEINLINE bool IsSet() const
	{
		return ContainerType == EPinContainerType::Set;
	}

	FORCEINLINE bool IsMap() const
	{
		return ContainerType == EPinContainerType::Map;
	}
};

USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEM_API FEventTypeRegistry
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FString, FEventTypeInfo> EventTypes;

	void RegisterEventType(const FString& EventName, const FEventTypeInfo& TypeInfo)
	{
		EventTypes.Add(EventName, TypeInfo);
	}

	const FEventTypeInfo* GetEventTypeInfo(const FString& EventName) const
	{
		return EventTypes.Find(EventName);
	}

	bool IsEventRegistered(const FString& EventName) const
	{
		return EventTypes.Contains(EventName);
	}

	void UnregisterEventType(const FString& EventName)
	{
		EventTypes.Remove(EventName);
	}

	void ClearAll()
	{
		EventTypes.Empty();
	}
};

#pragma endregion Property

#pragma region Event

USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEM_API FEventId
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	/** String format event identifier, format: "A.B.C" */
	UPROPERTY(BlueprintReadWrite, Category="GameEventSystem")
	FString Key;

	FEventId() = default;

	explicit FEventId(const FString& InStringId) : Key(InStringId)
	{
	}

	explicit FEventId(const FGameplayTag& InTag) : Tag(InTag)
	{
		Key = TagToEventName(InTag);
	}

	FString GetName() const
	{
		return Key;
	}

	FString ToString() const
	{
		return Key;
	}

	bool IsValid() const
	{
		return !Key.IsEmpty();
	}

	bool operator==(const FEventId& Other) const
	{
		return Key == Other.Key;
	}

	bool operator!=(const FEventId& Other) const
	{
		return Key != Other.Key;
	}

	friend uint32 GetTypeHash(const FEventId& EventId)
	{
		return GetTypeHash(EventId.Key);
	}

	/** Convert GameplayTag to event name */
	static FString TagToEventName(const FGameplayTag InTag)
	{
		if (!InTag.IsValid())
		{
			return FString();
		}

		FString TagString = InTag.ToString();
		const FString Prefix = TEXT("GameplayTag.");

		if (TagString.StartsWith(Prefix))
		{
			return TagString.RightChop(Prefix.Len());
		}
		return TagString;
	}
};

#pragma region Listener

struct GAMEEVENTSYSTEM_API FListenerContext
{
	TWeakObjectPtr<> Receiver;
	FString FunctionName;
	UFunction* Function;
	FEventPropertyDelegate PropertyDelegate;
	TFunction<void(const FEventProperty&)> LambdaFunction;

	FListenerContext() : Receiver(nullptr),
	                     Function(nullptr)
	{
	}

	bool IsValid() const
	{
		if (!Receiver.IsValid())
		{
			return false;
		}

		if (PropertyDelegate.IsBound())
		{
			return true;
		}

		if (LambdaFunction != nullptr)
		{
			return true;
		}

		return Function != nullptr;
	}

	bool LinkFunction()
	{
		if (!Receiver.IsValid() || FunctionName.IsEmpty())
		{
			return false;
		}

		Function = Receiver->FindFunction(FName(*FunctionName));
		return Function != nullptr;
	}

	bool IsBoundToDelegate() const
	{
		if (!Receiver.IsValid())
		{
			return false;
		}
		return PropertyDelegate.IsBound();
	}

	bool IsBoundToLambda() const
	{
		if (!Receiver.IsValid())
		{
			return false;
		}
		return LambdaFunction != nullptr;
	}

	bool operator==(const FListenerContext& Other) const
	{
		if (Receiver != Other.Receiver)
		{
			return false;
		}

		// For non-empty function names, compare function names (applies to both Lambda and regular functions)
		if (!FunctionName.IsEmpty() && !Other.FunctionName.IsEmpty())
		{
			return FunctionName == Other.FunctionName;
		}

		// For UFunction, compare function pointers (when function name is empty or one of them is empty)
		if (Function && Other.Function)
		{
			return Function == Other.Function;
		}

		// For delegates, compare delegate object function names
		if (PropertyDelegate.IsBound() && Other.PropertyDelegate.IsBound())
		{
			return PropertyDelegate.GetFunctionName() == Other.PropertyDelegate.GetFunctionName();
		}

		// For Lambda functions, compare TFunction object addresses
		if (LambdaFunction && Other.LambdaFunction)
		{
			return &LambdaFunction == &Other.LambdaFunction;
		}

		// Special case: one has function name, another has function pointer, but function name matches the function pointer's name
		if (!FunctionName.IsEmpty() && Other.Function)
		{
			return FunctionName == Other.Function->GetName();
		}
		if (Function && !Other.FunctionName.IsEmpty())
		{
			return Function->GetName() == Other.FunctionName;
		}

		// If none of the above conditions are met, consider them unequal
		return false;
	}

	friend uint32 GetTypeHash(const FListenerContext& Listener)
	{
		uint32 Hash = GetTypeHash(Listener.Receiver);

		// Generate different hash values for different types of listeners
		if (Listener.LambdaFunction != nullptr)
		{
			// Generate hash using Lambda function object address
			Hash ^= PointerHash(&Listener.LambdaFunction) ^ 0x55555555;
		}
		else if (Listener.PropertyDelegate.IsBound())
		{
			// Generate hash using delegate object address
			Hash ^= PointerHash(&Listener.PropertyDelegate) ^ 0xAAAAAAAA;
		}
		else if (Listener.Function != nullptr)
		{
			// Generate hash using UFunction pointer
			Hash ^= GetTypeHash(Listener.Function) ^ 0x33333333;
		}

		// If there's a function name, include it in hash calculation
		if (!Listener.FunctionName.IsEmpty())
		{
			Hash ^= GetTypeHash(Listener.FunctionName);
		}

		return Hash;
	}
};

struct GAMEEVENTSYSTEM_API FListener
{
	FEventId EventId;
	FListenerContext Listener;

	FListener()
	{
	}

	bool operator==(const FListener& Other) const
	{
		return EventId == Other.EventId && Listener == Other.Listener;
	}
};

#pragma endregion Listener

struct GAMEEVENTSYSTEM_API FEventContextBase
{
	FEventId EventId;
	UObject* WorldContext;
	bool bPinned;

	FEventContextBase() : WorldContext(nullptr),
	                      bPinned(false)
	{
	}

	FEventContextBase(const FString& InEventName, UObject* InWorldContext, const bool InPinned) : EventId(InEventName),
	                                                                                              WorldContext(InWorldContext),
	                                                                                              bPinned(InPinned)
	{
	}
};

struct GAMEEVENTSYSTEM_API FEventContext : FEventContextBase
{
	TArray<FListenerContext> Listeners;

	TArray<FPropertyContext> PropertyContexts;

	FListenerContext* SpecificTarget;

	FEventContext() : SpecificTarget(nullptr)
	{
	}

	FEventContext(const FEventContextBase& Context) : FEventContextBase(Context),
	                                                  SpecificTarget(nullptr)
	{
	}

	void AddPropertyContext(const FPropertyContext& Context)
	{
		PropertyContexts.Add(Context);
	}

	void AddPropertyContext(FProperty* Property, void* PropertyPtr, FListenerContext* InSpecificTarget = nullptr)
	{
		SpecificTarget = InSpecificTarget;
		PropertyContexts.Add(FPropertyContext(Property, PropertyPtr));
	}

	void SetPropertyContexts(const TArray<FPropertyContext>& Contexts)
	{
		PropertyContexts = Contexts;
	}

	int32 GetParameterCount() const
	{
		return PropertyContexts.Num();
	}

	bool HasValidParameters() const
	{
		return PropertyContexts.Num() > 0;
	}
};

#pragma endregion Event
