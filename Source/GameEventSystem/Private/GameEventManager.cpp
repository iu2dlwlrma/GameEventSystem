#include "GameEventManager.h"
#include "GameEventTypes.h"
#include "Logger.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include <atomic>

TSharedPtr<FGameEventManager> FGameEventManager::PrivateDefaultManager = nullptr;

FGameEventManager::FGameEventManager() : LambdaListenerIdCounter(1) // Start from 1 to ensure ID is never 0
{
}

FGameEventManager::~FGameEventManager()
{
}

TSharedPtr<FGameEventManager> FGameEventManager::Get()
{
	if (!PrivateDefaultManager.IsValid())
	{
		PrivateDefaultManager = MakeShared<FGameEventManager>();
		GES_LOG_DISPLAY(TEXT("GameEventManager instance created successfully, hash: 0x%08X"), GetTypeHash(PrivateDefaultManager));
	}

	return PrivateDefaultManager;
}

void FGameEventManager::Clear()
{
	FScopeLock Lock(&CriticalSection);

	const int32 EventCount = EventMap.Num();
	const int32 ReceiverCount = ReceiverMap.Num();
	const int32 LambdaListenerCount = LambdaListenerMap.Num();

	EventMap.Empty();
	ReceiverMap.Empty();
	LambdaListenerMap.Empty();
	LambdaListenerIdCounter.store(1);

	GES_LOG_DISPLAY(TEXT("GameEventManager cleanup completed - cleared %d events, %d receivers, %d LambdaListeners"), EventCount, ReceiverCount, LambdaListenerCount);
}

#pragma region "static"

void FGameEventManager::GetFunctionParameters(const UFunction* Function, TArray<FProperty*>& OutParamProperties)
{
	if (!Function)
	{
		return;
	}

	// Iterate through all properties of the function.
	// The loop condition correctly filters for parameters (`CPF_Parm`) and stops when it encounters
	// a non-parameter property (like a local variable) or the return value.
	for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		// A function's input parameter is any parameter that is NOT an output parameter.
		// This correctly includes pass-by-value (e.g., int32) and const-ref (e.g., const FString&) parameters,
		// while excluding non-const ref (e.g., FString& OutName) and return parameters.
		if (!PropIt->HasAnyPropertyFlags(CPF_OutParm) || PropIt->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
			OutParamProperties.Add(*PropIt);
		}
	}
}

bool FGameEventManager::ValidateFunctionParameters(const UFunction* Function, const TArray<FPropertyContext>& PropertyContexts)
{
	if (!Function)
	{
		GES_LOG_ERROR(TEXT("Function is null"));
		return false;
	}

	TArray<FProperty*> FunctionParams;
	GetFunctionParameters(Function, FunctionParams);

	const FString& FunctionName = Function->GetName();
	const int32 ExpectedParamCount = FunctionParams.Num();
	const int32 ProvidedParamCount = PropertyContexts.Num();

	// Check for an exact match of parameter counts.
	if (ProvidedParamCount != ExpectedParamCount)
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Parameter count mismatch: expected %d, but got %d."),
		                *FunctionName,
		                ExpectedParamCount,
		                ProvidedParamCount);
		return false;
	}

	// Validate provided parameters type by type
	for (int32 i = 0; i < ProvidedParamCount; ++i)
	{
		const FProperty* ExpectedProperty = FunctionParams[i];
		const FProperty* ProvidedProperty = PropertyContexts[i].Property.Get();

		if (!IsParameterCompatible(ExpectedProperty, ProvidedProperty))
		{
			const FString ExpectedTypeName = ExpectedProperty->GetCPPType();
			const FString ProvidedTypeName = ProvidedProperty->GetCPPType();
			GES_LOG_WARNING(TEXT("Event[%s] - Parameter[%d] ('%s') type mismatch. Expected compatible with '%s', but got '%s'."),
			                *FunctionName,
			                i,
			                *ExpectedProperty->GetName(),
			                *ExpectedTypeName,
			                *ProvidedTypeName);
			return false;
		}
	}

	return true;
}

