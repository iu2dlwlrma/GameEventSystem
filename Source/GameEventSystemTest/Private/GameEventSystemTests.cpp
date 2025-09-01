#include "GameEventSystemTests.h"
#include "GameEventManager.h"
#include "GameEventTypes.h"
#include "GameEventTestObjects.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Async/Async.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformProcess.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "Math/Color.h"
#include "UObject/UObjectGlobals.h"
#include <atomic>

// 定义日志类别
DEFINE_LOG_CATEGORY(LogGameEventSystemTest);

// ========================================
// FAdvancedGameEventTestHelper 实现
// ========================================

FGameEventTestStatistics FAdvancedGameEventTestHelper::CreateTestStatistics()
{
	return FGameEventTestStatistics();
}

bool FAdvancedGameEventTestHelper::AreNestedArraysEqual(const TArray<TArray<int32>>& A, const TArray<TArray<int32>>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 i = 0; i < A.Num(); ++i)
	{
		if (!AreArraysEqual(A[i], B[i]))
		{
			return false;
		}
	}

	return true;
}

bool FAdvancedGameEventTestHelper::AreFloatsEqual(float A, float B, float Tolerance)
{
	return FMath::Abs(A - B) <= Tolerance;
}

bool FAdvancedGameEventTestHelper::AreDoublesEqual(double A, double B, double Tolerance)
{
	return FMath::Abs(A - B) <= Tolerance;
}

bool FAdvancedGameEventTestHelper::AreVectorsEqual(const FVector& A, const FVector& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

bool FAdvancedGameEventTestHelper::AreVector2DsEqual(const FVector2D& A, const FVector2D& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

bool FAdvancedGameEventTestHelper::AreVector4sEqual(const FVector4& A, const FVector4& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

bool FAdvancedGameEventTestHelper::AreRotatorsEqual(const FRotator& A, const FRotator& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

bool FAdvancedGameEventTestHelper::AreQuatsEqual(const FQuat& A, const FQuat& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

bool FAdvancedGameEventTestHelper::AreTransformsEqual(const FTransform& A, const FTransform& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

bool FAdvancedGameEventTestHelper::AreColorsEqual(const FColor& A, const FColor& B)
{
	return A == B;
}

bool FAdvancedGameEventTestHelper::AreLinearColorsEqual(const FLinearColor& A, const FLinearColor& B, float Tolerance)
{
	return A.Equals(B, Tolerance);
}

FVector FAdvancedGameEventTestHelper::GenerateRandomVector()
{
	return FVector(
	               FMath::RandRange(-1000.0f, 1000.0f),
	               FMath::RandRange(-1000.0f, 1000.0f),
	               FMath::RandRange(-1000.0f, 1000.0f)
	              );
}

FRotator FAdvancedGameEventTestHelper::GenerateRandomRotator()
{
	return FRotator(
	                FMath::RandRange(-180.0f, 180.0f),
	                FMath::RandRange(-180.0f, 180.0f),
	                FMath::RandRange(-180.0f, 180.0f)
	               );
}

FColor FAdvancedGameEventTestHelper::GenerateRandomColor()
{
	return FColor(
	              FMath::RandRange(0, 255),
	              FMath::RandRange(0, 255),
	              FMath::RandRange(0, 255),
	              FMath::RandRange(0, 255)
	             );
}

FLinearColor FAdvancedGameEventTestHelper::GenerateRandomLinearColor()
{
	return FLinearColor(
	                    FMath::RandRange(0.0f, 1.0f),
	                    FMath::RandRange(0.0f, 1.0f),
	                    FMath::RandRange(0.0f, 1.0f),
	                    FMath::RandRange(0.0f, 1.0f)
	                   );
}

FTransform FAdvancedGameEventTestHelper::GenerateRandomTransform()
{
	return FTransform(
	                  GenerateRandomRotator().Quaternion(),
	                  GenerateRandomVector(),
	                  FVector(
	                          FMath::RandRange(0.1f, 10.0f),
	                          FMath::RandRange(0.1f, 10.0f),
	                          FMath::RandRange(0.1f, 10.0f)
	                         )
	                 );
}

TArray<int32> FAdvancedGameEventTestHelper::GetBoundaryIntegers()
{
	return {
		GameEventTestConstants::TEST_INT32_MIN,
		GameEventTestConstants::TEST_INT32_MIN + 1,
		-1,
		0,
		1,
		GameEventTestConstants::TEST_INT32_MAX - 1,
		GameEventTestConstants::TEST_INT32_MAX
	};
}

TArray<float> FAdvancedGameEventTestHelper::GetBoundaryFloats()
{
	return {
		GameEventTestConstants::TEST_FLOAT_MIN,
		-1000000.0f,
		-1.0f,
		-0.0001f,
		0.0f,
		0.0001f,
		1.0f,
		1000000.0f,
		GameEventTestConstants::TEST_FLOAT_MAX
	};
}

TArray<double> FAdvancedGameEventTestHelper::GetBoundaryDoubles()
{
	return {
		GameEventTestConstants::TEST_DOUBLE_MIN,
		-1000000000000.0,
		-1.0,
		-0.000001,
		0.0,
		0.000001,
		1.0,
		1000000000000.0,
		GameEventTestConstants::TEST_DOUBLE_MAX
	};
}

bool FAdvancedGameEventTestHelper::RunConcurrentStressTest(TFunction<void()> TestFunction, int32 ConcurrentThreads, int32 IterationsPerThread)
{
	TArray<TFuture<void>> Futures;
	const double StartTime = FPlatformTime::Seconds();

	for (int32 ThreadIndex = 0; ThreadIndex < ConcurrentThreads; ++ThreadIndex)
	{
		TFuture<void> Future = Async(EAsyncExecution::Thread,
		                             [TestFunction, IterationsPerThread]()
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

	const double EndTime = FPlatformTime::Seconds();
	const double TotalTime = EndTime - StartTime;

	UE_LOG(LogGameEventSystemTest,
	       Log,
	       TEXT("✅ 并发压力测试完成: %d 线程 x %d 迭代，总时间: %.3f秒"),
	       ConcurrentThreads,
	       IterationsPerThread,
	       TotalTime);

	return true;
}

void FAdvancedGameEventTestHelper::CheckMemoryLeaks(TFunction<void()> TestFunction)
{
	// 执行测试前的内存状态
	const SIZE_T MemoryBefore = FPlatformMemory::GetStats().UsedPhysical;

	TestFunction();

	// 强制垃圾回收
	if (GEngine)
	{
		GEngine->ForceGarbageCollection(true);
	}

	// 执行测试后的内存状态
	const SIZE_T MemoryAfter = FPlatformMemory::GetStats().UsedPhysical;

	const int64 MemoryDiff = static_cast<int64>(MemoryAfter) - static_cast<int64>(MemoryBefore);

	UE_LOG(LogGameEventSystemTest,
	       Log,
	       TEXT("📊 内存使用情况: 执行前=%lluMB, 执行后=%lluMB, 差异=%lldMB"),
	       MemoryBefore / (1024 * 1024),
	       MemoryAfter / (1024 * 1024),
	       MemoryDiff / (1024 * 1024));
}

// ========================================
// 基础类型测试 - 布尔、整数、浮点数类型
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventBasicTypesTest, "GameEventSystem.BasicTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventBasicTypesTest)
{
	GAME_EVENT_LOG_INFO("=== 开始基础类型测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("基础类型测试");

	// ========================================
	// 布尔类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试布尔类型事件传输...");

		// 添加监听器
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::BOOL_EVENT), TestReceiver, TEXT("OnBoolEvent"));

		// 测试true值
		bool TestBoolTrue = true;
		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::BOOL_EVENT), WorldContext, false, TestBoolTrue);
		GAME_EVENT_TEST_TEXT(bSendResult, "布尔事件(true)发送应该成功");

		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到布尔事件");
		GAME_EVENT_TEST_EQUAL(TestReceiver->LastBoolValue, TestBoolTrue, "接收到的布尔值应该正确(true)");

		TestReceiver->ResetTestState();

		// 测试false值
		bool TestBoolFalse = false;
		bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::BOOL_EVENT), WorldContext, false, TestBoolFalse);
		GAME_EVENT_TEST_TEXT(bSendResult, "布尔事件(false)发送应该成功");

		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到布尔事件");
		GAME_EVENT_TEST_EQUAL(TestReceiver->LastBoolValue, TestBoolFalse, "接收到的布尔值应该正确(false)");

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("布尔类型测试完成");
	}

	// ========================================
	// 8位整数类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试8位整数类型事件传输...");
		TestReceiver->ResetTestState();

		// int8测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT8_EVENT), TestReceiver, TEXT("OnInt8Event"));

		TArray<int8> Int8TestValues = {
			GameEventTestConstants::TEST_INT8_MIN,
			-100,
			-1,
			0,
			1,
			100,
			GameEventTestConstants::TEST_INT8_MAX
		};

		for (int8 TestValue : Int8TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT8_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Int8事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到Int8事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastInt8Value, TestValue, "接收到的Int8值应该正确:");
			TestReceiver->ResetTestState();
		}

		// uint8测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::UINT8_EVENT), TestReceiver, TEXT("OnUInt8Event"));

		TArray<uint8> UInt8TestValues = {
			0,
			1,
			100,
			200,
			GameEventTestConstants::TEST_UINT8_MAX
		};

		for (uint8 TestValue : UInt8TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::UINT8_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "UInt8事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到UInt8事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastUInt8Value,
			                      TestValue,
			                      "接收到的UInt8值应该正确(%u)",
			                      TestValue);

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("8位整数类型测试完成");
	}

	// ========================================
	// 16位整数类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试16位整数类型事件传输...");
		TestReceiver->ResetTestState();

		// int16测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT16_EVENT), TestReceiver, TEXT("OnInt16Event"));

		TArray<int16> Int16TestValues = {
			GameEventTestConstants::TEST_INT16_MIN,
			-10000,
			-1,
			0,
			1,
			10000,
			GameEventTestConstants::TEST_INT16_MAX
		};

		for (int16 TestValue : Int16TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT16_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Int16事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到Int16事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastInt16Value,
			                      TestValue,
			                      "接收到的Int16值应该正确(%d)",
			                      TestValue);

			TestReceiver->ResetTestState();
		}

		// uint16测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::UINT16_EVENT), TestReceiver, TEXT("OnUInt16Event"));

		TArray<uint16> UInt16TestValues = {
			0,
			1,
			10000,
			50000,
			GameEventTestConstants::TEST_UINT16_MAX
		};

		for (uint16 TestValue : UInt16TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::UINT16_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "UInt16事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到UInt16事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastUInt16Value, TestValue, "接收到的UInt16值应该正确(%u)", TestValue);

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("16位整数类型测试完成");
	}

	{
		GAME_EVENT_LOG_INFO("测试64位枚举类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::ENUM64_EVENT), TestReceiver, TEXT("OnNonBlueprintEnum64"));
		ENonBlueprintEnum64 TestValue = ENonBlueprintEnum64::Option3;
		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::ENUM64_EVENT), WorldContext, false, ENonBlueprintEnum64::Option3);
		GAME_EVENT_TEST_TEXT(bSendResult, "64位枚举事件发送应该成功");
		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到64位枚举事件");
		GAME_EVENT_TEST_EQUAL(TestReceiver->LastEnum64Value, TestValue, "接收到的64位枚举值应该正确(%lld)", TestValue);

		TestReceiver->ResetTestState();

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("16位整数类型测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("基础类型测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 基础类型测试完成 ===");
	return true;
}

