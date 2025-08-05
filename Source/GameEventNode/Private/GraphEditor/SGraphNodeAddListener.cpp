#include "GraphEditor/SGraphNodeAddListener.h"
#include "K2Node/K2Node_AddListener.h"
#include "GraphEditor/SGraphPinDataType.h"

#define LOCTEXT_NAMESPACE "SGraphNodeAddListener"

void SGraphNodeAddListener::Construct(const FArguments& InArgs, UK2Node_AddListener* InNode)
{
	GraphNode = InNode;

	UpdateGraphNode();
}

TSharedPtr<SGraphPin> SGraphNodeAddListener::CreatePinWidget(UEdGraphPin* Pin) const
{
	if (Pin && Pin->PinName == FName(TEXT("DataType")))
	{
		return SNew(SGraphPinDataType, Pin);
	}

	return SGraphNode::CreatePinWidget(Pin);
}

#undef LOCTEXT_NAMESPACE
