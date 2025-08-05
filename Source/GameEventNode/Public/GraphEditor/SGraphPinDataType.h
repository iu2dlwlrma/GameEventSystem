#pragma once

#include "CoreMinimal.h"
#include "SGraphPin.h"
#include "EdGraph/EdGraphPin.h"

class GAMEEVENTNODE_API SGraphPinDataType final : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPinDataType)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	// End SGraphPin interface

private:
	FEdGraphPinType OnGetPinType() const;

	void OnTypeSelectionChanged(const FEdGraphPinType& NewType) const;
};