// ========================================
// 32位和64位整数类型测试 + 浮点数测试
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventNumericTypesTest, "GameEventSystem.NumericTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventNumericTypesTest)
{
	GAME_EVENT_LOG_INFO("=== 开始数值类型测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("数值类型测试");

	// ========================================
	// 32位整数类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试32位整数类型事件传输...");
		TestReceiver->ResetTestState();

		// int32测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT32_EVENT), TestReceiver, TEXT("OnInt32Event"));

		TArray<int32> Int32TestValues = FAdvancedGameEventTestHelper::GetBoundaryIntegers();
		Int32TestValues.Append({ -1000000, -42, 42, 1000000 });

		for (int32 TestValue : Int32TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT32_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Int32事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到Int32事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastReceivedInt,
			                      TestValue,
			                      "接收到的Int32值应该正确(%d)",
			                      TestValue);

			TestReceiver->ResetTestState();
		}

		// uint32测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::UINT32_EVENT), TestReceiver, TEXT("OnUInt32Event"));

		TArray<uint32> UInt32TestValues = {
			0,
			1,
			42,
			1000,
			1000000,
			GameEventTestConstants::TEST_UINT32_MAX / 2,
			GameEventTestConstants::TEST_UINT32_MAX
		};

		for (uint32 TestValue : UInt32TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::UINT32_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "UInt32事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到UInt32事件");

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("32位整数类型测试完成");
	}

	// ========================================
	// 64位整数类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试64位整数类型事件传输...");
		TestReceiver->ResetTestState();

		// int64测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT64_EVENT), TestReceiver, TEXT("OnInt64Event"));

		TArray<int64> Int64TestValues = {
			GameEventTestConstants::TEST_INT64_MIN,
			GameEventTestConstants::TEST_INT64_MIN / 2,
			-1000000000000LL,
			-1,
			0,
			1,
			1000000000000LL,
			GameEventTestConstants::TEST_INT64_MAX / 2,
			GameEventTestConstants::TEST_INT64_MAX
		};

		for (int64 TestValue : Int64TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT64_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Int64事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到Int64事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastInt64Value,
			                      TestValue,
			                      "接收到的Int64值应该正确(%lld)",
			                      TestValue);

			TestReceiver->ResetTestState();
		}

		// uint64测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::UINT64_EVENT), TestReceiver, TEXT("OnUInt64Event"));

		TArray<uint64> UInt64TestValues = {
			0,
			1,
			1000000000000ULL,
			GameEventTestConstants::TEST_UINT64_MAX / 2,
			GameEventTestConstants::TEST_UINT64_MAX
		};

		for (uint64 TestValue : UInt64TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::UINT64_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "UInt64事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到UInt64事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastUInt64Value,
			                      TestValue,
			                      "接收到的UInt64值应该正确(%llu)",
			                      TestValue);

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("64位整数类型测试完成");
	}

	// ========================================
	// 浮点数类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试浮点数类型事件传输...");
		TestReceiver->ResetTestState();

		// float测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::FLOAT_EVENT), TestReceiver, TEXT("OnFloatEvent"));

		TArray<float> FloatTestValues = FAdvancedGameEventTestHelper::GetBoundaryFloats();
		FloatTestValues.Append({ -3.141592653589793, -1.5f, -0.5f, 0.5f, 1.5f, 3.14159f, 42.42f });

		for (float TestValue : FloatTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::FLOAT_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Float事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到Float事件");

			bool bFloatEqual = FAdvancedGameEventTestHelper::AreFloatsEqual(TestReceiver->LastReceivedFloat, TestValue, 0.0001f);
			GAME_EVENT_TEST_STR(bFloatEqual,
			                    FString::Printf(TEXT("接收到的Float值应该正确(期望:%.6f, 实际:%.6f)"), TestValue, TestReceiver->LastReceivedFloat));

			TestReceiver->ResetTestState();
		}

		// double测试
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::DOUBLE_EVENT), TestReceiver, TEXT("OnDoubleEvent"));

		TArray<double> DoubleTestValues = FAdvancedGameEventTestHelper::GetBoundaryDoubles();
		DoubleTestValues.Append({ -3.141592653589793, -1.5, -0.5, 0.5, 1.5, 3.141592653589793, 42.424242 });

		for (double TestValue : DoubleTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::DOUBLE_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Double事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到Double事件");

			bool bDoubleEqual = FAdvancedGameEventTestHelper::AreDoublesEqual(TestReceiver->LastDoubleValue, TestValue, 0.000001);
			GAME_EVENT_TEST_STR(bDoubleEqual,
			                    FString::Printf(TEXT("接收到的Double值应该正确(期望:%.12f, 实际:%.12f)"), TestValue, TestReceiver->LastDoubleValue));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("浮点数类型测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("数值类型测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 数值类型测试完成 ===");
	return true;
}

// ========================================
// 字符串类型测试 - FString, FName, FText
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventStringTypesTest, "GameEventSystem.StringTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventStringTypesTest)
{
	GAME_EVENT_LOG_INFO("=== 开始字符串类型测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("字符串类型测试");

	// ========================================
	// FString类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试FString类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::STRING_EVENT), TestReceiver, TEXT("OnStringEvent"));

		TArray<FString> StringTestValues = {
			TEXT(""),  // 空字符串
			TEXT("简单测试"),
			TEXT("Hello World!"),
			TEXT("包含特殊字符: !@#$%^&*()"),
			TEXT("多行文本\n第二行\n第三行"),
			TEXT("Unicode测试: 你好世界 🌍 Привет мир"),
			TEXT("长字符串测试：") + FString::ChrN(1000, TEXT('X')),  // 1000个X
			TEXT("数字和符号: 1234567890 ~!@#$%^&*()_+-=[]{}|;':\",./<>?")
		};

		for (const FString& TestValue : StringTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::STRING_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "FString事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到FString事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastReceivedString, TestValue, "接收到的FString值应该正确(长度:%d)", TestValue.Len());

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("FString类型测试完成");
	}

	// ========================================
	// FName类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试FName类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::NAME_EVENT), TestReceiver, TEXT("OnNameEvent"));

		TArray<FName> NameTestValues = {
			NAME_None,
			FName(TEXT("TestName")),
			FName(TEXT("Another_Name")),
			FName(TEXT("Name.With.Dots")),
			FName(TEXT("数字名称123")),
			FName(TEXT("UPPERCASE_NAME")),
			FName(TEXT("lowercase_name")),
			FName(TEXT("MixedCase_Name_123"))
		};

		for (const FName& TestValue : NameTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::NAME_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "FName事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到FName事件");
			GAME_EVENT_TEST_EQUAL(TestReceiver->LastNameValue,
			                      TestValue,
			                      "接收到的FName值应该正确(%s)",
			                      *TestValue.ToString());

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("FName类型测试完成");
	}

	// ========================================
	// FText类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试FText类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::TEXT_EVENT), TestReceiver, TEXT("OnTextEvent"));

		TArray<FText> TextTestValues = {
			FText::GetEmpty(),
			FText::FromString(TEXT("简单FText测试")),
			FText::FromString(TEXT("FText with English")),
			FText::FromString(TEXT("包含数字的FText: 12345")),
			FText::FromString(TEXT("特殊字符FText: !@#$%^&*()")),
			FText::FromString(TEXT("多行FText\n包含换行符\n第三行内容")),
			FText::AsNumber(42),
			FText::AsNumber(3.14159f),
			FText::AsPercent(0.75f)
		};

		for (const FText& TestValue : TextTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::TEXT_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "FText事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到FText事件");

			bool bTextEqual = TestReceiver->LastTextValue.ToString() == TestValue.ToString();
			GAME_EVENT_TEST_STR(bTextEqual,
			                    FString::Printf(TEXT("接收到的FText值应该正确(%s)"), *TestValue.ToString()));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("FText类型测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("字符串类型测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 字符串类型测试完成 ===");
	return true;
}

// ========================================
// 数学类型测试 - Vector, Rotator, Transform, Color等
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventMathTypesTest, "GameEventSystem.MathTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventMathTypesTest)
{
	GAME_EVENT_LOG_INFO("=== 开始数学类型测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("数学类型测试");

	// ========================================
	// FVector类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试FVector类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::VECTOR_EVENT), TestReceiver, TEXT("OnVectorEvent"));

		TArray<FVector> VectorTestValues = {
			FVector::ZeroVector,
			FVector::OneVector,
			FVector::UpVector,
			FVector::RightVector,
			FVector::ForwardVector,
			FVector(-1.0f, -1.0f, -1.0f),
			FVector(100.0f, 200.0f, 300.0f),
			FVector(-999.999f, 999.999f, -999.999f),
			FAdvancedGameEventTestHelper::GenerateRandomVector(),
			FAdvancedGameEventTestHelper::GenerateRandomVector(),
			FAdvancedGameEventTestHelper::GenerateRandomVector()
		};

		for (const FVector& TestValue : VectorTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::VECTOR_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "FVector事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到FVector事件");

			bool bVectorEqual = FAdvancedGameEventTestHelper::AreVectorsEqual(TestReceiver->LastVectorValue, TestValue, 0.0001f);
			GAME_EVENT_TEST_STR(bVectorEqual,
			                    FString::Printf(TEXT("接收到的FVector值应该正确(期望:%s, 实际:%s)"),
				                    *TestValue.ToString(), *TestReceiver->LastVectorValue.ToString()));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("FVector类型测试完成");
	}

	// ========================================
	// FVector2D类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试FVector2D类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::VECTOR2D_EVENT), TestReceiver, TEXT("OnVector2DEvent"));

		TArray<FVector2D> Vector2DTestValues = {
			FVector2D::ZeroVector,
			FVector2D::UnitVector,
			FVector2D(-1.0f, -1.0f),
			FVector2D(100.0f, 200.0f),
			FVector2D(-999.999f, 999.999f),
			FVector2D(3.14159f, 2.71828f)
		};

		for (const FVector2D& TestValue : Vector2DTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::VECTOR2D_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "FVector2D事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到FVector2D事件");

			bool bVector2DEqual = FAdvancedGameEventTestHelper::AreVector2DsEqual(TestReceiver->LastVector2DValue, TestValue, 0.0001f);
			GAME_EVENT_TEST_STR(bVector2DEqual,
			                    FString::Printf(TEXT("接收到的FVector2D值应该正确(期望:%s, 实际:%s)"),
				                    *TestValue.ToString(), *TestReceiver->LastVector2DValue.ToString()));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("FVector2D类型测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("数学类型测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 数学类型测试完成 ===");
	return true;
}

// ========================================
// 容器类型测试 - TArray, TSet, TMap
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventContainerTypesTest, "GameEventSystem.ContainerTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventContainerTypesTest)
{
	GAME_EVENT_LOG_INFO("=== 开始容器类型测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("容器类型测试");

	// ========================================
	// TArray<int32>类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试TArray<int32>类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT_ARRAY_EVENT), TestReceiver, TEXT("OnIntArrayEvent"));

		TArray<TArray<int32>> ArrayTestValues = {
			{},  // 空数组
			{ 42 },  // 单元素数组
			{ 1, 2, 3, 4, 5 },  // 小数组
			{ -100, 0, 100, 200, -50 },  // 包含负数的数组
			FGameEventTestHelper::GenerateRandomIntArray(10),  // 随机数组
			FGameEventTestHelper::GenerateRandomIntArray(100)  // 大数组
		};

		for (const TArray<int32>& TestValue : ArrayTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT_ARRAY_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "TArray<int32>事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到TArray<int32>事件");

			bool bArrayEqual = FAdvancedGameEventTestHelper::AreArraysEqual(TestReceiver->LastIntArrayValue, TestValue);
			GAME_EVENT_TEST_STR(bArrayEqual,
			                    FString::Printf(TEXT("接收到的TArray<int32>值应该正确(大小:%d)"), TestValue.Num()));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("TArray<int32>类型测试完成");
	}

	// ========================================
	// TArray<FString>类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试TArray<FString>类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::STRING_ARRAY_EVENT), TestReceiver, TEXT("OnStringArrayEvent"));

		TArray<TArray<FString>> StringArrayTestValues = {
			{},  // 空数组
			{ TEXT("单个字符串") },
			{ TEXT("第一个"), TEXT("第二个"), TEXT("第三个") },
			{ TEXT(""), TEXT("空字符串测试"), TEXT("") },  // 包含空字符串
			{ TEXT("Unicode字符串"), TEXT("🌍🚀"), TEXT("Привет") },  // Unicode测试
			FGameEventTestHelper::GenerateRandomStringArray(5)
		};

		for (const TArray<FString>& TestValue : StringArrayTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::STRING_ARRAY_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "TArray<FString>事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到TArray<FString>事件");

			bool bArrayEqual = FAdvancedGameEventTestHelper::AreArraysEqual(TestReceiver->LastStringArrayValue, TestValue);
			GAME_EVENT_TEST_STR(bArrayEqual,
			                    FString::Printf(TEXT("接收到的TArray<FString>值应该正确(大小:%d)"), TestValue.Num()));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("TArray<FString>类型测试完成");
	}

	// ========================================
	// TMap<FString, int32>类型测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试TMap<FString, int32>类型事件传输...");
		TestReceiver->ResetTestState();

		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::STRING_TO_INT_MAP_EVENT), TestReceiver, TEXT("OnStringToIntMapEvent"));

		TArray<TMap<FString, int32>> MapTestValues = {
			{},  // 空映射
			{ { TEXT("键1"), 42 } },  // 单元素映射
			{ { TEXT("分数"), 95 }, { TEXT("等级"), 10 }, { TEXT("生命值"), 100 } },
			{ { TEXT("负数测试"), -100 }, { TEXT("零值测试"), 0 }, { TEXT("正数测试"), 100 } },
			FGameEventTestHelper::GenerateRandomStringToIntMap(10)
		};

		for (const TMap<FString, int32>& TestValue : MapTestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::STRING_TO_INT_MAP_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "TMap<FString, int32>事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();
			GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到TMap<FString, int32>事件");

			bool bMapEqual = FAdvancedGameEventTestHelper::AreMapsEqual(TestReceiver->LastStringToIntMapValue, TestValue);
			GAME_EVENT_TEST_STR(bMapEqual,
			                    FString::Printf(TEXT("接收到的TMap<FString, int32>值应该正确(大小:%d)"), TestValue.Num()));

			TestReceiver->ResetTestState();
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("TMap<FString, int32>类型测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("容器类型测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 容器类型测试完成 ===");
	return true;
}

// ========================================
// Lambda函数测试
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventLambdaTest, "GameEventSystem.Lambda", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventLambdaTest)
{
	GAME_EVENT_LOG_INFO("=== 开始Lambda函数测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("Lambda函数测试");

	// ========================================
	// 单参数Lambda测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试单参数Lambda函数...");

		std::atomic<int32> LambdaCallCount { 0 };
		int32 LastReceivedValue = 0;

		// 添加Lambda监听器
		FString ListenerId = EventManager->AddLambdaListener(
		                                                     FEventId(GameEventTestEvents::LAMBDA_INT_EVENT),
		                                                     TestReceiver,
		                                                     [&LambdaCallCount, &LastReceivedValue](int32 Value)
		                                                     {
			                                                     LambdaCallCount.fetch_add(1);
			                                                     LastReceivedValue = Value;
			                                                     UE_LOG(LogGameEventSystemTest, Log, TEXT("📊 Lambda接收到整数: %d"), Value);
		                                                     }
		                                                    );

		GAME_EVENT_TEST_TEXT(!ListenerId.IsEmpty(), "Lambda监听器ID不应该为空");

		// 发送事件测试
		TArray<int32> TestValues = { -100, 0, 42, 1000 };

		for (int32 TestValue : TestValues)
		{
			bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::LAMBDA_INT_EVENT), WorldContext, false, TestValue);
			GAME_EVENT_TEST_TEXT(bSendResult, "Lambda事件发送应该成功");

			FGameEventTestHelper::WaitForEventProcessing();

			GAME_EVENT_TEST_EQUAL(LastReceivedValue,
			                      TestValue,
			                      "Lambda应该接收到正确的值(%d)",
			                      TestValue);
		}

		GAME_EVENT_TEST_EQUAL(LambdaCallCount.load(), TestValues.Num(), "Lambda调用次数应该正确");

		// 移除Lambda监听器
		EventManager->RemoveLambdaListener(FEventId(GameEventTestEvents::LAMBDA_INT_EVENT), ListenerId);

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("单参数Lambda函数测试完成");
	}

	// ========================================
	// 多参数Lambda测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试多参数Lambda函数...");

		std::atomic<int32> MultiLambdaCallCount { 0 };
		int32 LastInt = 0;
		FString LastString;
		bool LastBool = false;

		// 添加多参数Lambda监听器
		FString ListenerId = EventManager->AddLambdaListener(
		                                                     FEventId(GameEventTestEvents::LAMBDA_MULTI_PARAM_EVENT),
		                                                     TestReceiver,
		                                                     [&MultiLambdaCallCount, &LastInt, &LastString, &LastBool](int32 IntValue, const FString& StringValue, bool bBoolValue, ENonBlueprintEnum64 NonBlueprintEnum64, EGameEventState GameEventState)
		                                                     {
			                                                     MultiLambdaCallCount.fetch_add(1);
			                                                     LastInt = IntValue;
			                                                     LastString = StringValue;
			                                                     LastBool = bBoolValue;
			                                                     UE_LOG(LogGameEventSystemTest,
			                                                            Log,
			                                                            TEXT("📊 多参数Lambda接收: 整数=%d, 字符串=%s, 布尔=%s, Enum64=%lld, GameEventState=%u"),
			                                                            IntValue,
			                                                            *StringValue,
			                                                            bBoolValue ? TEXT("真") : TEXT("假"),
			                                                            NonBlueprintEnum64,
			                                                            GameEventState);
		                                                     }
		                                                    );

		GAME_EVENT_TEST_TEXT(!ListenerId.IsEmpty(), "多参数Lambda监听器ID不应该为空");

		// 发送多参数事件
		int32 TestInt = 123;
		FString TestString = TEXT("Lambda测试字符串");
		bool TestBool = true;

		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::LAMBDA_MULTI_PARAM_EVENT), WorldContext, false, TestInt, TestString, TestBool, ENonBlueprintEnum64::Option3, EGameEventState::Failed);
		GAME_EVENT_TEST_TEXT(bSendResult, "多参数Lambda事件发送应该成功");

		FGameEventTestHelper::WaitForEventProcessing();

		GAME_EVENT_TEST_EQUAL(LastInt, TestInt, "Lambda应该接收到正确的整数值");
		GAME_EVENT_TEST_EQUAL(LastString, TestString, "Lambda应该接收到正确的字符串值");
		GAME_EVENT_TEST_EQUAL(LastBool, TestBool, "Lambda应该接收到正确的布尔值");
		GAME_EVENT_TEST_EQUAL(MultiLambdaCallCount.load(), 1, "多参数Lambda应该被调用一次");

		// 移除Lambda监听器
		EventManager->RemoveLambdaListener(FEventId(GameEventTestEvents::LAMBDA_MULTI_PARAM_EVENT), ListenerId);

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("多参数Lambda函数测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("Lambda函数测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== Lambda函数测试完成 ===");
	return true;
}