bool FGameEventManager::IsParameterCompatible(const FProperty* ExpectedParam, const FProperty* ProvidedParam)
{
	if (!ExpectedParam || !ProvidedParam)
	{
		return false;
	}

	// Array type
	if (const FArrayProperty* ExpectedArray = CastField<FArrayProperty>(ExpectedParam))
	{
		if (const FArrayProperty* ProvidedArray = CastField<FArrayProperty>(ProvidedParam))
		{
			// RECURSIVE CHECK: The compatibility of the array depends on the compatibility of its inner elements.
			return IsParameterCompatible(ExpectedArray->Inner, ProvidedArray->Inner);
		}
	}
	// Set type
	else if (const FSetProperty* ExpectedSet = CastField<FSetProperty>(ExpectedParam))
	{
		if (const FSetProperty* ProvidedSet = CastField<FSetProperty>(ProvidedParam))
		{
			// RECURSIVE CHECK
			return IsParameterCompatible(ExpectedSet->ElementProp, ProvidedSet->ElementProp);
		}
	}
	// Map type
	else if (const FMapProperty* ExpectedMap = CastField<FMapProperty>(ExpectedParam))
	{
		if (const FMapProperty* ProvidedMap = CastField<FMapProperty>(ProvidedParam))
		{
			// RECURSIVE CHECK for both key and value.
			return IsParameterCompatible(ExpectedMap->KeyProp, ProvidedMap->KeyProp) &&
			       IsParameterCompatible(ExpectedMap->ValueProp, ProvidedMap->ValueProp);
		}
	}
	// Byte Enum
	else if (const FByteProperty* ExpectedByte = CastField<FByteProperty>(ExpectedParam))
	{
		if (const FEnumProperty* ProvidedByte = CastField<FEnumProperty>(ProvidedParam))
		{
			if (ExpectedByte->Enum == ProvidedByte->GetEnum())
			{
				return true;
			}
		}
	}
	else if (const FEnumProperty* ExpectedEnum = CastField<FEnumProperty>(ExpectedParam))
	{
		if (const FByteProperty* ProvidedByte = CastField<FByteProperty>(ProvidedParam))
		{
			// Todo ProvidedByte->Enum Always nullptr
			// if (ExpectedEnum->GetEnum() == ProvidedByte->Enum)
			// {
			// 	return true;
			// }
			return true;
		}
	}
	// check for an exact class match (e.g., FIntProperty == FIntProperty).
	// This is fast and covers most primitive types.
	if (ExpectedParam->GetClass() == ProvidedParam->GetClass())
	{
		// For complex types that matched their class, we must check their inner types.
		// Object/Class properties: Check for class compatibility (inheritance).
		if (const FObjectProperty* ExpectedObj = CastField<FObjectProperty>(ExpectedParam))
		{
			const FObjectProperty* ProvidedObj = CastField<FObjectProperty>(ProvidedParam);
			// This correctly handles cases where an AActor is expected and an APawn is provided.
			return ProvidedObj && ProvidedObj->PropertyClass->IsChildOf(ExpectedObj->PropertyClass);
		}

		// Struct properties: Must be the exact same struct type.
		if (const FStructProperty* ExpectedStruct = CastField<FStructProperty>(ExpectedParam))
		{
			const FStructProperty* ProvidedStruct = CastField<FStructProperty>(ProvidedParam);
			return ProvidedStruct && ExpectedStruct->Struct == ProvidedStruct->Struct;
		}

		// Enum properties: Must be the exact same enum.
		if (const FEnumProperty* ExpectedEnum = CastField<FEnumProperty>(ExpectedParam))
		{
			const FEnumProperty* ProvidedEnum = CastField<FEnumProperty>(ProvidedParam);
			return ProvidedEnum && ExpectedEnum->GetEnum() == ProvidedEnum->GetEnum();
		}

		// For all other types with matching classes (FIntProperty, FFloatProperty, etc.), they are compatible.
		return true;
	}

	// By default, if none of the above conditions are met, the types are not compatible.
	// We avoid the overly permissive numeric check from the original code.
	return false;
}

void FGameEventManager::RemoveListenerFromEvent(const FEventId& EventId, const FListenerContext& Listener)
{
	if (!EventMap.Contains(EventId))
	{
		return;
	}

	FEventContext& TargetEvent = EventMap[EventId];

	// Iterate backwards to safely remove elements
	for (int32 i = TargetEvent.Listeners.Num() - 1; i >= 0; --i)
	{
		const FListenerContext& CurrentListener = TargetEvent.Listeners[i];

		if (!CurrentListener.Receiver.IsValid())
		{
			TargetEvent.Listeners.RemoveAt(i);
			continue;
		}

		if (CurrentListener == Listener)
		{
			TargetEvent.Listeners.RemoveAt(i);

			// Update receiver mapping table
			if (UObject* ReceiverObj = Listener.Receiver.Get())
			{
				if (TArray<FListener>* Contexts = ReceiverMap.Find(ReceiverObj))
				{
					for (int32 j = Contexts->Num() - 1; j >= 0; --j)
					{
						const FListener& Context = (*Contexts)[j];
						if (Context.EventId == EventId && Context.Listener == Listener)
						{
							Contexts->RemoveAt(j);
							break;
						}
					}

					if (Contexts->Num() == 0)
					{
						ReceiverMap.Remove(ReceiverObj);
					}
				}
			}
			break;
		}
	}

	// Check if event still has listeners, delete event if not
	if (TargetEvent.Listeners.Num() == 0 && !TargetEvent.bPinned)
	{
		EventMap.Remove(EventId);
	}
}

#pragma endregion "static"

#pragma region  "Listener"

