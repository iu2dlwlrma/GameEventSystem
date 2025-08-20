#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "GameEventTypes.h"
#include "GameEventPropertyHelper.h"
#include "Logger.h"
#include <atomic>

class UGameEventBlueprintLibrary;
class UAsyncGameEventListener;
class UGameEventListenerComponent;
class FGameEventTypeManager;

class GAMEEVENTSYSTEM_API FGameEventManager
{
public:
	FGameEventManager();
	~FGameEventManager();

	static TSharedPtr<FGameEventManager> Get();

	void Clear();

#pragma region "static"
	static void GetFunctionParameters(const UFunction* Function, TArray<FProperty*>& OutParamProperties);

	static bool IsNumericProperty(const FProperty* Property);
	static bool ValidateFunctionParameters(const UFunction* Function, const TArray<FPropertyContext>& PropertyContexts);
	static bool IsParameterCompatible(FProperty* ExpectedParam, FProperty* ProvidedParam);
#pragma endregion "static"

#pragma region  "Listener"
	void AddListener(const FEventId& EventId, const FListenerContext& Listener);

	void AddListenerFunction(const FEventId& EventId, UObject* Receiver, const FString& FunctionName);

	void RemoveListener(const FEventId& EventId, const FListenerContext& Listener);

	/**
	 * Remove a specific function listener for a given recipient
	 * @param EventId Event identifier
	 * @param Receiver Receiving objects
	 * @param FunctionName Function name
	 */
	void RemoveListener(const FEventId& EventId, UObject* Receiver, const FString& FunctionName);

	/**
	 * Remove all listeners for a given receiver in a specific event
	 * @param EventId Event identifier
	 * @param Receiver Receiving objects
	 */
	void RemoveAllListenersForReceiver(const FEventId& EventId, const UObject* Receiver);

	/**
	 * Remove the Lambda listener
	 * @param EventId Event identifier
	 * @param LambdaListenerId AddLambdaListener returns the listener ID
	 */
	void RemoveLambdaListener(const FEventId& EventId, const FString& LambdaListenerId);

	/**
	 * 🚀 Universal Lambda listeners
	 * Automatically derive Lambda function signatures with any number of parameters (0-N)
	 * 
	 * @tparam Lambda Lambda expression types (auto-derived)
	 * @param EventId Event identifier
	 * @param Receiver Receiving Objects (for Lifecycle Management)
	 * @param InLambda Lambda function, arbitrary signature
	 * @return Listener ID for subsequent removal
	 */
	template<typename Lambda>
	FString AddLambdaListener(const FEventId& EventId, UObject* Receiver, Lambda&& InLambda);
#pragma endregion  "Listener"

#pragma region  "Send"
	bool SendEvent(const FEventContext& EventContext);

	/**
	 * 🚀 Generic variable parameter SendEvent function
	 * Automatically derive parameter types, support any number of parameters (0-N)
	 * 
	 * @tparam Args Variable parameter type package, which is automatically derived from the parameter
	 * @param EventId Event identifier
	 * @param WorldContext The following is the world
	 * @param bPinned Whether the event is fixed or not
	 * @param Params Variable parameter list
	 * @return Whether the delivery was successful
	 */
	template<typename... Args>
	bool SendEvent(const FEventId& EventId, UObject* WorldContext, const bool bPinned, Args&&... Params);

private:
	bool SendEventInternal(const FEventId& EventId, UObject* WorldContext, const bool bPinned, const TArray<FPropertyContext>& PropertyContexts);
	void SendEventInternal(const FListenerContext* Listener);
	bool SendSpecificEventInternal(const FListenerContext* Listener, const FEventContext& EventContext);
	void SendPropertyEvent(const FListenerContext* Listener, const FEventContext& EventContext);
	void SendFunctionEvent(const FListenerContext* Listener, const TArray<FPropertyContext>& PropertyContexts);

	void ProcessFunctionParameters(const TArray<FProperty*>& Params, const TArray<FPropertyContext>& PropertyContexts, uint8* ParamsBuffer, const FString& FunctionName);
	void CopyPropertyByType(const FProperty* DestProperty, const FPropertyContext& PropertyContext, uint8* ParamsBuffer);
	void HandleCompatiblePropertyTypes(FProperty* DestProperty, const FPropertyContext& PropertyContext, uint8* ParamsBuffer);