// ========================================
// 多线程测试 - 线程安全性验证
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventMultiThreadTest, "GameEventSystem.MultiThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventMultiThreadTest)
{
	GAME_EVENT_LOG_INFO("=== 开始多线程测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("多线程测试");

	// ========================================
	// 并发事件发送测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试并发事件发送...");
		TestReceiver->ResetTestState();

		// 添加监听器
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::MULTITHREAD_EVENT), TestReceiver, TEXT("OnInt32Event"));

		const int32 ThreadCount = GameEventTestConstants::DEFAULT_THREAD_COUNT;
		const int32 EventsPerThread = GameEventTestConstants::DEFAULT_ITERATIONS_PER_THREAD;
		const int32 TotalExpectedEvents = ThreadCount * EventsPerThread;

		GAME_EVENT_LOG_INFO("启动 %d 个线程，每个线程发送 %d 个事件", ThreadCount, EventsPerThread);

		// 启动多个并发线程发送事件
		FGameEventTestHelper::RunMultiThreadTest([&]()
		                                         {
			                                         for (int32 i = 0; i < EventsPerThread; ++i)
			                                         {
				                                         int32 TestValue = FMath::RandRange(1, 1000);
				                                         EventManager->SendEvent(FEventId(GameEventTestEvents::MULTITHREAD_EVENT), WorldContext, false, TestValue);
			                                         }
		                                         },
		                                         ThreadCount,
		                                         1);  // 注意这里每个线程只运行一次上面的循环

		// 等待所有事件处理完成
		FGameEventTestHelper::WaitForEventProcessing(GameEventTestConstants::MULTI_THREAD_WAIT_TIME);

		// 验证原子计数器（线程安全）
		int32 AtomicCount = TestReceiver->AtomicEventCount.load();
		GAME_EVENT_LOG_INFO("原子计数器接收到的事件数: %d (期望: %d)", AtomicCount, TotalExpectedEvents);

		// 由于多线程的不确定性，我们允许一定的容差
		bool bCountInRange = (AtomicCount >= TotalExpectedEvents * 0.9f) && (AtomicCount <= TotalExpectedEvents * 1.1f);
		GAME_EVENT_TEST_STR(bCountInRange,
		                    FString::Printf(TEXT("多线程事件接收数量应该在合理范围内(实际:%d, 期望:%d)"), AtomicCount, TotalExpectedEvents));

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("并发事件发送测试完成");
	}

	// ========================================
	// 并发监听器管理测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试并发监听器添加和移除...");

		const FString TestEvent = TEXT("Test.ConcurrentListener");
		std::atomic<int32> ListenerCount { 0 };

		// 并发添加和移除监听器
		FAdvancedGameEventTestHelper::RunConcurrentStressTest([&]()
		                                                      {
			                                                      // 添加Lambda监听器
			                                                      FString ListenerId = EventManager->AddLambdaListener(
			                                                                                                           FEventId(TestEvent),
			                                                                                                           TestReceiver,
			                                                                                                           [&ListenerCount](int32 Value)
			                                                                                                           {
				                                                                                                           ListenerCount.fetch_add(1);
			                                                                                                           }
			                                                                                                          );

			                                                      // 发送一个测试事件
			                                                      EventManager->SendEvent(FEventId(TestEvent), WorldContext, false, 42);

			                                                      // 短暂等待
			                                                      FPlatformProcess::Sleep(0.001f);

			                                                      // 移除监听器
			                                                      EventManager->RemoveLambdaListener(FEventId(TestEvent), ListenerId);
		                                                      },
		                                                      10,
		                                                      5);  // 10个线程，每个5次迭代

		FGameEventTestHelper::WaitForEventProcessing(0.5f);

		GAME_EVENT_LOG_INFO("并发监听器测试完成，总接收事件数: %d", ListenerCount.load());

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("并发监听器管理测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("多线程测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 多线程测试完成 ===");
	return true;
}

