#pragma once

#include "CoreMinimal.h"
#include "GameEventTypes.h"
#include "Logger.h"
#include "GameEventNodeTypes.generated.h"

static inline struct FGlobalConfig
{
	int32 MaxParameterNum = 8;
} GlobalConfig;

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

USTRUCT(BlueprintType)
struct GAMEEVENTNODE_API FEventParameterInfo
{
	GENERATED_BODY()

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

	FEventParameterInfo() : ContainerType(EPinContainerType::None)
	{
	}

	FEventParameterInfo(const FName& InPinCategory, const FName& InPinSubCategory, const TWeakObjectPtr<UObject>& InPinSubCategoryObject, const FEdGraphTerminalType& InPinValueType, const EPinContainerType InContainerType) : PinCategory(InPinCategory),
	                                                                                                                                                                                                                             PinSubCategory(InPinSubCategory),
	                                                                                                                                                                                                                             PinSubCategoryObject(InPinSubCategoryObject),
	                                                                                                                                                                                                                             PinValueType(InPinValueType),
	                                                                                                                                                                                                                             ContainerType(InContainerType)
	{
	}

	FEventParameterInfo(const FEdGraphPinType& InPinType) : PinCategory(InPinType.PinCategory),
	                                                        PinSubCategory(InPinType.PinSubCategory),
	                                                        PinSubCategoryObject(InPinType.PinSubCategoryObject),
	                                                        PinValueType(InPinType.PinValueType),
	                                                        ContainerType(InPinType.ContainerType)
	{
	}

	FString ToString() const
	{
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
			Result += TEXT(".") + PinValueType.TerminalCategory.ToString() + TEXT(" Map");
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

	FORCEINLINE bool IsValid() const
	{
		return !PinCategory.IsNone();
	}
};

USTRUCT(BlueprintType)
struct GAMEEVENTNODE_API FEventTypeInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FEventParameterInfo> Parameters;

	FEventTypeInfo()
	{
	}

	FEventTypeInfo(const FName& InPinCategory, const FName& InPinSubCategory, const TWeakObjectPtr<UObject>& InPinSubCategoryObject, const FEdGraphTerminalType& InPinValueType, const EPinContainerType InContainerType)

	{
		Parameters.Add(FEventParameterInfo(InPinCategory, InPinSubCategory, InPinSubCategoryObject, InPinValueType, InContainerType));
	}

	FEventTypeInfo(const TArray<FEventParameterInfo>& InParameters) : Parameters(InParameters)
	{
	}

	FORCEINLINE void Add(const FName& InPinCategory, const FName& InPinSubCategory, const TWeakObjectPtr<UObject>& InPinSubCategoryObject, const FEdGraphTerminalType& InPinValueType, const EPinContainerType InContainerType)

	{
		Parameters.Add(FEventParameterInfo(InPinCategory, InPinSubCategory, InPinSubCategoryObject, InPinValueType, InContainerType));
	}

	FString ToString() const
	{
		if (Parameters.Num() > 0)
		{
			FString Result = FString::Printf(TEXT("MultiParam(%d): "), Parameters.Num());
			for (int32 i = 0; i < Parameters.Num(); ++i)
			{
				if (i > 0)
					Result += TEXT(", ");
				Result += Parameters[i].ToString();
			}
			return Result;
		}
		return FString("NoParams");
	}

	FORCEINLINE bool IsValid() const
	{
		return Parameters.Num() > 0;
	}

	FORCEINLINE bool IsMultiParameter() const
	{
		return Parameters.Num() > 1;
	}

	FORCEINLINE int32 GetParameterCount() const
	{
		return FMath::Max(Parameters.Num(), 0);
	}

	const FEventParameterInfo* GetParameterInfo(const int32 Index) const
	{
		if (Parameters.IsValidIndex(Index))
		{
			return &Parameters[Index];
		}
		return nullptr;
	}
};

struct GAMEEVENTNODE_API FEventTypeRegistry
{
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