#pragma region "Add"
void FGameEventManager::AddListener(const FEventId& EventId, const FListenerContext& Listener)
{
	if (!EventId.IsValid())
	{
		GES_LOG_ERROR(TEXT("Event[%s] - Cannot add listener - EventId is invalid"), *EventId.GetName());
		return;
	}

	if (!Listener.Receiver.IsValid())
	{
		GES_LOG_ERROR(TEXT("Event[%s] - Cannot add listener - Receiver object is invalid"), *EventId.GetName());
		return;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventMap.Contains(EventId))
	{
		CreateEvent(EventId);
	}

	FListenerContext NewListener = Listener;
	if (!NewListener.IsBoundToDelegate() && !NewListener.IsBoundToLambda())
	{
		if (!NewListener.LinkFunction())
		{
			GES_LOG_ERROR(TEXT("Event[%s] - Listener registration failed - Cannot link function: %s"),
			              *EventId.GetName(),
			              *NewListener.ToString());
			return;
		}
	}

	FEventContext& TargetEvent = EventMap[EventId];

	if (TargetEvent.Listeners.Contains(NewListener))
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Listener already exists, skipping duplicate registration: %s"),
		                *EventId.GetName(),
		                *NewListener.ToString());
		return;
	}

	TargetEvent.Listeners.Add(NewListener);

	// Update receiver mapping table
	FListener ListenerContext;
	ListenerContext.EventId = EventId;
	ListenerContext.Listener = NewListener;

	TArray<FListener>* ReceiverListeners = ReceiverMap.Find(NewListener.Receiver.Get());
	if (!ReceiverListeners)
	{
		ReceiverMap.Add(NewListener.Receiver.Get(), TArray<FListener>());
		ReceiverListeners = ReceiverMap.Find(NewListener.Receiver.Get());
	}

	ReceiverListeners->Add(ListenerContext);

	// If it's a Lambda listener, add it to Lambda mapping table
	if (NewListener.IsBoundToLambda() && !NewListener.FunctionName.IsEmpty())
	{
		LambdaListenerMap.Add(NewListener.FunctionName, NewListener);
	}

	GES_LOG_DISPLAY(TEXT("Event[%s] - Listener registered successfully -> %s"), *EventId.GetName(), *NewListener.ToString());

	// Handle immediate trigger for pinned events
	if (TargetEvent.bPinned)
	{
		GES_LOG_DISPLAY(TEXT("Event[%s] - Pinned event detected, preparing to send to new listener"), *EventId.GetName());

		if (TargetEvent.HasValidParameters())
		{
			SendSpecificEventInternal(&NewListener, TargetEvent);
		}
		else
		{
			SendEventInternal(&NewListener);
			GES_LOG_DISPLAY(TEXT("Event[%s] -Triggered successfully -> %s"), *EventId.GetName(), *NewListener.ToString());
		}
	}
}

void FGameEventManager::AddListenerFunction(const FEventId& EventId, UObject* Receiver, const FString& FunctionName)
{
	if (!Receiver || FunctionName.IsEmpty())
	{
		GES_LOG_WARNING(TEXT("Event[%s] - AddListenerFunction Failed"), *EventId.GetName());
		return;
	}

	FListenerContext Listener;
	Listener.Receiver = Receiver;
	Listener.FunctionName = FunctionName;

	// Try to link function
	if (Listener.LinkFunction())
	{
		AddListener(EventId, Listener);
	}
	else
	{
		GES_LOG_WARNING(TEXT("Event[%s] Failed to find function '%s' in object '%s'"),
		                *EventId.GetName(),
		                *FunctionName,
		                *Receiver->GetClass()->GetName());
	}
}

#pragma endregion

void FGameEventManager::RemoveListener(const FEventId& EventId, const FListenerContext& Listener)
{
	FScopeLock Lock(&CriticalSection);

	if (!EventMap.Contains(EventId))
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Attempting to remove listener from non-existent event"), *EventId.GetName());
		return;
	}

	FEventContext& TargetEvent = EventMap[EventId];
	const int32 OriginalListenerCount = TargetEvent.Listeners.Num();
	int32 RemovedCount = 0;
	int32 InvalidRemovedCount = 0;

	// Iterate backwards to safely remove elements
	for (int32 i = OriginalListenerCount - 1; i >= 0; --i)
	{
		const FListenerContext& CurrentListener = TargetEvent.Listeners[i];

		if (!CurrentListener.Receiver.IsValid())
		{
			TargetEvent.Listeners.RemoveAt(i);
			InvalidRemovedCount++;
			GES_LOG_DISPLAY(TEXT("Event[%s] - Cleaning up invalid listener, index: %d"), *EventId.GetName(), i);
			continue;
		}

		if (CurrentListener == Listener)
		{
			TargetEvent.Listeners.RemoveAt(i);
			RemovedCount++;

			if (UObject* ReceiverObj = Listener.Receiver.Get())
			{
				if (TArray<FListener>* Contexts = ReceiverMap.Find(ReceiverObj))
				{
					for (int32 j = Contexts->Num() - 1; j >= 0; --j)
					{
						const FListener& Context = (*Contexts)[j];
						if (Context.EventId == EventId && Context.Listener == Listener)
						{
							Contexts->RemoveAt(j);
							break;
						}
					}

					if (Contexts->Num() == 0)
					{
						ReceiverMap.Remove(ReceiverObj);
						GES_LOG_DISPLAY(TEXT("Event[%s] All listeners for receiver [%s] have been removed, cleaning from mapping table"), *EventId.GetName(), *ReceiverObj->GetName());
					}
				}
			}

			// If it's a Lambda listener, also remove from Lambda mapping table
			if (Listener.IsBoundToLambda() && !Listener.FunctionName.IsEmpty())
			{
				LambdaListenerMap.Remove(Listener.FunctionName);
			}

			GES_LOG_DISPLAY(TEXT("Event[%s] Listener removed successfully -> %s"), *EventId.GetName(), *Listener.ToString());

			break;
		}
	}

	if (RemovedCount > 0)
	{
		GES_LOG_DISPLAY(TEXT("Event[%s] - Listener removal completed - Removed %d target listeners, cleaned %d invalid listeners, current count: %d"), *EventId.GetName(), RemovedCount, InvalidRemovedCount, TargetEvent.Listeners.Num());
	}
	else if (InvalidRemovedCount > 0)
	{
		GES_LOG_DISPLAY(TEXT("Event[%s] - Cleaned %d invalid listeners, but target listener not found"), *EventId.GetName(), InvalidRemovedCount);
	}
	else
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Listener to remove not found in [%s]"), *EventId.GetName(), *Listener.ToString());
	}

	// Check if event still has listeners, delete event if not
	if (TargetEvent.Listeners.Num() == 0 && !TargetEvent.bPinned)
	{
		EventMap.Remove(EventId);
		GES_LOG_DISPLAY(TEXT("Event[%s] - No listeners remaining, event deleted"), *EventId.GetName());
	}
}