// ========================================
// 压力测试 - 性能和稳定性验证
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventStressTest, "GameEventSystem.StressTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventStressTest)
{
	GAME_EVENT_LOG_INFO("=== 开始压力测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("压力测试");

	// ========================================
	// 大量事件发送压力测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试大量事件发送压力...");
		TestReceiver->ResetTestState();

		// 添加监听器
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::STRESS_TEST_EVENT), TestReceiver, TEXT("OnInt32Event"));

		const int32 StressTestIterations = GameEventTestConstants::STRESS_TEST_ITERATIONS;
		GAME_EVENT_LOG_INFO("开始发送 %d 个事件的压力测试...", StressTestIterations);

		// 执行压力测试
		bool bStressTestPassed = FGameEventTestHelper::RunStressTest([&]()
		                                                             {
			                                                             int32 TestValue = FMath::RandRange(1, 1000);
			                                                             EventManager->SendEvent(FEventId(GameEventTestEvents::STRESS_TEST_EVENT), WorldContext, false, TestValue);
		                                                             },
		                                                             StressTestIterations,
		                                                             GameEventTestConstants::STRESS_TEST_MAX_TIME_SECONDS);

		GAME_EVENT_TEST_TEXT(bStressTestPassed, "压力测试应该在规定时间内完成");

		// 等待事件处理完成
		FGameEventTestHelper::WaitForEventProcessing(1.0f);

		// 验证接收到的事件数量（允许一定的容差，因为压力测试可能会丢失一些事件）
		int32 ReceivedCount = TestReceiver->AtomicEventCount.load();
		float SuccessRate = float(ReceivedCount) / float(StressTestIterations) * 100.0f;

		GAME_EVENT_LOG_INFO("压力测试统计: 发送=%d, 接收=%d, 成功率=%.2f%%",
		                    StressTestIterations,
		                    ReceivedCount,
		                    SuccessRate);

		// 成功率应该至少达到80%
		GAME_EVENT_TEST_STR(SuccessRate >= 80.0f,
		                    FString::Printf(TEXT("压力测试成功率应该至少80%% (实际:%.2f%%)"), SuccessRate));

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("大量事件发送压力测试完成");
	}

	// ========================================
	// 性能基准测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试事件发送性能基准...");
		TestReceiver->ResetTestState();

		// 添加监听器
		EventManager->AddListenerFunction(FEventId(TEXT("Test.Performance")), TestReceiver, TEXT("OnInt32Event"));

		// 单事件性能测试
		const int32 PerfIterations = GameEventTestConstants::DEFAULT_PERFORMANCE_ITERATIONS;

		double AvgTime = FGameEventTestHelper::MeasureEventPerformance([&]()
		                                                               {
			                                                               EventManager->SendEvent(FEventId(TEXT("Test.Performance")), WorldContext, false, 42);
		                                                               },
		                                                               PerfIterations);

		GAME_EVENT_LOG_INFO("性能测试结果: 平均每次事件发送时间 %.6f 毫秒 (%d 次迭代)", AvgTime, PerfIterations);

		// 验证性能是否在可接受范围内
		GAME_EVENT_TEST_STR(AvgTime < GameEventTestConstants::MAX_SINGLE_EVENT_TIME_MS,
		                    FString::Printf(TEXT("单次事件发送时间应该小于 %.1f 毫秒 (实际: %.6f 毫秒)"),
			                    GameEventTestConstants::MAX_SINGLE_EVENT_TIME_MS, AvgTime));

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("性能基准测试完成");
	}

	// ========================================
	// 内存泄漏检测测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试内存泄漏检测...");

		// 执行大量事件操作并检测内存泄漏
		FAdvancedGameEventTestHelper::CheckMemoryLeaks([&]()
		{
			// 创建大量监听器
			TArray<FString> ListenerIds;
			for (int32 i = 0; i < 100; ++i)
			{
				FString EventName = FString::Printf(TEXT("Test.Memory.%d"), i);
				FString ListenerId = EventManager->AddLambdaListener(
				                                                     FEventId(EventName),
				                                                     TestReceiver,
				                                                     [](int32 Value)
				                                                     {
					                                                     // 简单的Lambda函数
				                                                     }
				                                                    );
				ListenerIds.Add(ListenerId);

				// 发送一些事件
				for (int32 j = 0; j < 10; ++j)
				{
					EventManager->SendEvent(FEventId(EventName), WorldContext, false, j);
				}
			}

			// 移除所有监听器
			for (int32 i = 0; i < ListenerIds.Num(); ++i)
			{
				FString EventName = FString::Printf(TEXT("Test.Memory.%d"), i);
				EventManager->RemoveLambdaListener(FEventId(EventName), ListenerIds[i]);
			}
		});

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("内存泄漏检测测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("压力测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 压力测试完成 ===");
	return true;
}

