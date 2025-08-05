#pragma once

#include "CoreMinimal.h"
#include "GameEventTypes.h"
#include "GameplayTagContainer.h"
#include "GameEventNodeUtils.generated.h"

UCLASS()
class GAMEEVENTNODE_API UGameEventNodeUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool IsDelegateMode(const UEdGraphPin* Pin);
	static bool IsStringEventId(const UEdGraphPin* Pin);
	static void ClearPinValue(UEdGraphPin* Pin);
	static FString GetEventName(const UEdGraphPin* Pin);
	static FString GetCurrentEventName(const UEdGraphPin* EventIdTypePin, const UEdGraphPin* EventTagPin, const UEdGraphPin* EventStringPin);

#pragma region "BlueprintInternalUseOnly"
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="AddListener (FunctionName)", meta = ( WorldContext = "WorldContextObject"))
	static void AddListener_ByFuncName(UObject* WorldContextObject, const FGameplayTag EventName, const FString& FunctionName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="AddListener_StrKey (FunctionName)", meta = ( WorldContext = "WorldContextObject"))
	static void AddListener_StrKey_ByFuncName(UObject* WorldContextObject, const FString EventName, const FString& FunctionName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="AddListener (Delegate)", meta = ( WorldContext = "WorldContextObject"))
	static void AddListener_ByDelegate(UObject* WorldContextObject, const FGameplayTag EventName, const FEventPropertyDelegate& PropertyDelegate);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="AddListener_StrKey (Delegate)", meta = ( WorldContext = "WorldContextObject"))
	static void AddListener_StrKey_ByDelegate(UObject* WorldContextObject, const FString EventName, const FEventPropertyDelegate& PropertyDelegate);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="RemoveListener (BPF)", meta = ( WorldContext = "WorldContextObject"))
	static void RemoveListener(UObject* WorldContextObject, const FGameplayTag EventName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="RemoveListener_StrKey (BPF)", meta = ( WorldContext = "WorldContextObject"))
	static void RemoveListener_StrKey(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEvent", meta = (CustomStructureParam = "ParamData", WorldContext = "WorldContextObject"))
	static void SendEvent(UObject* WorldContextObject, const FGameplayTag EventName, const bool bPinned, const int32& ParamData);
	DECLARE_FUNCTION(execSendEvent);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEvent_StrKey", meta = (CustomStructureParam = "ParamData", WorldContext = "WorldContextObject"))
	static void SendEvent_StrKey(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData);
	DECLARE_FUNCTION(execSendEvent_StrKey);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEvent (No Param)", meta = ( WorldContext = "WorldContextObject"))
	static void SendEvent_NoParam(UObject* WorldContextObject, const FGameplayTag EventName, const bool bPinned);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEvent_StrKey (No Param)", meta = ( WorldContext = "WorldContextObject"))
	static void SendEvent_NoParam_StrKey(UObject* WorldContextObject, const FString EventName, const bool bPinned);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="UnpinEvent", meta = ( WorldContext = "WorldContextObject"))
	static void UnpinEvent(UObject* WorldContextObject, const FGameplayTag EventName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="UnpinEvent_StrKey", meta = ( WorldContext = "WorldContextObject"))
	static void UnpinEvent_StrKey(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="HasEvent", meta = ( WorldContext = "WorldContextObject"))
	static bool HasEvent(UObject* WorldContextObject, const FGameplayTag EventName);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="HasEvent_StrKey", meta = ( WorldContext = "WorldContextObject"))
	static bool HasEvent_StrKey(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="GetEventListenerCount", meta = ( WorldContext = "WorldContextObject"))
	static int32 GetEventListenerCount(UObject* WorldContextObject, const FGameplayTag EventName);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="GetEventListenerCount_StrKey", meta = ( WorldContext = "WorldContextObject"))
	static int32 GetEventListenerCount_StrKey(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="RemoveAllListenersForReceiver", meta = ( WorldContext = "Receiver"))
	static void RemoveAllListenersForReceiver(UObject* Receiver);

#pragma region "Convert"
	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Boolean", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToBoolean(const FEventProperty& InProp, bool& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Byte", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToByte(const FEventProperty& InProp, uint8& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Integer", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToInt(const FEventProperty& InProp, int32& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Integer64", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToInt64(const FEventProperty& InProp, int64& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Float", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToFloat(const FEventProperty& InProp, float& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Double", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToDouble(const FEventProperty& InProp, double& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To String"), Category = "GameEventSystem|Conversions")
	static bool ConvertToString(const FEventProperty& InProp, FString& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Text"), Category = "GameEventSystem|Conversions")
	static bool ConvertToText(const FEventProperty& InProp, FText& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Name", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToName(const FEventProperty& InProp, FName& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, CustomThunk, meta = (DisplayName = "Convert To Struct", CustomStructureParam = "OutValue", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToStruct(const FEventProperty& InProp, TFieldPath<FProperty>& OutValue);
	DECLARE_FUNCTION(execConvertToStruct);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Object", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToObject(const FEventProperty& InProp, UObject*& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To SoftObject", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToSoftObject(const FEventProperty& InProp, TSoftObjectPtr<UObject>& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Class", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToClass(const FEventProperty& InProp, UClass*& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To SoftClass", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToSoftClass(const FEventProperty& InProp, TSoftClassPtr<UObject>& OutValue);

	UFUNCTION(BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Array", BlueprintAutocast, CustomStructureParam = "OutValue"), Category = "GameEventSystem|Conversions")
	static bool ConvertArrayType(const FEventProperty& InProp, TArray<uint8>& OutValue);
	DECLARE_FUNCTION(execConvertArrayType);

	UFUNCTION(BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Set", BlueprintAutocast, CustomStructureParam = "OutValue"), Category = "GameEventSystem|Conversions")
	static bool ConvertSetType(const FEventProperty& InProp, TSet<uint8>& OutValue);
	DECLARE_FUNCTION(execConvertSetType);

	UFUNCTION(BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Map", BlueprintAutocast, CustomStructureParam = "OutValue"), Category = "GameEventSystem|Conversions")
	static bool ConvertMapType(const FEventProperty& InProp, TMap<uint8, uint8>& OutValue);
	DECLARE_FUNCTION(execConvertMapType);

#pragma endregion

#pragma endregion
};