void FGameEventManager::RemoveListener(const FEventId& EventId, UObject* Receiver, const FString& FunctionName)
{
	if (!Receiver || FunctionName.IsEmpty())
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Invalid receiver or function name"), *EventId.GetName());
		return;
	}

	FListenerContext Listener;
	Listener.Receiver = Receiver;
	Listener.FunctionName = FunctionName;

	// Try to link function to get complete listener information
	Listener.LinkFunction();

	RemoveListener(EventId, Listener);
}

void FGameEventManager::RemoveAllListenersForReceiver(const FEventId& EventId, const UObject* Receiver)
{
	if (!Receiver)
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Receiver cannot be null"), *EventId.GetName());
		return;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventMap.Contains(EventId))
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Event does not exist"), *EventId.GetName());
		return;
	}

	// Call internal generic method, only process specified events
	TSet<FEventId> EventsToProcess;
	EventsToProcess.Add(EventId);

	int32 RemovedCount = RemoveListenersForReceiverInternal(Receiver, &EventsToProcess);

	if (RemovedCount > 0)
	{
		GES_LOG_DISPLAY(TEXT("Event[%s] - Removed %d listeners for receiver[%s]"), *EventId.GetName(), RemovedCount, *Receiver->GetName());
	}
	else
	{
		GES_LOG_WARNING(TEXT("Event[%s] - No listeners found for receiver[%s]"), *EventId.GetName(), *Receiver->GetName());
	}
}

void FGameEventManager::RemoveLambdaListener(const FEventId& EventId, const FString& LambdaListenerId)
{
	if (LambdaListenerId.IsEmpty())
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Lambda listener ID cannot be empty"), *EventId.GetName());
		return;
	}

	FScopeLock Lock(&CriticalSection);

	FListenerContext* LambdaListener = LambdaListenerMap.Find(LambdaListenerId);
	if (!LambdaListener)
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Lambda listener [%s] not found"), *EventId.GetName(), *LambdaListenerId);
		return;
	}

	// Copy listener information and remove from mapping table
	FListenerContext ListenerToRemove = *LambdaListener;
	LambdaListenerMap.Remove(LambdaListenerId);

	// Remove listener from event
	RemoveListenerFromEvent(EventId, ListenerToRemove);

	GES_LOG_DISPLAY(TEXT("Event[%s] - Lambda listener [%s] removed successfully"), *EventId.GetName(), *LambdaListenerId);
}
#pragma endregion  "Listener"

#pragma region  "Send"

bool FGameEventManager::SendEvent(const FEventContext& EventContext)
{
	FScopeLock Lock(&CriticalSection);

	if (!EventMap.Contains(EventContext.EventId))
	{
		CreateEvent(EventContext.EventId, EventContext.bPinned);
	}

	FEventContext& TargetEvent = EventMap[EventContext.EventId];

	// Set multi-parameter context
	TargetEvent.SetPropertyContexts(EventContext.PropertyContexts);

	TargetEvent.bPinned = EventContext.bPinned;

	if (TargetEvent.Listeners.Num() == 0)
	{
		if (TargetEvent.bPinned && TargetEvent.HasValidParameters())
		{
			GES_LOG_DISPLAY(TEXT("Event[%s] - Pinned event saved with %d parameters, waiting for listener registration"), *EventContext.EventId.GetName(), TargetEvent.GetParameterCount());
		}
		else
		{
			GES_LOG_WARNING(TEXT("Event[%s] - Pinned No listeners registered"), *EventContext.EventId.GetName());
		}
		return true;
	}

	for (FListenerContext& Listener : TargetEvent.Listeners)
	{
		if (!Listener.IsValid())
		{
			continue;
		}
		if (TargetEvent.HasValidParameters())
		{
			if (SendSpecificEventInternal(&Listener, TargetEvent))
			{
				break;
			}
		}
		else
		{
			SendEventInternal(&Listener);
			GES_LOG_DISPLAY(TEXT("Event[%s] -Triggered successfully -> %s"), *EventContext.EventId.GetName(), *Listener.ToString());
		}
	}

	return true;
}

