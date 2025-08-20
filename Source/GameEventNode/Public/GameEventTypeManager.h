#pragma once

#include "CoreMinimal.h"
#include "GameEventNodeTypes.h"

class GAMEEVENTNODE_API FGameEventTypeManager
{
public:
	FGameEventTypeManager();
	~FGameEventTypeManager();
	static FGameEventTypeManager* Get();

	static void GetPinTypeFromProperty(FProperty* Property, FName& OutPinCategory, FName& OutPinSubCategory, UObject*& OutSubCategoryObject, FEdGraphTerminalType& OutPinValueType, EPinContainerType& OutContainerType);
	static void AddTypeInfoFromProperty(FProperty* Property, FEventTypeInfo& OutTypeInfo);
	static bool AnalyzeUFunctionParameters(const UFunction* Function, FEventTypeInfo& OutTypeInfo);
	static bool AnalyzeFunctionSignature(const UClass* ObjectClass, const FString& FunctionName, FEventTypeInfo& OutTypeInfo);

	void RegisterEventType(const FString& EventName, const FEventTypeInfo& TypeInfo);
	bool GetEventTypeInfo(const FString& EventName, FEventTypeInfo& OutTypeInfo) const;
	bool IsEventRegistered(const FString& EventName) const;
	void UnregisterEventType(const FString& EventName);
	void ClearAllEventTypes();
	TArray<FString> GetAllRegisteredEventNames() const;

	bool AnalyzeAndRegisterFunctionType(const FString& EventName, const UClass* Receiver, const FString& FunctionName);

protected:
	FEventTypeRegistry TypeRegistry;

private:
	static TSharedPtr<FGameEventTypeManager> TypeManagerPtr;
};
