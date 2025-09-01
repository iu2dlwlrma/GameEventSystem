#include "GameEventTestObjects.h"
#include "GameEventManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Async/Async.h"

// ========================================
// UGameEventTestReceiver 实现
// ========================================

UGameEventTestReceiver::UGameEventTestReceiver()
{
	ResetTestState();
}

void UGameEventTestReceiver::ResetTestState()
{
	EventReceivedCount = 0;
	AtomicEventCount.store(0);

	LastReceivedString = TEXT("");
	LastReceivedInt = 0;
	LastReceivedFloat = 0.0f;

	// 重置所有类型的最后接收值
	LastBoolValue = false;
	LastInt8Value = 0;
	LastUInt8Value = 0;
	LastInt16Value = 0;
	LastUInt16Value = 0;
	LastInt64Value = 0;
	LastUInt64Value = 0;
	LastDoubleValue = 0.0;

	LastNameValue = NAME_None;
	LastTextValue = FText::GetEmpty();

	LastVectorValue = FVector::ZeroVector;
	LastVector2DValue = FVector2D::ZeroVector;
	LastVector4Value = FVector4::Zero();
	LastRotatorValue = FRotator::ZeroRotator;
	LastQuatValue = FQuat::Identity;
	LastTransformValue = FTransform::Identity;
	LastColorValue = FColor::White;
	LastLinearColorValue = FLinearColor::White;

	LastEnumValue = ETestEnum::None;
	LastPriorityValue = EGameEventPriority::Normal;
	LastStateValue = EGameEventState::Inactive;

	LastSimpleStructValue = FSimpleTestStruct();
	LastComplexStructValue = FComplexTestStruct();
	LastNestedStructValue = FNestedContainerStruct();

	LastIntArrayValue.Empty();
	LastStringArrayValue.Empty();
	LastIntSetValue.Empty();
	LastStringToIntMapValue.Empty();
	LastIntToFloatMapValue.Empty();

	LastNestedIntArrayValue.Empty();
	LastStringToFloatArrayMapValue.Empty();
	LastStructArrayValue.Empty();

	LastMultiParamValues = FMultiParamValues();
	LastComplexMultiParamValues = FComplexMultiParamValues();

	LastBoundaryEnumValue = EBoundaryEnum::MinValue;
	LastNonBlueprintEnumValue = ENonBlueprintEnum::Option1;
}

// ========================================
// 基础类型事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnBoolEvent(const bool bValue)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastBoolValue = bValue;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到布尔事件: %s"), bValue ? TEXT("真") : TEXT("假"));
}

void UGameEventTestReceiver::OnInt8Event(const int8 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastInt8Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到8位整数事件: %d"), Value);
}

void UGameEventTestReceiver::OnUInt8Event(const uint8 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastUInt8Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到无符号8位整数事件: %u"), Value);
}

void UGameEventTestReceiver::OnInt16Event(const int16 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastInt16Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到16位整数事件: %d"), Value);
}

void UGameEventTestReceiver::OnUInt16Event(const uint16 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastUInt16Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到无符号16位整数事件: %u"), Value);
}

void UGameEventTestReceiver::OnInt32Event(const int32 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastReceivedInt = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到32位整数事件: %d"), Value);
}

void UGameEventTestReceiver::OnUInt32Event(const uint32 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	UE_LOG(LogTemp, Log, TEXT("📊 接收到无符号32位整数事件: %u"), Value);
}

void UGameEventTestReceiver::OnInt64Event(const int64 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastInt64Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到64位整数事件: %lld"), Value);
}

void UGameEventTestReceiver::OnUInt64Event(const uint64 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastUInt64Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到无符号64位整数事件: %llu"), Value);
}

void UGameEventTestReceiver::OnFloatEvent(const float Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastReceivedFloat = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到浮点数事件: %f"), Value);
}

void UGameEventTestReceiver::OnDoubleEvent(const double Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastDoubleValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到双精度浮点数事件: %lf"), Value);
}

// ========================================
// 字符串类型事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnStringEvent(const FString& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastReceivedString = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到字符串事件: %s"), *Value);
}