bool FGameEventManager::SendEventInternal(const FEventId& EventId, UObject* WorldContext, const bool bPinned, const TArray<FPropertyContext>& PropertyContexts)
{
	if (!WorldContext)
	{
		return false;
	}

	FEventContext EventContext;
	EventContext.WorldContext = WorldContext;
	EventContext.EventId = EventId;
	EventContext.bPinned = bPinned;
	EventContext.SetPropertyContexts(PropertyContexts);

	return SendEvent(EventContext);
}

void FGameEventManager::SendEventInternal(const FListenerContext* Listener)
{
	if (Listener->Function)
	{
		if (!ValidateFunctionParameters(Listener->Function, TArray<FPropertyContext>()))
		{
			GES_LOG_WARNING(TEXT("Function[%s] has invalid parameters"), *Listener->Function->GetName());
			return;
		}
		Listener->Receiver->ProcessEvent(Listener->Function, nullptr);
	}
	else if (Listener->LambdaFunction)
	{
		// Zero-parameter Lambda call
		FPropertyContext EmptyProperty;
		EmptyProperty.Property = nullptr;
		EmptyProperty.PropertyPtr = nullptr;

		Listener->LambdaFunction(EmptyProperty);
	}
	else if (Listener->PropertyDelegate.IsBound())
	{
		Listener->PropertyDelegate.Execute(TArray<FPropertyContext>());
	}
}

bool FGameEventManager::SendSpecificEventInternal(const FListenerContext* Listener, const FEventContext& EventContext)
{
	if (EventContext.PropertyContexts.Num() > 0 && EventContext.SpecificTarget && EventContext.SpecificTarget->IsValid())
	{
		SendPropertyEvent(EventContext.SpecificTarget, EventContext);
		return true;
	}
	SendPropertyEvent(Listener, EventContext);
	return false;
}

void FGameEventManager::SendPropertyEvent(const FListenerContext* Listener, const FEventContext& EventContext)
{
	const FString EventKey = EventContext.EventId.GetName();

	// Use multi-parameter context
	const TArray<FPropertyContext>& PropertyContexts = EventContext.PropertyContexts;

	if (Listener->Function)
	{
		SendFunctionEvent(Listener, PropertyContexts);

		GES_LOG_DISPLAY(TEXT("Event[%s] -Triggered successfully -> %s"), *EventKey, *Listener->ToString());
		return;
	}

	if (Listener->LambdaFunction)
	{
		FPropertyContext ParamProperty;

		if (PropertyContexts.Num() > 0)
		{
			// For multi-parameter Lambda, we pass pointer to entire PropertyContexts array
			// CreatePropertyWrapper will recognize this and extract all parameters
			ParamProperty.Property = nullptr;
			ParamProperty.PropertyPtr = const_cast<void*>(static_cast<const void*>(&PropertyContexts));
		}
		else
		{
			ParamProperty.Property = nullptr;
			ParamProperty.PropertyPtr = nullptr;
		}

		Listener->LambdaFunction(ParamProperty);

		GES_LOG_DISPLAY(TEXT("Event[%s] -Triggered successfully -> %s"), *EventKey, *Listener->ToString());
		return;
	}

	if (Listener->PropertyDelegate.IsBound())
	{
		Listener->PropertyDelegate.Execute(PropertyContexts);
		GES_LOG_DISPLAY(TEXT("Event[%s] -Triggered successfully -> %s"), *EventKey, *Listener->ToString());
	}
}

void FGameEventManager::SendFunctionEvent(const FListenerContext* Listener, const TArray<FPropertyContext>& PropertyContexts)
{
	// Check validity and compatibility of each parameter
	if (!ValidateFunctionParameters(Listener->Function, PropertyContexts))
	{
		return;
	}

	uint8* ParamsBuffer = static_cast<uint8*>(FMemory::Malloc(Listener->Function->ParmsSize));
	FMemory::Memzero(ParamsBuffer, Listener->Function->ParmsSize);

	TArray<FProperty*> Params;
	GetFunctionParameters(Listener->Function, Params);

	// Use multi-parameter version of processing function
	ProcessFunctionParameters(Params, PropertyContexts, ParamsBuffer, Listener->Function->GetName());

	Listener->Receiver->ProcessEvent(Listener->Function, ParamsBuffer);

	// Clean up parameter memory
	for (FProperty* Prop : Params)
	{
		Prop->DestroyValue_InContainer(ParamsBuffer);
	}
	FMemory::Free(ParamsBuffer);
}

