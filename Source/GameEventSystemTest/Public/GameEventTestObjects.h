#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include <atomic>
#include "GameEventTestObjects.generated.h"

// ========================================
// 测试用枚举类型定义
// ========================================

/**
 * 基础枚举类型 - 用于常见枚举测试
 */
UENUM(BlueprintType)
enum class ETestEnum : uint8
{
	None    = 0 UMETA(DisplayName = "无"),
	Alpha   = 1 UMETA(DisplayName = "阿尔法"),
	Beta    = 2 UMETA(DisplayName = "贝塔"),
	Gamma   = 3 UMETA(DisplayName = "伽马"),
	Delta   = 4 UMETA(DisplayName = "德尔塔"),
	Epsilon = 5 UMETA(DisplayName = "艾普西隆")
};

/**
 * 游戏事件优先级枚举
 */
UENUM(BlueprintType)
enum class EGameEventPriority : uint8
{
	Lowest    = 0 UMETA(DisplayName = "最低"),
	Low       = 1 UMETA(DisplayName = "低"),
	Normal    = 2 UMETA(DisplayName = "正常"),
	High      = 3 UMETA(DisplayName = "高"),
	Critical  = 4 UMETA(DisplayName = "关键"),
	Emergency = 5 UMETA(DisplayName = "紧急")
};

/**
 * 游戏事件状态枚举
 */
UENUM(BlueprintType)
enum class EGameEventState : uint8
{
	Inactive   = 0 UMETA(DisplayName = "未激活"),
	Pending    = 1 UMETA(DisplayName = "等待中"),
	Active     = 2 UMETA(DisplayName = "激活"),
	Processing = 3 UMETA(DisplayName = "处理中"),
	Completed  = 4 UMETA(DisplayName = "已完成"),
	Failed     = 5 UMETA(DisplayName = "失败"),
	Cancelled  = 6 UMETA(DisplayName = "已取消")
};

/**
 * 不常用的边界枚举类型
 */
UENUM()
enum class EBoundaryEnum : uint8
{
	MinValue = 0,
	MidValue = 127,
	MaxValue = 255
};

/**
 * 非蓝图枚举 - 测试边界情况
 */
UENUM()
enum class ENonBlueprintEnum : uint8
{
	Option1 = 0,
	Option2 = 1,
	Option3 = 233
};

UENUM()
enum class ENonBlueprintEnum16 : uint16
{
	Option1 = 0,
	Option2 = 1,
	Option3 = 34444
};

UENUM()
enum class ENonBlueprintEnum32 : uint32
{
	Option1 = 0,
	Option2 = 1,
	Option3 = 2333444
};

UENUM()
enum class ENonBlueprintEnum64 : uint64
{
	Option1 = 0,
	Option2 = 1,
	Option3 = 45554444
};

// ========================================
// 测试用结构体定义
// ========================================

/**
 * 简单测试结构体
 */
USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEMTEST_API FSimpleTestStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 IntValue = 0;

	UPROPERTY(BlueprintReadWrite)
	FString StringValue;

	UPROPERTY(BlueprintReadWrite)
	bool bBoolValue = false;

	FSimpleTestStruct()
	{
		IntValue = 0;
		StringValue = TEXT("");
		bBoolValue = false;
	}

	FSimpleTestStruct(const int32 InInt, const FString& InString, const bool InBool) : IntValue(InInt),
	                                                                                   StringValue(InString),
	                                                                                   bBoolValue(InBool)
	{
	}

	bool operator==(const FSimpleTestStruct& Other) const
	{
		return IntValue == Other.IntValue &&
		       StringValue == Other.StringValue &&
		       bBoolValue == Other.bBoolValue;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("简单结构体[整数值:%d, 字符串值:%s, 布尔值:%s]"),
		                       IntValue,
		                       *StringValue,
		                       bBoolValue ? TEXT("真") : TEXT("假"));
	}
};

/**
 * 复杂测试结构体
 */
USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEMTEST_API FComplexTestStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 Id = 0;

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	EGameEventPriority Priority = EGameEventPriority::Normal;

	UPROPERTY(BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> Values;

	UPROPERTY(BlueprintReadWrite)
	TMap<FString, float> Properties;

	FComplexTestStruct()
	{
		Id = 0;
		Name = TEXT("");
		Priority = EGameEventPriority::Normal;
		Position = FVector::ZeroVector;
		Values.Empty();
		Properties.Empty();
	}

	bool operator==(const FComplexTestStruct& Other) const
	{
		bool bArraysEqual = Values.Num() == Other.Values.Num();
		if (bArraysEqual)
		{
			for (int32 i = 0; i < Values.Num(); ++i)
			{
				if (Values[i] != Other.Values[i])
				{
					bArraysEqual = false;
					break;
				}
			}
		}

		bool bMapsEqual = Properties.Num() == Other.Properties.Num();
		if (bMapsEqual)
		{
			for (const auto& Pair : Properties)
			{
				const float* OtherValue = Other.Properties.Find(Pair.Key);
				if (!OtherValue || *OtherValue != Pair.Value)
				{
					bMapsEqual = false;
					break;
				}
			}
		}

		return Id == Other.Id &&
		       Name == Other.Name &&
		       Priority == Other.Priority &&
		       Position.Equals(Other.Position, 0.001f) &&
		       bArraysEqual && bMapsEqual;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("复杂结构体[ID:%d, 名称:%s, 优先级:%d, 位置:%s, 数值数量:%d, 属性数量:%d]"),
		                       Id,
		                       *Name,
		                       static_cast<int32>(Priority),
		                       *Position.ToString(),
		                       Values.Num(),
		                       Properties.Num());
	}
};

/**
 * 嵌套容器结构体 - 用于测试复杂嵌套情况
 */
USTRUCT(BlueprintType)
struct GAMEEVENTSYSTEMTEST_API FNestedContainerStruct
{
	GENERATED_BODY()

	TArray<TArray<int32>> NestedIntArrays;

	TMap<FString, TArray<float>> StringToFloatArrayMap;

	UPROPERTY(BlueprintReadWrite)
	TArray<FSimpleTestStruct> StructArray;

	FNestedContainerStruct()
	{
		NestedIntArrays.Empty();
		StringToFloatArrayMap.Empty();
		StructArray.Empty();
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("嵌套容器结构体[嵌套数组数量:%d, 映射表数量:%d, 结构体数组数量:%d]"),
		                       NestedIntArrays.Num(),
		                       StringToFloatArrayMap.Num(),
		                       StructArray.Num());
	}
};

// ========================================
// 测试用类定义
// ========================================

/**
 * 基础测试接收器类
 */
UCLASS(BlueprintType)
class GAMEEVENTSYSTEMTEST_API UGameEventTestReceiver : public UObject
{
	GENERATED_BODY()

public:
	UGameEventTestReceiver();

	// 重置测试状态
	void ResetTestState();

	// 事件接收计数器
	UPROPERTY(BlueprintReadOnly)
	int32 EventReceivedCount = 0;

	// 原子计数器 - 用于多线程测试
	std::atomic<int32> AtomicEventCount { 0 };

	// 最后接收到的数据
	UPROPERTY(BlueprintReadOnly)
	FString LastReceivedString;

	UPROPERTY(BlueprintReadOnly)
	int32 LastReceivedInt = 0;

	UPROPERTY(BlueprintReadOnly)
	float LastReceivedFloat = 0.0f;

	// ========================================
	// 基础类型事件处理函数
	// ========================================

	UFUNCTION()
	void OnBoolEvent(bool bValue);

	UFUNCTION()
	void OnInt8Event(int8 Value);

	UFUNCTION()
	void OnUInt8Event(uint8 Value);

	UFUNCTION()
	void OnInt16Event(int16 Value);

	UFUNCTION()
	void OnUInt16Event(uint16 Value);

	UFUNCTION()
	void OnInt32Event(int32 Value);

	UFUNCTION()
	void OnUInt32Event(uint32 Value);

	UFUNCTION()
	void OnInt64Event(int64 Value);

	UFUNCTION()
	void OnUInt64Event(uint64 Value);

	UFUNCTION()
	void OnFloatEvent(float Value);

	UFUNCTION()
	void OnDoubleEvent(double Value);

	// ========================================
	// 字符串类型事件处理函数
	// ========================================

	UFUNCTION()
	void OnStringEvent(const FString& Value);

	UFUNCTION()
	void OnNameEvent(const FName& Value);

	UFUNCTION()
	void OnTextEvent(const FText& Value);

	// ========================================
	// 数学类型事件处理函数
	// ========================================

