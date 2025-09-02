#pragma once

#include "CoreMinimal.h"
#include "GameEventNodeTypes.h"
#include "GameplayTagContainer.h"
#include "GameEventNodeUtils.generated.h"

UCLASS()
class GAMEEVENTNODE_API UGameEventNodeUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#pragma region "static"
	static bool IsDelegateMode(const UEdGraphPin* Pin);
	static bool IsStringEventId(const UEdGraphPin* Pin);
	static void ClearPinValue(UEdGraphPin* Pin);
	static FString GetEventName(const UEdGraphPin* Pin);
	static FString GetCurrentEventName(const UEdGraphPin* EventIdTypePin, const UEdGraphPin* EventTagPin, const UEdGraphPin* EventStringPin);
	static FName GetMultiParameterPinName(const FName PinName, const int32 Index);
	static UFunction* GetConvertFunction(const UEdGraphPin* DataTypePin);
#pragma endregion

#pragma region "BlueprintInternalUseOnly"
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="AddListener (FunctionName)", meta = ( WorldContext = "WorldContextObject"))
	static void AddListener_ByFuncName(UObject* WorldContextObject, const FString EventName, const FString& FunctionName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="AddListener (Delegate)", meta = ( WorldContext = "WorldContextObject"))
	static void AddListener_ByDelegate(UObject* WorldContextObject, const FString EventName, const FEventPropertyDelegate& PropertyDelegate);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="RemoveListener (BPF)", meta = ( WorldContext = "WorldContextObject"))
	static void RemoveListener(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam1", meta = (CustomStructureParam = "ParamData", WorldContext = "WorldContextObject"))
	static void SendEventParam1(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData);
	DECLARE_FUNCTION(execSendEventParam1);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam2", meta = (CustomStructureParam = "ParamData,ParamData1", WorldContext = "WorldContextObject"))
	static void SendEventParam2(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1);
	DECLARE_FUNCTION(execSendEventParam2);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam3", meta = (CustomStructureParam = "ParamData,ParamData1,ParamData2", WorldContext = "WorldContextObject"))
	static void SendEventParam3(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2);
	DECLARE_FUNCTION(execSendEventParam3);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam4", meta = (CustomStructureParam = "ParamData,ParamData1,ParamData2,ParamData3", WorldContext = "WorldContextObject"))
	static void SendEventParam4(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3);
	DECLARE_FUNCTION(execSendEventParam4);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam5", meta = (CustomStructureParam = "ParamData,ParamData1,ParamData2,ParamData3,ParamData4", WorldContext = "WorldContextObject"))
	static void SendEventParam5(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3, const int32& ParamData4);
	DECLARE_FUNCTION(execSendEventParam5);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam6", meta = (CustomStructureParam = "ParamData,ParamData1,ParamData2,ParamData3,ParamData4,ParamData5", WorldContext = "WorldContextObject"))
	static void SendEventParam6(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3, const int32& ParamData4, const int32& ParamData5);
	DECLARE_FUNCTION(execSendEventParam6);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam7", meta = (CustomStructureParam = "ParamData,ParamData1,ParamData2,ParamData3,ParamData4,ParamData5,ParamData6", WorldContext = "WorldContextObject"))
	static void SendEventParam7(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3, const int32& ParamData4, const int32& ParamData5, const int32& ParamData6);
	DECLARE_FUNCTION(execSendEventParam7);

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEventParam8", meta = (CustomStructureParam = "ParamData,ParamData1,ParamData2,ParamData3,ParamData4,ParamData5,ParamData6,ParamData7", WorldContext = "WorldContextObject"))
	static void SendEventParam8(UObject* WorldContextObject, const FString EventName, const bool bPinned, const int32& ParamData, const int32& ParamData1, const int32& ParamData2, const int32& ParamData3, const int32& ParamData4, const int32& ParamData5, const int32& ParamData6, const int32& ParamData7);
	DECLARE_FUNCTION(execSendEventParam8);

	static void ProcessEventParameters(FFrame& Stack, FEventContext& EventContext, int32 NumParams);
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="SendEvent (No Param)", meta = ( WorldContext = "WorldContextObject"))
	static void SendEvent_NoParam(UObject* WorldContextObject, const FString EventName, const bool bPinned);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="UnpinEvent", meta = ( WorldContext = "WorldContextObject"))
	static void UnpinEvent(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="HasEvent", meta = ( WorldContext = "WorldContextObject"))
	static bool HasEvent(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="GetEventListenerCount", meta = ( WorldContext = "WorldContextObject"))
	static int32 GetEventListenerCount(UObject* WorldContextObject, const FString EventName);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="RemoveAllListenersForReceiver", meta = ( WorldContext = "Receiver"))
	static void RemoveAllListenersForReceiver(UObject* Receiver);

#pragma region "Convert"
	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = "GameEventSystem", DisplayName="TagToEventName")
	static FString TagToEventName(const FGameplayTag InTag);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Boolean", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToBoolean(const FPropertyContext& InProp, bool& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Byte", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToByte(const FPropertyContext& InProp, uint8& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Integer", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToInt(const FPropertyContext& InProp, int32& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Integer64", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToInt64(const FPropertyContext& InProp, int64& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Float", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToFloat(const FPropertyContext& InProp, float& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Double", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToDouble(const FPropertyContext& InProp, double& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To String"), Category = "GameEventSystem|Conversions")
	static bool ConvertToString(const FPropertyContext& InProp, FString& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Text"), Category = "GameEventSystem|Conversions")
	static bool ConvertToText(const FPropertyContext& InProp, FText& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Name", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToName(const FPropertyContext& InProp, FName& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, CustomThunk, meta = (DisplayName = "Convert To Struct", CustomStructureParam = "OutValue", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToStruct(const FPropertyContext& InProp, TFieldPath<FProperty>& OutValue);
	DECLARE_FUNCTION(execConvertToStruct);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Object", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToObject(const FPropertyContext& InProp, UObject*& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To SoftObject", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToSoftObject(const FPropertyContext& InProp, TSoftObjectPtr<UObject>& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Class", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToClass(const FPropertyContext& InProp, UClass*& OutValue);

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To SoftClass", BlueprintAutocast), Category = "GameEventSystem|Conversions")
	static bool ConvertToSoftClass(const FPropertyContext& InProp, TSoftClassPtr<UObject>& OutValue);

	UFUNCTION(BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Array", BlueprintAutocast, CustomStructureParam = "OutValue"), Category = "GameEventSystem|Conversions")
	static bool ConvertArrayType(const FPropertyContext& InProp, TArray<uint8>& OutValue);
	DECLARE_FUNCTION(execConvertArrayType);

	UFUNCTION(BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Set", BlueprintAutocast, CustomStructureParam = "OutValue"), Category = "GameEventSystem|Conversions")
	static bool ConvertSetType(const FPropertyContext& InProp, TSet<uint8>& OutValue);
	DECLARE_FUNCTION(execConvertSetType);

	UFUNCTION(BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (DisplayName = "Convert To Map", BlueprintAutocast, CustomStructureParam = "OutValue"), Category = "GameEventSystem|Conversions")
	static bool ConvertMapType(const FPropertyContext& InProp, TMap<uint8, uint8>& OutValue);
	DECLARE_FUNCTION(execConvertMapType);

#pragma endregion

#pragma endregion
};