void FGameEventManager::ProcessFunctionParameters(const TArray<FProperty*>& Params, const TArray<FPropertyContext>& PropertyContexts, uint8* ParamsBuffer, const FString& FunctionName)
{
	if (Params.Num() == 0)
	{
		return;
	}

	// Process multiple parameters
	for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
	{
		FProperty* DestProperty = Params[ParamIndex];

		// Check if there's a corresponding PropertyContext
		if (ParamIndex < PropertyContexts.Num() && PropertyContexts[ParamIndex].Property.Get())
		{
			const FPropertyContext& PropertyContext = PropertyContexts[ParamIndex];

			// Copy parameters when types match
			if (DestProperty->GetClass() == PropertyContext.Property->GetClass())
			{
				CopyPropertyByType(DestProperty, PropertyContext, ParamsBuffer);
			}
			else
			{
				// Handle cases where types don't match exactly but are compatible
				HandleCompatiblePropertyTypes(DestProperty, PropertyContext, ParamsBuffer);
			}
		}
	}
}

void FGameEventManager::CopyPropertyByType(const FProperty* DestProperty, const FPropertyContext& PropertyContext, uint8* ParamsBuffer)
{
	void* DestPtr = ParamsBuffer + DestProperty->GetOffset_ForUFunction();

	// Choose appropriate copy method based on property type
	if (const FMapProperty* MapProp = CastField<FMapProperty>(PropertyContext.Property.Get()))
	{
		CopyMapProperty(MapProp, PropertyContext.PropertyPtr, DestPtr);
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(PropertyContext.Property.Get()))
	{
		CopySetProperty(SetProp, PropertyContext.PropertyPtr, DestPtr);
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(PropertyContext.Property.Get()))
	{
		CopyArrayProperty(ArrayProp, PropertyContext.PropertyPtr, DestPtr);
	}
	else
	{
		// For other property types, use standard copy
		PropertyContext.Property->CopyCompleteValue(DestPtr, PropertyContext.PropertyPtr);
	}
}

void FGameEventManager::CopyMapProperty(const FMapProperty* MapProp, const void* SrcPtr, void* DestPtr)
{
	// Initialize target Map
	MapProp->InitializeValue(DestPtr);

	try
	{
		FScriptMapHelper SrcMapHelper(MapProp, SrcPtr);

		// Validate source Map validity
		if (SrcMapHelper.Num() >= 0)
		{
			FScriptMapHelper DestMapHelper(MapProp, DestPtr);

			// Copy Map elements one by one
			for (int32 SparseIndex = 0; SparseIndex < SrcMapHelper.GetMaxIndex(); ++SparseIndex)
			{
				if (SrcMapHelper.IsValidIndex(SparseIndex))
				{
					int32 NewIndex = DestMapHelper.AddUninitializedValue();

					// Copy Key
					uint8* SrcKey = SrcMapHelper.GetKeyPtr(SparseIndex);
					uint8* DestKey = DestMapHelper.GetKeyPtr(NewIndex);
					MapProp->KeyProp->InitializeValue(DestKey);
					MapProp->KeyProp->CopyCompleteValue(DestKey, SrcKey);

					// Copy Value
					uint8* SrcValue = SrcMapHelper.GetValuePtr(SparseIndex);
					uint8* DestValue = DestMapHelper.GetValuePtr(NewIndex);
					MapProp->ValueProp->InitializeValue(DestValue);
					MapProp->ValueProp->CopyCompleteValue(DestValue, SrcValue);
				}
			}

			DestMapHelper.Rehash();
		}
	}
	catch (...)
	{
		GES_LOG_ERROR(TEXT("Map property copy failed, using fallback method"));

		// Clean up and reinitialize
		MapProp->DestroyValue(DestPtr);
		MapProp->InitializeValue(DestPtr);
	}
}

void FGameEventManager::CopySetProperty(const FSetProperty* SetProp, const void* SrcPtr, void* DestPtr)
{
	// Initialize target Set
	SetProp->InitializeValue(DestPtr);

	FScriptSetHelper SrcSetHelper(SetProp, SrcPtr);
	FScriptSetHelper DestSetHelper(SetProp, DestPtr);

	int32 SrcNum = SrcSetHelper.Num();

	if (SrcNum > 0)
	{
		for (int32 SparseIndex = 0; SparseIndex < SrcSetHelper.GetMaxIndex(); ++SparseIndex)
		{
			if (SrcSetHelper.IsValidIndex(SparseIndex))
			{
				uint8* SrcElement = SrcSetHelper.GetElementPtr(SparseIndex);

				// Add elements to target Set
				int32 NewIndex = DestSetHelper.AddUninitializedValue();
				uint8* DestElement = DestSetHelper.GetElementPtr(NewIndex);

				// Initialize and copy elements
				SetProp->ElementProp->InitializeValue(DestElement);
				SetProp->ElementProp->CopyCompleteValue(DestElement, SrcElement);
			}
		}

		DestSetHelper.Rehash();
	}
}

