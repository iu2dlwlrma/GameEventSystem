#include "GraphEditor/SGraphPinDataType.h"
#include "EdGraphSchema_K2.h"
#include "GameEventNodePinTypeFilter.h"
#include "SPinTypeSelector.h"
#include "Widgets/Layout/SBox.h"
#include "EdGraph/EdGraphNode.h"
#include "ScopedTransaction.h"

void SGraphPinDataType::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SGraphPinDataType::GetDefaultValueWidget()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	TArray<TSharedPtr<IPinTypeSelectorFilter>> CustomPinTypeFilters;
	CustomPinTypeFilters.Add(MakeShareable(new FGameEventNodePinTypeFilter));
	return SNew(SBox)
		.MaxDesiredWidth(280.0f)
		.MinDesiredWidth(80.0f)
		.HeightOverride(30.0f)
		[
			SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(K2Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
			.TargetPinType(this, &SGraphPinDataType::OnGetPinType)
			.OnPinTypeChanged(this, &SGraphPinDataType::OnTypeSelectionChanged)
			.Schema(K2Schema)
			.TypeTreeFilter(ETypeTreeFilter::None)
			.SelectorType(SPinTypeSelector::ESelectorType::Full)
			.bAllowArrays(true)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.IsEnabled(true)
			.CustomFilters(CustomPinTypeFilters)
		];
}

FEdGraphPinType SGraphPinDataType::OnGetPinType() const
{
	return GraphPinObj->PinType;
}

void SGraphPinDataType::OnTypeSelectionChanged(const FEdGraphPinType& NewType) const
{
	if (GraphPinObj)
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("Change Data Type")));

		GraphPinObj->PinType = NewType;

		if (UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode())
		{
			OwningNode->PinTypeChanged(GraphPinObj);
		}
	}
}
