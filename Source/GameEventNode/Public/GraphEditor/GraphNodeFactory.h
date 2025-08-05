// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"
#include "GraphEditor/SGraphNodeAddListener.h"
#include "K2Node/K2Node_AddListener.h"
#include "SGraphNode.h"

class FGameEventGraphNodeFactory final : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		if (UK2Node_AddListener* AddListenerNode = Cast<UK2Node_AddListener>(Node))
		{
			return SNew(SGraphNodeAddListener, AddListenerNode);
		}

		return nullptr;
	}
};