void UGameEventTestReceiver::OnNameEvent(const FName& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastNameValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到名称事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnTextEvent(const FText& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastTextValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到文本事件: %s"), *Value.ToString());
}

// ========================================
// 数学类型事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnVectorEvent(const FVector& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastVectorValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到向量事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnVector2DEvent(const FVector2D& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastVector2DValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到2D向量事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnVector4Event(const FVector4& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastVector4Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到4D向量事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnRotatorEvent(const FRotator& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastRotatorValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到旋转器事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnQuatEvent(const FQuat& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastQuatValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到四元数事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnTransformEvent(const FTransform& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastTransformValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到变换事件: 位置=%s"), *Value.GetLocation().ToString());
}

void UGameEventTestReceiver::OnColorEvent(const FColor& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastColorValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到颜色事件: R=%d G=%d B=%d A=%d"), Value.R, Value.G, Value.B, Value.A);
}

void UGameEventTestReceiver::OnLinearColorEvent(const FLinearColor& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastLinearColorValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到线性颜色事件: R=%.3f G=%.3f B=%.3f A=%.3f"), Value.R, Value.G, Value.B, Value.A);
}

// ========================================
// 自定义类型事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnEnumEvent(ETestEnum Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastEnumValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到枚举事件: %d"), (int32)Value);
}

void UGameEventTestReceiver::OnPriorityEvent(EGameEventPriority Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastPriorityValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到优先级事件: %d"), (int32)Value);
}

void UGameEventTestReceiver::OnStateEvent(EGameEventState Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastStateValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到状态事件: %d"), (int32)Value);
}

void UGameEventTestReceiver::OnNonBlueprintEnum64(const ENonBlueprintEnum64 Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastEnum64Value = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到枚举事件: %lld"), LastEnum64Value);
}

void UGameEventTestReceiver::OnSimpleStructEvent(const FSimpleTestStruct& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastSimpleStructValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到简单结构体事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnComplexStructEvent(const FComplexTestStruct& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastComplexStructValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到复杂结构体事件: %s"), *Value.ToString());
}

void UGameEventTestReceiver::OnNestedStructEvent(const FNestedContainerStruct& Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastNestedStructValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到嵌套结构体事件: %s"), *Value.ToString());
}

// ========================================
// 容器类型事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnIntArrayEvent(const TArray<int32>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastIntArrayValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到整数数组事件: 数量=%d"), Values.Num());
}

void UGameEventTestReceiver::OnStringArrayEvent(const TArray<FString>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastStringArrayValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到字符串数组事件: 数量=%d"), Values.Num());
}

void UGameEventTestReceiver::OnIntSetEvent(const TSet<int32>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastIntSetValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到整数集合事件: 数量=%d"), Values.Num());
}

void UGameEventTestReceiver::OnStringToIntMapEvent(const TMap<FString, int32>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastStringToIntMapValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到字符串到整数映射事件: 数量=%d"), Values.Num());
}

void UGameEventTestReceiver::OnIntToFloatMapEvent(const TMap<int32, float>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastIntToFloatMapValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到整数到浮点数映射事件: 数量=%d"), Values.Num());
}

// ========================================
// 嵌套容器类型事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnNestedIntArrayEvent(const TArray<TArray<int32>>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastNestedIntArrayValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到嵌套整数数组事件: 外层数量=%d"), Values.Num());
}

void UGameEventTestReceiver::OnStringToFloatArrayMapEvent(const TMap<FString, TArray<float>>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastStringToFloatArrayMapValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到字符串到浮点数组映射事件: 数量=%d"), Values.Num());
}

void UGameEventTestReceiver::OnStructArrayEvent(const TArray<FSimpleTestStruct>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastStructArrayValue = Values;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到结构体数组事件: 数量=%d"), Values.Num());
}

// ========================================
// 多参数事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnMultiParamEvent(const int32 IntValue, const FString& StringValue, const bool bBoolValue)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastMultiParamValues.IntValue = IntValue;
	LastMultiParamValues.StringValue = StringValue;
	LastMultiParamValues.bBoolValue = bBoolValue;
	UE_LOG(LogTemp,
	       Log,
	       TEXT("📊 接收到多参数事件: 整数=%d, 字符串=%s, 布尔=%s"),
	       IntValue,
	       *StringValue,
	       bBoolValue ? TEXT("真") : TEXT("假"));
}