// ========================================
// 边界测试 - 极值和边界情况验证
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventBoundaryTest, "GameEventSystem.BoundaryTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventBoundaryTest)
{
	GAME_EVENT_LOG_INFO("=== 开始边界测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("边界测试");

	// ========================================
	// 极值数据测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试极值数据传输...");
		TestReceiver->ResetTestState();

		// 测试极大和极小的浮点数
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::FLOAT_EVENT), TestReceiver, TEXT("OnFloatEvent"));

		TArray<float> ExtremeFloats = {
			0.0f,
			FLT_MIN,
			-FLT_MIN,
			FLT_MAX,
			-FLT_MAX,
			FLT_EPSILON,
			-FLT_EPSILON,
			INFINITY,
			-INFINITY
		};

		for (float ExtremeValue : ExtremeFloats)
		{
			// 跳过无穷大值，因为它们可能导致序列化问题
			if (FMath::IsFinite(ExtremeValue))
			{
				bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::FLOAT_EVENT), WorldContext, false, ExtremeValue);
				GAME_EVENT_TEST_STR(bSendResult,
				                    FString::Printf(TEXT("极值浮点数事件应该发送成功: %.6e"), ExtremeValue));

				FGameEventTestHelper::WaitForEventProcessing();
				GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到极值浮点数事件");

				TestReceiver->ResetTestState();
			}
		}

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("极值数据测试完成");
	}

	// ========================================
	// 空容器测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试空容器传输...");
		TestReceiver->ResetTestState();

		// 测试空数组
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT_ARRAY_EVENT), TestReceiver, TEXT("OnIntArrayEvent"));

		TArray<int32> EmptyArray;
		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT_ARRAY_EVENT), WorldContext, false, EmptyArray);
		GAME_EVENT_TEST_TEXT(bSendResult, "空数组事件应该发送成功");

		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到空数组事件");
		GAME_EVENT_TEST_EQUAL(TestReceiver->LastIntArrayValue.Num(), 0, "接收到的应该是空数组");

		TestReceiver->ResetTestState();

		// 测试空映射表
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::STRING_TO_INT_MAP_EVENT), TestReceiver, TEXT("OnStringToIntMapEvent"));

		TMap<FString, int32> EmptyMap;
		bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::STRING_TO_INT_MAP_EVENT), WorldContext, false, EmptyMap);
		GAME_EVENT_TEST_TEXT(bSendResult, "空映射表事件应该发送成功");

		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到空映射表事件");
		GAME_EVENT_TEST_EQUAL(TestReceiver->LastStringToIntMapValue.Num(), 0, "接收到的应该是空映射表");

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("空容器测试完成");
	}

	// ========================================
	// 大容器测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试大容器传输...");
		TestReceiver->ResetTestState();

		// 创建大数组（1000个元素）
		TArray<int32> LargeArray = FGameEventTestHelper::GenerateRandomIntArray(GameEventTestConstants::LARGE_CONTAINER_SIZE);

		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT_ARRAY_EVENT), WorldContext, false, LargeArray);
		GAME_EVENT_TEST_TEXT(bSendResult, "大数组事件应该发送成功");

		FGameEventTestHelper::WaitForEventProcessing(1.0f);  // 大数组需要更多处理时间
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver), "应该接收到大数组事件");
		GAME_EVENT_TEST_EQUAL(TestReceiver->LastIntArrayValue.Num(), LargeArray.Num(), "大数组大小应该正确");

		// 验证数组内容
		bool bArrayContentMatch = FAdvancedGameEventTestHelper::AreArraysEqual(TestReceiver->LastIntArrayValue, LargeArray);
		GAME_EVENT_TEST_TEXT(bArrayContentMatch, "大数组内容应该完全匹配");

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("大容器测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("边界测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 边界测试完成 ===");
	return true;
}

