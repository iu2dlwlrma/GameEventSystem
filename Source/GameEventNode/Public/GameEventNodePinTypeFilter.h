#pragma once

#include "CoreMinimal.h"
#include "PinTypeSelectorFilter.h"

class FGameEventNodePinTypeFilter final : public IPinTypeSelectorFilter
{
	virtual bool ShouldShowPinTypeTreeItem(const FPinTypeTreeItem InItem) const override
	{
		const FEdGraphPinType PinType = InItem->GetPinType(false);

		if (PinType.PinCategory == UEdGraphSchema_K2::PC_Interface ||
		    PinType.PinCategory == UEdGraphSchema_K2::PC_Exec ||
		    PinType.PinCategory == UEdGraphSchema_K2::PC_Delegate ||
		    PinType.PinCategory == UEdGraphSchema_K2::PC_MCDelegate)
		{
			return false;
		}

		return true;
	};
};
