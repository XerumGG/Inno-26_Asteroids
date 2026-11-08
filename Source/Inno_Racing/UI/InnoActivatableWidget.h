// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoActivatableWidget.generated.h"

/**
 * CommonUI activatable widget base class for all Inno_Race screens.
 * Automatically coordinates gamepad focus, Back action routing, and device glyph switching.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class INNO_RACING_API UInnoActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UInnoActivatableWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Screen")
	EInnoScreen ScreenType = EInnoScreen::None;
};