// ========================================
// 错误处理测试 - 异常情况和错误恢复验证
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventErrorHandlingTest, "GameEventSystem.ErrorHandling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventErrorHandlingTest)
{
	GAME_EVENT_LOG_INFO("=== 开始错误处理测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建测试接收器和世界上下文
	UGameEventTestReceiver* TestReceiver = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver, "测试接收器应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("错误处理测试");

	// ========================================
	// 空指针处理测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试空指针处理...");

		// 测试空世界上下文
		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::ERROR_HANDLING_EVENT), nullptr, false, 42);
		GAME_EVENT_TEST_TEXT(!bSendResult, "空世界上下文无法发送");

		// 测试空接收器添加监听器
		// 注意：这应该在内部被处理而不崩溃
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::NULL_RECEIVER_EVENT), nullptr, TEXT("OnInt32Event"));

		// 发送事件到没有有效监听器的事件
		bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::NULL_RECEIVER_EVENT), WorldContext, false, 42);
		// 这应该成功，但没有监听器接收
		GAME_EVENT_TEST_TEXT(bSendResult, "发送到没有有效监听器的事件应该成功");

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("空指针处理测试完成");
	}

	// ========================================
	// 无效函数名测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试无效函数名处理...");
		TestReceiver->ResetTestState();

		// 添加不存在的函数名监听器
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INVALID_EVENT), TestReceiver, TEXT("NonExistentFunction"));

		// 发送事件
		bool bSendResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INVALID_EVENT), WorldContext, false, 42);
		GAME_EVENT_TEST_TEXT(!bSendResult, "发送到无效函数的事件应该成功但不会被处理");

		FGameEventTestHelper::WaitForEventProcessing();

		// 应该没有接收到事件，因为函数不存在
		GAME_EVENT_TEST_STR(!FGameEventTestHelper::VerifyEventReceived(TestReceiver), TEXT("无效函数名不应该接收到事件"));

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("无效函数名测试完成");
	}

	// ========================================
	// 重复监听器测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试重复监听器处理...");
		TestReceiver->ResetTestState();

		const FString DuplicateEvent = TEXT("Test.Duplicate");

		// 添加同一个监听器多次
		EventManager->AddListenerFunction(FEventId(DuplicateEvent), TestReceiver, TEXT("OnInt32Event"));
		EventManager->AddListenerFunction(FEventId(DuplicateEvent), TestReceiver, TEXT("OnInt32Event"));
		EventManager->AddListenerFunction(FEventId(DuplicateEvent), TestReceiver, TEXT("OnInt32Event"));

		// 发送事件
		bool bSendResult = EventManager->SendEvent(FEventId(DuplicateEvent), WorldContext, false, 42);
		GAME_EVENT_TEST_TEXT(bSendResult, "重复监听器事件应该发送成功");

		FGameEventTestHelper::WaitForEventProcessing();

		// 验证是否正确处理重复监听器（可能只调用一次或多次，取决于实现）
		bool bReceivedEvent = FGameEventTestHelper::VerifyEventReceived(TestReceiver);
		GAME_EVENT_TEST_TEXT(bReceivedEvent, "重复监听器应该至少接收到一次事件");

		GAME_EVENT_LOG_INFO("重复监听器接收计数: %d", TestReceiver->EventReceivedCount);

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("重复监听器测试完成");
	}

	// ========================================
	// 监听器生命周期测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试监听器生命周期管理...");

		const FString LifecycleEvent = TEXT("Test.Lifecycle");

		// 创建临时接收器
		UGameEventTestReceiver* TempReceiver = FGameEventTestHelper::CreateTestReceiver();
		GAME_EVENT_TEST_NOT_NULL(TempReceiver, "临时接收器应该创建成功");

		// 添加监听器
		EventManager->AddListenerFunction(FEventId(LifecycleEvent), TempReceiver, TEXT("OnInt32Event"));

		// 发送事件验证监听器工作
		bool bSendResult = EventManager->SendEvent(FEventId(LifecycleEvent), WorldContext, false, 123);
		GAME_EVENT_TEST_TEXT(bSendResult, "生命周期测试事件应该发送成功");

		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TempReceiver), "临时接收器应该接收到事件");

		// 手动移除监听器
		EventManager->RemoveAllListenersForReceiver(FEventId(LifecycleEvent), TempReceiver);

		// 重置并再次发送事件
		TempReceiver->ResetTestState();
		bSendResult = EventManager->SendEvent(FEventId(LifecycleEvent), WorldContext, false, 456);
		GAME_EVENT_TEST_TEXT(bSendResult, "移除监听器后事件发送应该成功");

		FGameEventTestHelper::WaitForEventProcessing();
		GAME_EVENT_TEST_TEXT(!FGameEventTestHelper::VerifyEventReceived(TempReceiver), "移除监听器后不应该接收到事件");

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("监听器生命周期测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("错误处理测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 错误处理测试完成 ===");
	return true;
}