	// Copy functions for specific attribute types
	void CopyMapProperty(const FMapProperty* MapProp, const void* SrcPtr, void* DestPtr);
	void CopySetProperty(const FSetProperty* SetProp, const void* SrcPtr, void* DestPtr);
	void CopyArrayProperty(const FArrayProperty* ArrayProp, const void* SrcPtr, void* DestPtr);
	void CopyStructProperty(FProperty* DestProperty, const FStructProperty* StructProp, const void* SrcPtr, uint8* ParamsBuffer);
	void CopyObjectProperty(FProperty* DestProperty, const FObjectProperty* ObjProp, const void* SrcPtr, uint8* ParamsBuffer);
	int32 RemoveListenersForReceiverInternal(const UObject* Receiver, const TSet<FEventId>* EventsToProcess = nullptr);
	void RemoveListenerFromEvent(const FEventId& EventId, const FListenerContext& Listener);

#pragma endregion  "Send"

#pragma region Other Event

public:
	bool HasEvent(const FEventId& EventId);
	int32 GetEventListenerCount(const FEventId& EventId);
	void UnpinEvent(const FEventId& EventId);
	void RemoveAllListenersForReceiver(const UObject* Receiver);

private:
	void CreateEvent(const FEventId& EventId, const bool bPinned = false);
	void DeleteEvent(const FEventId& EventId);
#pragma endregion

	static TSharedPtr<FGameEventManager> PrivateDefaultManager;

	FCriticalSection CriticalSection;

	TMap<FEventId, FEventContext> EventMap;

	TMap<TObjectPtr<UObject>, TArray<FListener>> ReceiverMap;

	// Lambda Listener Mapping Table - Used to quickly remove Lambda listeners by ID
	TMap<FString, FListenerContext> LambdaListenerMap;

	// Atomic counter to generate a unique event listener ID
	std::atomic<uint64> LambdaListenerIdCounter;
};

template<typename Lambda>
FString FGameEventManager::AddLambdaListener(const FEventId& EventId, UObject* Receiver, Lambda&& InLambda)
{
	if (!Receiver)
	{
		GES_LOG_DISPLAY(TEXT("GameEventManager:AddLambdaListener, Receiver cannot be null for Lambda listener"));
		return FString();
	}

	// 🚀 Use Lambda type derivation to automatically expand parameter types
	auto WrapperLambda = FGameEventPropertyHelper::CreatePropertyWrapperFromLambda(std::forward<Lambda>(InLambda));

	// 🔧 Use atomic counters to generate truly unique Lambda IDs
	// Note: Each Lambda gets a unique ID, which avoids the problem of address collisions
	// If you need to detect Lambda with the same logic, we recommend using a normal function listener
	uint64 UniqueId = LambdaListenerIdCounter.fetch_add(1);
	FString FunctionName = FString::Printf(TEXT("%s.lambda.%llu"), *EventId.ToString(), UniqueId);

	// calculateTheNumberOfParametersAtCompileTime
	// constexpr size_t ParamCount = TLambdaArgsCount<std::decay_t<Lambda>>;

	FListenerContext Listener;
	Listener.Receiver = Receiver;
	Listener.LambdaFunction = WrapperLambda;
	Listener.FunctionName = FunctionName;

	AddListener(EventId, Listener);
	return FunctionName;
}

template<typename... Args>
bool FGameEventManager::SendEvent(const FEventId& EventId, UObject* WorldContext, const bool bPinned, Args&&... Params)
{
	if (!WorldContext || !EventId.IsValid())
	{
		return false;
	}

	// Parameter count at compile time
	constexpr size_t ParamCount = sizeof...(Args);

	// If there are no parameters, send an empty event directly
	if constexpr (ParamCount == 0)
	{
		FEventContext EventContext;
		EventContext.WorldContext = WorldContext;
		EventContext.EventId = EventId;
		EventContext.bPinned = bPinned;

		return SendEvent(EventContext);
	}
	else
	{
		TArray<FPropertyContext> PropertyContexts;

		([&](auto&& arg)
		{
			using ArgType = std::decay_t<decltype(arg)>;

			// Prevent string literals from crashing
			static_assert(!std::is_array_v<ArgType> ||
			              !(std::is_same_v<std::remove_cv_t<std::remove_extent_t<ArgType>>, char> ||
			                std::is_same_v<std::remove_cv_t<std::remove_extent_t<ArgType>>, wchar_t>),
			              "❌ You can't use string literals directly TEXT(\"string\") ✅ Please use FString(TEXT(\"string\")) replace!");

			if (FProperty* Property = FGameEventPropertyHelper::GetPropertyForType<ArgType>(WorldContext, arg))
			{
				PropertyContexts.Add(FPropertyContext(Property, const_cast<void*>(static_cast<const void*>(&arg))));
			}
		}(std::forward<Args>(Params)), ...);

		return SendEventInternal(EventId, WorldContext, bPinned, PropertyContexts);
	}
}
