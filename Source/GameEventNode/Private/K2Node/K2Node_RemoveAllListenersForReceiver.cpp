#include "K2Node/K2Node_RemoveAllListenersForReceiver.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "GameEventNodeUtils.h"
#include "KismetCompiler.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"

#define LOCTEXT_NAMESPACE "UK2Node_RemoveAllListenersForReceiver"

void UK2Node_RemoveAllListenersForReceiver::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self);
	SelfPin->PinFriendlyName = NSLOCTEXT("GameEventNode", "Target", "Target");
	SelfPin->PinToolTip = NSLOCTEXT("GameEventNode", "Target_Tooltip", "Target object for listening").ToString();
}

FText UK2Node_RemoveAllListenersForReceiver::GetTooltipText() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "RemoveAllListenersForReceiver_Tooltip", "Remove all event listeners registered by the specified receiver object. [{0}]"), FText::FromName(GetFName()));
}

FText UK2Node_RemoveAllListenersForReceiver::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("GameEventNode", "RemoveAllListenersForReceiver_Title", "Remove All Listeners For Receiver");
}

FText UK2Node_RemoveAllListenersForReceiver::GetKeywords() const
{
	return FText::Format(NSLOCTEXT("GameEventNode", "RemoveAllListeners_Keywords", "{0} remove all listeners receiver cleanup clear unbind unsubscribe event game system gameevent game event"), Super::GetKeywords());
}

FText UK2Node_RemoveAllListenersForReceiver::GetMenuCategory() const
{
	return NSLOCTEXT("GameEventNode", "GameEventCategory", "GameEventSystem");
}

void UK2Node_RemoveAllListenersForReceiver::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const UClass* ActionKey = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UK2Node_RemoveAllListenersForReceiver::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName ReceiverParamName(TEXT("Receiver"));

	const UFunction* RemoveAllListenersForReceiverFunction = UGameEventNodeUtils::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameEventNodeUtils, RemoveAllListenersForReceiver));

	if (!RemoveAllListenersForReceiverFunction)
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("GameEventNode", "RemoveAllListenersForReceiverFunctionNotFound", "RemoveAllListenersForReceiver function not found").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFuncNode->FunctionReference.SetExternalMember(RemoveAllListenersForReceiverFunction->GetFName(), UGameEventNodeUtils::StaticClass());
	CallFuncNode->AllocateDefaultPins();

	UEdGraphPin* CallFuncExecPin = CallFuncNode->GetExecPin();
	UEdGraphPin* CallFuncThenPin = CallFuncNode->GetThenPin();

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallFuncExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *CallFuncThenPin);

	if (UEdGraphPin* CallFuncReceiverPin = CallFuncNode->FindPin(ReceiverParamName))
	{
		CompilerContext.MovePinLinksToIntermediate(*GetReceiverPin(), *CallFuncReceiverPin);
	}

	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_RemoveAllListenersForReceiver::GetReceiverPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Self);
}

#undef LOCTEXT_NAMESPACE