void UGameEventTestReceiver::OnComplexMultiParamEvent(const FVector& Position, EGameEventPriority Priority, const TArray<int32>& Values)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastComplexMultiParamValues.Position = Position;
	LastComplexMultiParamValues.Priority = Priority;
	LastComplexMultiParamValues.Values = Values;
	UE_LOG(LogTemp,
	       Log,
	       TEXT("📊 接收到复杂多参数事件: 位置=%s, 优先级=%d, 数组大小=%d"),
	       *Position.ToString(),
	       (int32)Priority,
	       Values.Num());
}

// ========================================
// 无参数事件处理函数实现
// ========================================

void UGameEventTestReceiver::OnSimpleEvent()
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	UE_LOG(LogTemp, Log, TEXT("📊 接收到简单无参数事件"));
}

// ========================================
// 边界情况处理函数实现
// ========================================

void UGameEventTestReceiver::OnBoundaryEnumEvent(EBoundaryEnum Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastBoundaryEnumValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到边界枚举事件: %d"), (int32)Value);
}

void UGameEventTestReceiver::OnNonBlueprintEnumEvent(ENonBlueprintEnum Value)
{
	EventReceivedCount++;
	AtomicEventCount.fetch_add(1);
	LastNonBlueprintEnumValue = Value;
	UE_LOG(LogTemp, Log, TEXT("📊 接收到非蓝图枚举事件: %d"), (int32)Value);
}

// ========================================
// FGameEventTestHelper 实现
// ========================================

UGameEventTestReceiver* FGameEventTestHelper::CreateTestReceiver(UObject* Outer)
{
	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	UGameEventTestReceiver* TestReceiver = NewObject<UGameEventTestReceiver>(Outer);
	TestReceiver->ResetTestState();
	return TestReceiver;
}

UComplexTestObject* FGameEventTestHelper::CreateComplexTestObject(UObject* Outer)
{
	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	return NewObject<UComplexTestObject>(Outer);
}

UObject* FGameEventTestHelper::CreateTestWorldContext()
{
	if (GEngine && GEngine->GetWorldContexts().Num() > 0)
	{
		return GEngine->GetWorldContexts()[0].World();
	}
	return nullptr;
}

void FGameEventTestHelper::CleanupTestEnvironment()
{
	if (TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get())
	{
		EventManager->Clear();
	}
}

bool FGameEventTestHelper::VerifyEventReceived(const UGameEventTestReceiver* Receiver, const int32 ExpectedCount)
{
	if (!Receiver)
	{
		return false;
	}
	return Receiver->EventReceivedCount >= ExpectedCount;
}

void FGameEventTestHelper::WaitForEventProcessing(const float MaxWaitTime)
{
	const double StartTime = FPlatformTime::Seconds();
	while (FPlatformTime::Seconds() - StartTime < MaxWaitTime)
	{
		FPlatformProcess::Sleep(0.001f); // 1毫秒
	}
}

double FGameEventTestHelper::MeasureEventPerformance(const TFunction<void()>& TestFunction, const int32 Iterations)
{
	if (Iterations <= 0)
	{
		return 0.0;
	}

	double TotalTime = 0.0;

	for (int32 i = 0; i < Iterations; ++i)
	{
		double StartTime = FPlatformTime::Seconds();
		TestFunction();
		double EndTime = FPlatformTime::Seconds();
		TotalTime += EndTime - StartTime;
	}

	return TotalTime / Iterations * 1000.0; // 转换为毫秒
}

FSimpleTestStruct FGameEventTestHelper::CreateTestSimpleStruct(const int32 IntValue, const FString& StringValue, const bool bBoolValue)
{
	return FSimpleTestStruct(IntValue, StringValue, bBoolValue);
}

FComplexTestStruct FGameEventTestHelper::CreateTestComplexStruct(const int32 Id, const FString& Name, const EGameEventPriority Priority)
{
	FComplexTestStruct Result;
	Result.Id = Id;
	Result.Name = Name;
	Result.Priority = Priority;
	Result.Position = FVector(Id * 10.0f, Id * 20.0f, Id * 30.0f);

	// 添加一些测试数据
	for (int32 i = 0; i < 5; ++i)
	{
		Result.Values.Add(Id + i);
	}

	Result.Properties.Add(TEXT("测试属性1"), Id * 1.5f);
	Result.Properties.Add(TEXT("测试属性2"), Id * 2.5f);

	return Result;
}