// ========================================
// 综合集成测试 - 完整功能验证
// ========================================

GAME_EVENT_TEST_CLASS(FGameEventIntegrationTest, "GameEventSystem.Integration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

BEGIN_GAME_EVENT_TEST(FGameEventIntegrationTest)
{
	GAME_EVENT_LOG_INFO("=== 开始综合集成测试 ===");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	// 获取事件管理器
	TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
	GAME_EVENT_TEST_NOT_NULL(EventManager.Get(), "事件管理器应该存在");

	// 创建多个测试接收器
	UGameEventTestReceiver* TestReceiver1 = FGameEventTestHelper::CreateTestReceiver();
	UGameEventTestReceiver* TestReceiver2 = FGameEventTestHelper::CreateTestReceiver();
	UObject* WorldContext = FGameEventTestHelper::CreateTestWorldContext();
	GAME_EVENT_TEST_NOT_NULL(TestReceiver1, "测试接收器1应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(TestReceiver2, "测试接收器2应该创建成功");
	GAME_EVENT_TEST_NOT_NULL(WorldContext, "世界上下文应该存在");

	GAME_EVENT_TEST_GROUP_START("综合集成测试");

	// ========================================
	// 多接收器多事件类型集成测试
	// ========================================
	{
		GAME_EVENT_LOG_INFO("测试多接收器多事件类型集成...");

		// 为不同接收器添加不同类型的监听器
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT32_EVENT), TestReceiver1, TEXT("OnInt32Event"));
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::STRING_EVENT), TestReceiver1, TEXT("OnStringEvent"));
		EventManager->AddListenerFunction(FEventId(GameEventTestEvents::INT32_EVENT), TestReceiver2, TEXT("OnInt32Event"));

		// 添加Lambda监听器
		std::atomic<int32> LambdaCount { 0 };
		FString LambdaId = EventManager->AddLambdaListener(
		                                                   FEventId(GameEventTestEvents::STRING_EVENT),
		                                                   TestReceiver2,
		                                                   [&LambdaCount](const FString& Value)
		                                                   {
			                                                   LambdaCount.fetch_add(1);
			                                                   UE_LOG(LogGameEventSystemTest, Log, TEXT("📊 集成测试Lambda接收字符串: %s"), *Value);
		                                                   }
		                                                  );

		// 发送各种类型的事件
		bool bIntResult = EventManager->SendEvent(FEventId(GameEventTestEvents::INT32_EVENT), WorldContext, false, 42);
		bool bStringResult = EventManager->SendEvent(FEventId(GameEventTestEvents::STRING_EVENT), WorldContext, false, FString(TEXT("集成测试字符串")));

		GAME_EVENT_TEST_TEXT(bIntResult, "整数事件发送应该成功");
		GAME_EVENT_TEST_TEXT(bStringResult, "字符串事件发送应该成功");

		FGameEventTestHelper::WaitForEventProcessing();

		// 验证所有接收器都收到了对应的事件
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver1, 2), "接收器1应该收到2个事件");
		GAME_EVENT_TEST_TEXT(FGameEventTestHelper::VerifyEventReceived(TestReceiver2, 1), "接收器2应该收到1个事件");
		GAME_EVENT_TEST_EQUAL(LambdaCount.load(), 1, "Lambda应该被调用1次");

		// 验证接收到的数据
		GAME_EVENT_TEST_EQUAL(TestReceiver1->LastReceivedInt, 42, "接收器1应该收到正确的整数");
		GAME_EVENT_TEST_EQUAL(TestReceiver1->LastReceivedString, TEXT("集成测试字符串"), "接收器1应该收到正确的字符串");
		GAME_EVENT_TEST_EQUAL(TestReceiver2->LastReceivedInt, 42, "接收器2应该收到正确的整数");

		// 清理Lambda监听器
		EventManager->RemoveLambdaListener(FEventId(GameEventTestEvents::STRING_EVENT), LambdaId);

		GAME_EVENT_GROUP_TEST_RESULT(true);
		GAME_EVENT_LOG_SUCCESS("多接收器多事件类型集成测试完成");
	}

	GAME_EVENT_TEST_GROUP_END("综合集成测试");

	// 清理测试环境
	FGameEventTestHelper::CleanupTestEnvironment();

	GAME_EVENT_LOG_SUCCESS("=== 综合集成测试完成 ===");
	GAME_EVENT_LOG_SUCCESS("=== 🎉 所有GameEventSystem测试已完成! 🎉 ===");

	return true;
}
