#include "GameEventSystemSettings.h"

UGameEventSystemSettings::UGameEventSystemSettings(): bEnableDebugMode(false)
{
}

FName UGameEventSystemSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FText UGameEventSystemSettings::GetSectionText() const
{
	return NSLOCTEXT("GameEventSystemSettings", "SectionText", "Game Event System");
}

FText UGameEventSystemSettings::GetSectionDescription() const
{
	return NSLOCTEXT("GameEventSystemSettings",
	                 "SectionDescription",
	                 "Configure logging settings for the Game Event System plugin");
}

#if WITH_EDITOR
bool UGameEventSystemSettings::CanEditChange(const FProperty* InProperty) const
{
	return Super::CanEditChange(InProperty);
}

void UGameEventSystemSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

const UGameEventSystemSettings* UGameEventSystemSettings::Get()
{
	return GetDefault<UGameEventSystemSettings>();
}
