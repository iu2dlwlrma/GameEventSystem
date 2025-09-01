#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

// 前向声明
class FGameEventManager;
class UGameEventTestReceiver;

DECLARE_LOG_CATEGORY_EXTERN(LogGameEventSystemTest, Log, All);

// ========================================
// 测试宏定义
// ========================================

/**
 * 游戏事件测试类宏
 * 简化测试类的定义
 */
// 使用标准的UE自动化测试宏
#define GAME_EVENT_TEST_CLASS(TestClass, PrettyName, TestFlags) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass, PrettyName, TestFlags)

#define BEGIN_GAME_EVENT_TEST(TestClassName) \
	bool TestClassName::RunTest(const FString& Parameters)

/**
 * 测试断言宏 - 带中文日志输出
 */
#define GAME_EVENT_TEST_TEXT(Condition, Message) \
	do { \
		FString _MsgStr = TEXT(Message); \
		if (!(Condition)) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 测试失败: ") TEXT(Message)); \
			TestTrue(_MsgStr, Condition); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Log, TEXT("✅ 测试通过:") TEXT(Message)); \
		} \
	} while(0)

#define GAME_EVENT_TEST_STR(Condition, Message) \
	do { \
		FString _MsgStr = (Message); \
		if (!(Condition)) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 测试失败: %s"), *_MsgStr); \
			TestFalse(_MsgStr, Condition); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Log, TEXT("✅ 测试通过: %s"), *_MsgStr); \
		} \
	} while(0)

#define GAME_EVENT_TEST_EQUAL(Actual, Expected, Message, ...) \
	do { \
		if ((Actual) != (Expected)) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 测试失败: %s (实际值与期望值不相等)"),TEXT(Message)); \
			TestEqual(TEXT(Message), Actual, Expected); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Log, TEXT("✅ 测试通过: ") TEXT(Message), ##__VA_ARGS__); \
		} \
	} while(0)

#define GAME_EVENT_TEST_NOT_EQUAL(Actual, Expected, Message) \
	do { \
		FString _MsgStr = (Message); \
		if ((Actual) == (Expected)) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 测试失败: (实际值与期望值相等)"), TEXT(Message)); \
			TestNotEqual(TEXT(Message), Actual, Expected); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Log, TEXT("✅ 测试通过: "), TEXT(Message)); \
		} \
	} while(0)

#define GAME_EVENT_TEST_NULL(Pointer, Message) \
	do { \
		if ((Pointer) != nullptr) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 测试失败:  (指针不为空)"), TEXT(Message)); \
			TestNull(TEXT(Message), Pointer); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Log, TEXT("✅ 测试通过:") TEXT(Message)); \
		} \
	} while(0)

#define GAME_EVENT_TEST_NOT_NULL(Pointer, Format, ...) \
	do { \
		if ((Pointer) == nullptr) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 测试失败:  (指针为空)"), TEXT(Format)); \
			TestNotNull(TEXT(Format), Pointer); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Display, TEXT("✅ 测试通过: ") TEXT(Format), ##__VA_ARGS__); \
		} \
	} while(0)

/**
 * 测试性能宏 - 验证执行时间是否在预期范围内
 */
#define GAME_EVENT_TEST_PERFORMANCE(TestFunction, MaxTimeMs, Message) \
	do { \
		double ExecutionTime = FGameEventTestHelper::MeasureEventPerformance([&]() { TestFunction; }, 1); \
		if (ExecutionTime > MaxTimeMs) { \
			UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ 性能测试失败: %s (执行时间: %.3fms > 最大时间: %.3fms)"), TEXT(Message), ExecutionTime, MaxTimeMs); \
			return false; \
		} else { \
			UE_LOG(LogGameEventSystemTest, Log, TEXT("✅ 性能测试通过: %s (执行时间: %.3fms)"), TEXT(Message), ExecutionTime); \
		} \
	} while(0)

/**
 * 测试日志输出宏
 */
#define GAME_EVENT_LOG_INFO(Format, ...) \
	UE_LOG(LogGameEventSystemTest, Log, TEXT("ℹ️ ") TEXT(Format), ##__VA_ARGS__)

#define GAME_EVENT_LOG_WARNING(Format, ...) \
	UE_LOG(LogGameEventSystemTest, Warning, TEXT("⚠️ ") TEXT(Format), ##__VA_ARGS__)

#define GAME_EVENT_LOG_ERROR(Format, ...) \
	UE_LOG(LogGameEventSystemTest, Error, TEXT("❌ ") TEXT(Format), ##__VA_ARGS__)

#define GAME_EVENT_LOG_SUCCESS(Format, ...) \
	UE_LOG(LogGameEventSystemTest, Display, TEXT("✅ ") TEXT(Format), ##__VA_ARGS__)

/**
 * 测试分组宏
 */
#define GAME_EVENT_TEST_GROUP_START(GroupName) \
	UE_LOG(LogGameEventSystemTest, Display, TEXT("")); \
	UE_LOG(LogGameEventSystemTest, Display, TEXT("🚀 === 开始测试组: %s ==="), TEXT(GroupName)); \
	int32 GroupTestCount = 0; \
	int32 GroupPassedCount = 0;

#define GAME_EVENT_TEST_GROUP_END(GroupName) \
	UE_LOG(LogGameEventSystemTest, Display, TEXT("🏁 === %s 测试组完成 ==="), TEXT(GroupName)); \
	UE_LOG(LogGameEventSystemTest, Display, TEXT("📊 测试统计: 总计 %d 项，通过 %d 项，失败 %d 项，成功率 %.1f%%"), \
		GroupTestCount, GroupPassedCount, GroupTestCount - GroupPassedCount, \
		GroupTestCount > 0 ? (float(GroupPassedCount) / float(GroupTestCount) * 100.0f) : 0.0f); \
	UE_LOG(LogGameEventSystemTest, Display, TEXT(""));

#define GAME_EVENT_GROUP_TEST_RESULT(bSuccess) \
	do { \
		GroupTestCount++; \
		if (bSuccess) { \
			GroupPassedCount++; \
		} \
	} while(0)

// ========================================
// 测试辅助类和常量定义
// ========================================

/**
 * 测试常量定义
 */
namespace GameEventTestConstants
{
	// 性能测试常量
	static const int32 DEFAULT_PERFORMANCE_ITERATIONS = 1000;
	static const double MAX_SINGLE_EVENT_TIME_MS = 1.0; // 单个事件最大执行时间1毫秒
	static const double MAX_BATCH_EVENT_TIME_MS = 100.0; // 批量事件最大执行时间100毫秒

	// 多线程测试常量
	static const int32 DEFAULT_THREAD_COUNT = 4;
	static const int32 DEFAULT_ITERATIONS_PER_THREAD = 25;
	static const int32 STRESS_TEST_ITERATIONS = 10000;
	static const float STRESS_TEST_MAX_TIME_SECONDS = 30.0f;

	// 等待时间常量
	static const float EVENT_PROCESSING_WAIT_TIME = 0.05f; // 50毫秒
	static const float MULTI_THREAD_WAIT_TIME = 1.0f; // 1秒

	// 测试数据常量
	static const int32 TEST_ARRAY_SIZE = 10;
	static const int32 TEST_MAP_SIZE = 5;
	static const int32 LARGE_CONTAINER_SIZE = 1000;

	// 边界值测试常量
	static const int8 TEST_INT8_MIN = -128;
	static const int8 TEST_INT8_MAX = 127;
	static const uint8 TEST_UINT8_MAX = 255;
	static const int16 TEST_INT16_MIN = -32768;
	static const int16 TEST_INT16_MAX = 32767;
	static const uint16 TEST_UINT16_MAX = 65535;
	static const int32 TEST_INT32_MIN = -2147483648;
	static const int32 TEST_INT32_MAX = 2147483647;
	static const uint32 TEST_UINT32_MAX = 4294967295U;
	static const int64 TEST_INT64_MIN = -9223372036854775807LL - 1;
	static const int64 TEST_INT64_MAX = 9223372036854775807LL;
	static const uint64 TEST_UINT64_MAX = 18446744073709551615ULL;
	static const float TEST_FLOAT_MIN = -3.4028235e+38f;
	static const float TEST_FLOAT_MAX = 3.4028235e+38f;
	static const double TEST_DOUBLE_MIN = -1.7976931348623158e+308;
	static const double TEST_DOUBLE_MAX = 1.7976931348623158e+308;
}

/**
 * 测试事件ID常量
 */
namespace GameEventTestEvents
{
	// 基础类型事件ID
	static const FString BOOL_EVENT = TEXT("Test.Bool");
	static const FString INT8_EVENT = TEXT("Test.Int8");
	static const FString UINT8_EVENT = TEXT("Test.UInt8");
	static const FString INT16_EVENT = TEXT("Test.Int16");
	static const FString UINT16_EVENT = TEXT("Test.UInt16");
	static const FString INT32_EVENT = TEXT("Test.Int32");
	static const FString UINT32_EVENT = TEXT("Test.UInt32");
	static const FString INT64_EVENT = TEXT("Test.Int64");
	static const FString UINT64_EVENT = TEXT("Test.UInt64");
	static const FString FLOAT_EVENT = TEXT("Test.Float");
	static const FString DOUBLE_EVENT = TEXT("Test.Double");

	// 字符串类型事件ID
	static const FString STRING_EVENT = TEXT("Test.String");
	static const FString NAME_EVENT = TEXT("Test.Name");
	static const FString TEXT_EVENT = TEXT("Test.Text");

	// 数学类型事件ID
	static const FString VECTOR_EVENT = TEXT("Test.Vector");
	static const FString VECTOR2D_EVENT = TEXT("Test.Vector2D");
	static const FString VECTOR4_EVENT = TEXT("Test.Vector4");
	static const FString ROTATOR_EVENT = TEXT("Test.Rotator");
	static const FString QUAT_EVENT = TEXT("Test.Quat");
	static const FString TRANSFORM_EVENT = TEXT("Test.Transform");
	static const FString COLOR_EVENT = TEXT("Test.Color");
	static const FString LINEAR_COLOR_EVENT = TEXT("Test.LinearColor");

	// 自定义类型事件ID
	static const FString ENUM_EVENT = TEXT("Test.Enum");
	static const FString ENUM64_EVENT = TEXT("Test.Enum64");
	static const FString PRIORITY_EVENT = TEXT("Test.Priority");
	static const FString STATE_EVENT = TEXT("Test.State");
	static const FString SIMPLE_STRUCT_EVENT = TEXT("Test.SimpleStruct");
	static const FString COMPLEX_STRUCT_EVENT = TEXT("Test.ComplexStruct");
	static const FString NESTED_STRUCT_EVENT = TEXT("Test.NestedStruct");

	// 容器类型事件ID
	static const FString INT_ARRAY_EVENT = TEXT("Test.IntArray");
	static const FString STRING_ARRAY_EVENT = TEXT("Test.StringArray");
	static const FString INT_SET_EVENT = TEXT("Test.IntSet");
	static const FString STRING_TO_INT_MAP_EVENT = TEXT("Test.StringToIntMap");
	static const FString INT_TO_FLOAT_MAP_EVENT = TEXT("Test.IntToFloatMap");

	// 嵌套容器类型事件ID
	static const FString NESTED_INT_ARRAY_EVENT = TEXT("Test.NestedIntArray");
	static const FString STRING_TO_FLOAT_ARRAY_MAP_EVENT = TEXT("Test.StringToFloatArrayMap");
	static const FString STRUCT_ARRAY_EVENT = TEXT("Test.StructArray");

	// 多参数事件ID
	static const FString MULTI_PARAM_EVENT = TEXT("Test.MultiParam");
	static const FString COMPLEX_MULTI_PARAM_EVENT = TEXT("Test.ComplexMultiParam");

	// 无参数事件ID
	static const FString SIMPLE_EVENT = TEXT("Test.Simple");

	// 边界情况事件ID
	static const FString BOUNDARY_ENUM_EVENT = TEXT("Test.BoundaryEnum");
	static const FString NON_BLUEPRINT_ENUM_EVENT = TEXT("Test.NonBlueprintEnum");

	// Lambda测试事件ID
	static const FString LAMBDA_INT_EVENT = TEXT("Test.Lambda.Int");
	static const FString LAMBDA_STRING_EVENT = TEXT("Test.Lambda.String");
	static const FString LAMBDA_MULTI_PARAM_EVENT = TEXT("Test.Lambda.MultiParam");

	// 多线程测试事件ID
	static const FString MULTITHREAD_EVENT = TEXT("Test.MultiThread");
	static const FString STRESS_TEST_EVENT = TEXT("Test.StressTest");

	// 错误处理测试事件ID
	static const FString ERROR_HANDLING_EVENT = TEXT("Test.ErrorHandling");
	static const FString INVALID_EVENT = TEXT("");
	static const FString NULL_RECEIVER_EVENT = TEXT("Test.NullReceiver");
}

/**
 * 测试统计信息结构
 */
struct FGameEventTestStatistics
{
	int32 TotalTests = 0;
	int32 PassedTests = 0;
	int32 FailedTests = 0;
	double TotalExecutionTime = 0.0;
	double AverageExecutionTime = 0.0;
	double MinExecutionTime = DBL_MAX;
	double MaxExecutionTime = 0.0;

	void Reset()
	{
		TotalTests = 0;
		PassedTests = 0;
		FailedTests = 0;
		TotalExecutionTime = 0.0;
		AverageExecutionTime = 0.0;
		MinExecutionTime = DBL_MAX;
		MaxExecutionTime = 0.0;
	}

	void AddTestResult(const bool bPassed, const double ExecutionTime)
	{
		TotalTests++;
		if (bPassed)
		{
			PassedTests++;
		}
		else
		{
			FailedTests++;
		}

		TotalExecutionTime += ExecutionTime;
		AverageExecutionTime = TotalExecutionTime / TotalTests;

		if (ExecutionTime < MinExecutionTime)
		{
			MinExecutionTime = ExecutionTime;
		}

		if (ExecutionTime > MaxExecutionTime)
		{
			MaxExecutionTime = ExecutionTime;
		}
	}

	float GetSuccessRate() const
	{
		return TotalTests > 0 ? static_cast<float>(PassedTests) / static_cast<float>(TotalTests) * 100.0f : 0.0f;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("总测试数:%d, 通过:%d, 失败:%d, 成功率:%.1f%%, 总时间:%.3fms, 平均时间:%.3fms, 最快:%.3fms, 最慢:%.3fms"),
		                       TotalTests,
		                       PassedTests,
		                       FailedTests,
		                       GetSuccessRate(),
		                       TotalExecutionTime,
		                       AverageExecutionTime,
		                       MinExecutionTime,
		                       MaxExecutionTime);
	}
};

/**
 * 高级测试辅助类
 */
class GAMEEVENTSYSTEMTEST_API FAdvancedGameEventTestHelper
{
public:
	// 创建测试统计对象
	static FGameEventTestStatistics CreateTestStatistics();

	// 验证容器相等性
	template<typename T>
	static bool AreArraysEqual(const TArray<T>& A, const TArray<T>& B);

	template<typename T>
	static bool AreSetsEqual(const TSet<T>& A, const TSet<T>& B);

	template<typename K, typename V>
	static bool AreMapsEqual(const TMap<K, V>& A, const TMap<K, V>& B);

	// 验证嵌套容器相等性
	static bool AreNestedArraysEqual(const TArray<TArray<int32>>& A, const TArray<TArray<int32>>& B);

	// 验证浮点数相等性（带容差）
	static bool AreFloatsEqual(float A, float B, float Tolerance = 0.0001f);
	static bool AreDoublesEqual(double A, double B, double Tolerance = 0.000001);

	// 验证向量相等性（带容差）
	static bool AreVectorsEqual(const FVector& A, const FVector& B, float Tolerance = 0.0001f);
	static bool AreVector2DsEqual(const FVector2D& A, const FVector2D& B, float Tolerance = 0.0001f);
	static bool AreVector4sEqual(const FVector4& A, const FVector4& B, float Tolerance = 0.0001f);

	// 验证旋转相等性（带容差）
	static bool AreRotatorsEqual(const FRotator& A, const FRotator& B, float Tolerance = 0.0001f);
	static bool AreQuatsEqual(const FQuat& A, const FQuat& B, float Tolerance = 0.0001f);

	// 验证变换相等性（带容差）
	static bool AreTransformsEqual(const FTransform& A, const FTransform& B, float Tolerance = 0.0001f);

	// 验证颜色相等性
	static bool AreColorsEqual(const FColor& A, const FColor& B);
	static bool AreLinearColorsEqual(const FLinearColor& A, const FLinearColor& B, float Tolerance = 0.0001f);

	// 生成测试数据
	static FVector GenerateRandomVector();
	static FRotator GenerateRandomRotator();
	static FColor GenerateRandomColor();
	static FLinearColor GenerateRandomLinearColor();
	static FTransform GenerateRandomTransform();

	// 边界值测试数据生成
	static TArray<int32> GetBoundaryIntegers();
	static TArray<float> GetBoundaryFloats();
	static TArray<double> GetBoundaryDoubles();

	// 压力测试辅助
	static bool RunConcurrentStressTest(TFunction<void()> TestFunction, int32 ConcurrentThreads, int32 IterationsPerThread);

	// 内存泄漏检测辅助
	static void CheckMemoryLeaks(const TFunction<void()>& TestFunction);
};

// ========================================
// 模板函数实现
// ========================================

template<typename T>
bool FAdvancedGameEventTestHelper::AreArraysEqual(const TArray<T>& A, const TArray<T>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 i = 0; i < A.Num(); ++i)
	{
		if (A[i] != B[i])
		{
			return false;
		}
	}

	return true;
}

template<typename T>
bool FAdvancedGameEventTestHelper::AreSetsEqual(const TSet<T>& A, const TSet<T>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (const T& Element : A)
	{
		if (!B.Contains(Element))
		{
			return false;
		}
	}

	return true;
}

template<typename K, typename V>
bool FAdvancedGameEventTestHelper::AreMapsEqual(const TMap<K, V>& A, const TMap<K, V>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (const auto& Pair : A)
	{
		const V* ValueB = B.Find(Pair.Key);
		if (!ValueB || *ValueB != Pair.Value)
		{
			return false;
		}
	}

	return true;
}
