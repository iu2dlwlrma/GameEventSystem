#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "EdGraph/EdGraphPin.h"

class UK2Node_AddListener;

class GAMEEVENTNODE_API SGraphNodeAddListener final : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeAddListener)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UK2Node_AddListener* InNode);

protected:
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
};