	UFUNCTION()
	void OnVectorEvent(const FVector& Value);

	UFUNCTION()
	void OnVector2DEvent(const FVector2D& Value);

	UFUNCTION()
	void OnVector4Event(const FVector4& Value);

	UFUNCTION()
	void OnRotatorEvent(const FRotator& Value);

	UFUNCTION()
	void OnQuatEvent(const FQuat& Value);

	UFUNCTION()
	void OnTransformEvent(const FTransform& Value);

	UFUNCTION()
	void OnColorEvent(const FColor& Value);

	UFUNCTION()
	void OnLinearColorEvent(const FLinearColor& Value);

	// ========================================
	// 自定义类型事件处理函数
	// ========================================

	UFUNCTION()
	void OnEnumEvent(ETestEnum Value);

	UFUNCTION()
	void OnPriorityEvent(EGameEventPriority Value);

	UFUNCTION()
	void OnStateEvent(EGameEventState Value);

	UFUNCTION()
	void OnNonBlueprintEnum64(ENonBlueprintEnum64 Value);

	UFUNCTION()
	void OnSimpleStructEvent(const FSimpleTestStruct& Value);

	UFUNCTION()
	void OnComplexStructEvent(const FComplexTestStruct& Value);

	UFUNCTION()
	void OnNestedStructEvent(const FNestedContainerStruct& Value);

	// ========================================
	// 容器类型事件处理函数
	// ========================================

	UFUNCTION()
	void OnIntArrayEvent(const TArray<int32>& Values);

	UFUNCTION()
	void OnStringArrayEvent(const TArray<FString>& Values);

	UFUNCTION()
	void OnIntSetEvent(const TSet<int32>& Values);

	UFUNCTION()
	void OnStringToIntMapEvent(const TMap<FString, int32>& Values);

	UFUNCTION()
	void OnIntToFloatMapEvent(const TMap<int32, float>& Values);

	// ========================================
	// 嵌套容器类型事件处理函数
	// ========================================

	void OnNestedIntArrayEvent(const TArray<TArray<int32>>& Values);

	void OnStringToFloatArrayMapEvent(const TMap<FString, TArray<float>>& Values);

	UFUNCTION()
	void OnStructArrayEvent(const TArray<FSimpleTestStruct>& Values);

	// ========================================
	// 多参数事件处理函数
	// ========================================

	UFUNCTION()
	void OnMultiParamEvent(int32 IntValue, const FString& StringValue, bool bBoolValue);

	UFUNCTION()
	void OnComplexMultiParamEvent(const FVector& Position, EGameEventPriority Priority, const TArray<int32>& Values);

	// ========================================
	// 无参数事件处理函数
	// ========================================

	UFUNCTION()
	void OnSimpleEvent();

	// ========================================
	// 边界情况处理函数
	// ========================================

	UFUNCTION()
	void OnBoundaryEnumEvent(EBoundaryEnum Value);

	UFUNCTION()
	void OnNonBlueprintEnumEvent(ENonBlueprintEnum Value);

	// ========================================
	// 测试辅助变量
	// ========================================

	// 用于存储各种类型的最后接收值
	bool LastBoolValue = false;
	int8 LastInt8Value = 0;
	uint8 LastUInt8Value = 0;
	int16 LastInt16Value = 0;
	uint16 LastUInt16Value = 0;
	int64 LastInt64Value = 0;
	uint64 LastUInt64Value = 0;
	double LastDoubleValue = 0.0;

	FName LastNameValue;
	FText LastTextValue;

	FVector LastVectorValue = FVector::ZeroVector;
	FVector2D LastVector2DValue = FVector2D::ZeroVector;
	FVector4 LastVector4Value = FVector4::Zero();
	FRotator LastRotatorValue = FRotator::ZeroRotator;
	FQuat LastQuatValue = FQuat::Identity;
	FTransform LastTransformValue = FTransform::Identity;
	FColor LastColorValue = FColor::White;
	FLinearColor LastLinearColorValue = FLinearColor::White;

	ETestEnum LastEnumValue = ETestEnum::None;
	EGameEventPriority LastPriorityValue = EGameEventPriority::Normal;
	EGameEventState LastStateValue = EGameEventState::Inactive;
	ENonBlueprintEnum64 LastEnum64Value = ENonBlueprintEnum64::Option3;