void FGameEventManager::CopyArrayProperty(const FArrayProperty* ArrayProp, const void* SrcPtr, void* DestPtr)
{
	// Initialize target Array
	ArrayProp->InitializeValue(DestPtr);

	// Safely copy Array content
	ArrayProp->CopyCompleteValue(DestPtr, SrcPtr);
}

void FGameEventManager::HandleCompatiblePropertyTypes(FProperty* DestProperty, const FPropertyContext& PropertyContext, uint8* ParamsBuffer)
{
	if (const FStructProperty* StructProp = CastField<FStructProperty>(PropertyContext.Property.Get()))
	{
		CopyStructProperty(DestProperty, StructProp, PropertyContext.PropertyPtr, ParamsBuffer);
	}
	else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(PropertyContext.Property.Get()))
	{
		CopyObjectProperty(DestProperty, ObjProp, PropertyContext.PropertyPtr, ParamsBuffer);
	}
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(PropertyContext.Property.Get()))
	{
		CopyEnumProperty(DestProperty, ByteProp, PropertyContext.PropertyPtr, ParamsBuffer);
	}
}

void FGameEventManager::CopyStructProperty(FProperty* DestProperty, const FStructProperty* StructProp, const void* SrcPtr, uint8* ParamsBuffer)
{
	if (const FStructProperty* DestStructProp = CastField<FStructProperty>(DestProperty))
	{
		if (DestStructProp->Struct == StructProp->Struct)
		{
			StructProp->CopyCompleteValue(ParamsBuffer + DestStructProp->GetOffset_ForUFunction(), SrcPtr);
		}
	}
}

void FGameEventManager::CopyObjectProperty(FProperty* DestProperty, const FObjectProperty* ObjProp, const void* SrcPtr, uint8* ParamsBuffer)
{
	if (const FObjectProperty* DestObjProp = CastField<FObjectProperty>(DestProperty))
	{
		UObject* ObjectValue = ObjProp->GetObjectPropertyValue(SrcPtr);
		if (!ObjectValue || ObjectValue->IsA(DestObjProp->PropertyClass))
		{
			DestObjProp->SetObjectPropertyValue(ParamsBuffer + DestObjProp->GetOffset_ForUFunction(), ObjectValue);
		}
	}
}

void FGameEventManager::CopyEnumProperty(FProperty* DestProperty, const FByteProperty* ByteProp, const void* SrcPtr, uint8* ParamsBuffer)
{
	if (const FEnumProperty* DestEnumProp = CastField<FEnumProperty>(DestProperty))
	{
		void* DestPtr = ParamsBuffer + DestEnumProp->GetOffset_ForUFunction();
		const uint64 SourceValue = ByteProp->GetPropertyValue(SrcPtr);
		if (const FNumericProperty* UnderlyingProp = DestEnumProp->GetUnderlyingProperty())
		{
			UnderlyingProp->SetIntPropertyValue(DestPtr,SourceValue);
		}
	}
}
#pragma endregion  "Send"

#pragma region "Other Event"

bool FGameEventManager::HasEvent(const FEventId& EventId)
{
	FScopeLock Lock(&CriticalSection);

	return EventMap.Contains(EventId);
}

int32 FGameEventManager::GetEventListenerCount(const FEventId& EventId)
{
	FScopeLock Lock(&CriticalSection);

	if (const FEventContext* EventContext = EventMap.Find(EventId))
	{
		return EventContext->Listeners.Num();
	}
	return 0;
}

void FGameEventManager::UnpinEvent(const FEventId& EventId)
{
	FScopeLock Lock(&CriticalSection);

	if (EventMap.Contains(EventId))
	{
		FEventContext& TargetEvent = EventMap[EventId];

		if (TargetEvent.bPinned)
		{
			TargetEvent.bPinned = false;

			// Clean up all parameter contexts
			for (FPropertyContext& Context : TargetEvent.PropertyContexts)
			{
				Context.Clean();
			}
			TargetEvent.PropertyContexts.Empty();

			GES_LOG_DISPLAY(TEXT("Event[%s] - Unpinned"), *EventId.GetName());
		}
		else
		{
			GES_LOG_WARNING(TEXT("Event[%s] - Event is already not pinned"), *EventId.GetName());
		}
	}
	else
	{
		GES_LOG_WARNING(TEXT("Event[%s] - Event does not exist, cannot unpin"), *EventId.GetName());
	}
}