FNestedContainerStruct FGameEventTestHelper::CreateTestNestedStruct()
{
	FNestedContainerStruct Result;

	// 创建嵌套整数数组
	for (int32 i = 0; i < 3; ++i)
	{
		TArray<int32> InnerArray;
		for (int32 j = 0; j < 4; ++j)
		{
			InnerArray.Add(i * 10 + j);
		}
		Result.NestedIntArrays.Add(InnerArray);
	}

	// 创建字符串到浮点数组的映射
	for (int32 i = 0; i < 3; ++i)
	{
		FString Key = FString::Printf(TEXT("键_%d"), i);
		TArray<float> FloatArray;
		for (int32 j = 0; j < 3; ++j)
		{
			FloatArray.Add(i * 1.5f + j * 0.5f);
		}
		Result.StringToFloatArrayMap.Add(Key, FloatArray);
	}

	// 创建结构体数组
	for (int32 i = 0; i < 4; ++i)
	{
		FSimpleTestStruct TestStruct;
		TestStruct.IntValue = i * 100;
		TestStruct.StringValue = FString::Printf(TEXT("嵌套结构体_%d"), i);
		TestStruct.bBoolValue = i % 2 == 0;
		Result.StructArray.Add(TestStruct);
	}

	return Result;
}

bool FGameEventTestHelper::AreEqual(const FSimpleTestStruct& A, const FSimpleTestStruct& B)
{
	return A == B;
}

bool FGameEventTestHelper::AreEqual(const FComplexTestStruct& A, const FComplexTestStruct& B)
{
	return A == B;
}

TArray<int32> FGameEventTestHelper::GenerateRandomIntArray(const int32 Size)
{
	TArray<int32> Result;
	for (int32 i = 0; i < Size; ++i)
	{
		Result.Add(FMath::RandRange(-1000, 1000));
	}
	return Result;
}

TArray<FString> FGameEventTestHelper::GenerateRandomStringArray(const int32 Size)
{
	TArray<FString> Result;
	for (int32 i = 0; i < Size; ++i)
	{
		Result.Add(FString::Printf(TEXT("随机字符串_%d_%d"), i, FMath::RandRange(1, 9999)));
	}
	return Result;
}

TMap<FString, int32> FGameEventTestHelper::GenerateRandomStringToIntMap(const int32 Size)
{
	TMap<FString, int32> Result;
	for (int32 i = 0; i < Size; ++i)
	{
		FString Key = FString::Printf(TEXT("键_%d"), i);
		int32 Value = FMath::RandRange(-100, 100);
		Result.Add(Key, Value);
	}
	return Result;
}

void FGameEventTestHelper::RunMultiThreadTest(TFunction<void()> TestFunction, const int32 ThreadCount, int32 IterationsPerThread)
{
	TArray<TFuture<void>> Futures;

	for (int32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
	{
		TFuture<void> Future = Async(EAsyncExecution::Thread,
		                             [TestFunction, IterationsPerThread]
		                             {
			                             for (int32 i = 0; i < IterationsPerThread; ++i)
			                             {
				                             TestFunction();
			                             }
		                             });

		Futures.Add(MoveTemp(Future));
	}

	// 等待所有线程完成
	for (auto& Future : Futures)
	{
		Future.Wait();
	}
}

bool FGameEventTestHelper::RunStressTest(const TFunction<void()>& TestFunction, const int32 TotalIterations, const float MaxTimeSeconds)
{
	const double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < TotalIterations; ++i)
	{
		TestFunction();

		// 检查是否超时
		if (FPlatformTime::Seconds() - StartTime > MaxTimeSeconds)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ 压力测试在 %d 次迭代后超时（最大时间: %.2f秒）"), i + 1, MaxTimeSeconds);
			return false;
		}
	}

	const double EndTime = FPlatformTime::Seconds();
	const double TotalTime = EndTime - StartTime;

	UE_LOG(LogTemp,
	       Log,
	       TEXT("✅ 压力测试完成: %d 次迭代，总时间: %.3f秒，平均每次: %.6f秒"),
	       TotalIterations,
	       TotalTime,
	       TotalTime / TotalIterations);

	return true;
}