	FSimpleTestStruct LastSimpleStructValue;
	FComplexTestStruct LastComplexStructValue;
	FNestedContainerStruct LastNestedStructValue;

	TArray<int32> LastIntArrayValue;
	TArray<FString> LastStringArrayValue;
	TSet<int32> LastIntSetValue;
	TMap<FString, int32> LastStringToIntMapValue;
	TMap<int32, float> LastIntToFloatMapValue;

	TArray<TArray<int32>> LastNestedIntArrayValue;
	TMap<FString, TArray<float>> LastStringToFloatArrayMapValue;
	TArray<FSimpleTestStruct> LastStructArrayValue;

	// 多参数事件的最后接收值
	struct FMultiParamValues
	{
		int32 IntValue = 0;
		FString StringValue;
		bool bBoolValue = false;
	} LastMultiParamValues;

	struct FComplexMultiParamValues
	{
		FVector Position = FVector::ZeroVector;
		EGameEventPriority Priority = EGameEventPriority::Normal;
		TArray<int32> Values;
	} LastComplexMultiParamValues;

	// 边界情况枚举的最后接收值
	EBoundaryEnum LastBoundaryEnumValue = EBoundaryEnum::MinValue;
	ENonBlueprintEnum LastNonBlueprintEnumValue = ENonBlueprintEnum::Option1;
};

/**
 * 复杂测试对象类 - 用于对象类型测试
 */
UCLASS(BlueprintType)
class GAMEEVENTSYSTEMTEST_API UComplexTestObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	int32 ObjectId = 0;

	UPROPERTY(BlueprintReadWrite)
	FString ObjectName;

	UPROPERTY(BlueprintReadWrite)
	EGameEventPriority ObjectPriority = EGameEventPriority::Normal;

	UPROPERTY(BlueprintReadWrite)
	FComplexTestStruct ObjectData;

	UComplexTestObject()
	{
		ObjectId = 0;
		ObjectName = TEXT("默认对象");
		ObjectPriority = EGameEventPriority::Normal;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("复杂对象[ID:%d, 名称:%s, 优先级:%d]"),
		                       ObjectId,
		                       *ObjectName,
		                       static_cast<int32>(ObjectPriority));
	}
};

// ========================================
// 测试辅助类和宏定义
// ========================================

/**
 * 测试辅助功能类
 */
class GAMEEVENTSYSTEMTEST_API FGameEventTestHelper
{
public:
	// 创建测试接收器
	static UGameEventTestReceiver* CreateTestReceiver(UObject* Outer = nullptr);

	// 创建复杂测试对象
	static UComplexTestObject* CreateComplexTestObject(UObject* Outer = nullptr);

	// 创建测试世界上下文
	static UObject* CreateTestWorldContext();

	// 清理测试环境
	static void CleanupTestEnvironment();

	// 验证事件是否被接收
	static bool VerifyEventReceived(const UGameEventTestReceiver* Receiver, int32 ExpectedCount = 1);

	// 等待事件处理完成
	static void WaitForEventProcessing(float MaxWaitTime = 0.1f);

	// 性能测试辅助函数
	static double MeasureEventPerformance(const TFunction<void()>& TestFunction, int32 Iterations = 1000);

	// 创建测试数据辅助函数
	static FSimpleTestStruct CreateTestSimpleStruct(int32 IntValue, const FString& StringValue, bool bBoolValue);
	static FComplexTestStruct CreateTestComplexStruct(int32 Id, const FString& Name, EGameEventPriority Priority);
	static FNestedContainerStruct CreateTestNestedStruct();

	// 验证数据相等性
	static bool AreEqual(const FSimpleTestStruct& A, const FSimpleTestStruct& B);
	static bool AreEqual(const FComplexTestStruct& A, const FComplexTestStruct& B);

	// 生成随机测试数据
	static TArray<int32> GenerateRandomIntArray(int32 Size = 10);
	static TArray<FString> GenerateRandomStringArray(int32 Size = 5);
	static TMap<FString, int32> GenerateRandomStringToIntMap(int32 Size = 5);

	// 多线程测试辅助
	static void RunMultiThreadTest(TFunction<void()> TestFunction, int32 ThreadCount = 4, int32 IterationsPerThread = 25);

	// 压力测试辅助
	static bool RunStressTest(const TFunction<void()>& TestFunction, int32 TotalIterations = 10000, float MaxTimeSeconds = 10.0f);
};