int32 FGameEventManager::RemoveListenersForReceiverInternal(const UObject* Receiver, const TSet<FEventId>* EventsToProcess)
{
	if (!Receiver)
	{
		return 0;
	}

	int32 TotalRemovedCount = 0;
	int32 InvalidRemovedCount = 0;
	TSet<FEventId> EventsToCheck; // Record events that need to be checked

	// If specific events to process are specified, only process those events
	if (EventsToProcess && EventsToProcess->Num() > 0)
	{
		for (const FEventId& EventId : *EventsToProcess)
		{
			if (EventMap.Contains(EventId))
			{
				FEventContext& TargetEvent = EventMap[EventId];

				// Iterate backwards to safely remove elements
				for (int32 i = TargetEvent.Listeners.Num() - 1; i >= 0; --i)
				{
					const FListenerContext& CurrentListener = TargetEvent.Listeners[i];

					if (!CurrentListener.Receiver.IsValid())
					{
						TargetEvent.Listeners.RemoveAt(i);
						InvalidRemovedCount++;
						continue;
					}

					// Remove all listeners matching the receiver
					if (CurrentListener.Receiver.Get() == Receiver)
					{
						// If it's a Lambda listener, remove from Lambda mapping table
						if (CurrentListener.IsBoundToLambda() && !CurrentListener.FunctionName.IsEmpty())
						{
							LambdaListenerMap.Remove(CurrentListener.FunctionName);
						}

						TargetEvent.Listeners.RemoveAt(i);
						TotalRemovedCount++;
						EventsToCheck.Add(EventId);
					}
				}
			}
		}
	}
	else
	{
		// Process all events - search through ReceiverMap
		if (!ReceiverMap.Contains(Receiver))
		{
			return 0;
		}

		const TArray<FListener> Contexts = ReceiverMap[Receiver];

		for (const FListener& Context : Contexts)
		{
			if (EventMap.Contains(Context.EventId))
			{
				FEventContext& TargetEvent = EventMap[Context.EventId];

				for (int32 i = TargetEvent.Listeners.Num() - 1; i >= 0; --i)
				{
					FListenerContext& CurrentListener = TargetEvent.Listeners[i];

					if (CurrentListener.Receiver.Get() == Receiver && CurrentListener.FunctionName == Context.Listener.FunctionName)
					{
						// If it's a Lambda listener, remove from Lambda mapping table
						if (CurrentListener.IsBoundToLambda() && !CurrentListener.FunctionName.IsEmpty())
						{
							LambdaListenerMap.Remove(CurrentListener.FunctionName);
						}

						TargetEvent.Listeners.RemoveAt(i);
						TotalRemovedCount++;
						EventsToCheck.Add(Context.EventId);
						break;
					}
				}
			}
		}
	}

	// Update ReceiverMap
	if (EventsToProcess && EventsToProcess->Num() > 0)
	{
		// Only remove mappings for specified events
		if (TArray<FListener>* Contexts = ReceiverMap.Find(Receiver))
		{
			for (int32 j = Contexts->Num() - 1; j >= 0; --j)
			{
				const FListener& Context = (*Contexts)[j];
				if (EventsToProcess->Contains(Context.EventId))
				{
					Contexts->RemoveAt(j);
				}
			}

			if (Contexts->Num() == 0)
			{
				ReceiverMap.Remove(Receiver);
				GES_LOG_DISPLAY(TEXT("All listeners for receiver [%s] removed, cleaning from mapping table"), *Receiver->GetName());
			}
		}
	}
	else
	{
		// Remove all mappings
		ReceiverMap.Remove(Receiver);
	}

	// Check and delete events with no listeners
	for (const FEventId& EventId : EventsToCheck)
	{
		if (EventMap.Contains(EventId))
		{
			FEventContext& TargetEvent = EventMap[EventId];
			if (TargetEvent.Listeners.Num() == 0 && !TargetEvent.bPinned)
			{
				EventMap.Remove(EventId);
				GES_LOG_DISPLAY(TEXT("Event[%s] - No listeners remaining after receiver cleanup, event deleted"), *EventId.GetName());
			}
		}
	}

	if (InvalidRemovedCount > 0)
	{
		GES_LOG_DISPLAY(TEXT("Cleaned %d invalid listeners during receiver cleanup"), InvalidRemovedCount);
	}

	return TotalRemovedCount;
}

void FGameEventManager::RemoveAllListenersForReceiver(const UObject* Receiver)
{
	if (!Receiver)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	int32 RemovedCount = RemoveListenersForReceiverInternal(Receiver, nullptr);

	if (RemovedCount > 0)
	{
		GES_LOG_DISPLAY(TEXT("RemoveAllListenersForReceiver - Removed %d listeners for receiver[%s]"), RemovedCount, *Receiver->GetName());
	}
}

void FGameEventManager::CreateEvent(const FEventId& EventId, const bool bPinned)
{
	FScopeLock Lock(&CriticalSection);

	if (EventMap.Contains(EventId))
	{
		return;
	}

	FEventContext NewEvent;
	NewEvent.EventId = EventId;
	NewEvent.bPinned = bPinned;

	EventMap.Add(EventId, NewEvent);

	GES_LOG_DISPLAY(TEXT("Event[%s] - CreateEvent : %s"), *EventId.GetName(), NewEvent.bPinned ? TEXT("Pinned") : TEXT("Unpinned"));
}

void FGameEventManager::DeleteEvent(const FEventId& EventId)
{
	FScopeLock Lock(&CriticalSection);

	if (EventMap.Contains(EventId))
	{
		EventMap.Remove(EventId);
		GES_LOG_DISPLAY(TEXT("Event[%s] - DeletedEvent"), *EventId.GetName());
	}
}

#pragma endregion
