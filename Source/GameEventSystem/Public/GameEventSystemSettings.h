#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameEventSystemSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Game Event System"))
class GAMEEVENTSYSTEM_API UGameEventSystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGameEventSystemSettings();

	//~ Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UDeveloperSettings Interface

	static const UGameEventSystemSettings* Get();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (DisplayName = "Enable Debug Log", ToolTip = "Enable debugging log"))
	bool bEnableDebug;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (DisplayName = "Enable Node Debug Log", ToolTip = "Enable blueprint node debugging log"))
	bool bEnableNodeDebug;
};
