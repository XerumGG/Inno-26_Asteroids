// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inno_Racing/Data/HoverCarData.h"
#include "HoverClutchComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClutchChangedSignature, int32, NewClutch, const FText&, BandName);

/**
 * Manages the automatic 5-level speed clutch with hysteresis and dwell timers.
 * Pure logic state machine with no manual shifting.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INNO_RACING_API UHoverClutchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoverClutchComponent();

	// Initializes clutch configuration from car data asset
	void InitializeClutch(const UHoverCarData* InCarData);

	// Engages Clutch 1 at race start (GO)
	UFUNCTION(BlueprintCallable, Category = "Hover|Clutch")
	void EngageClutch();

	// Disengages clutch before race or on finish
	UFUNCTION(BlueprintCallable, Category = "Hover|Clutch")
	void DisengageClutch();

	// Updates the automatic shift state machine based on current forward speed
	void UpdateAutoShift(float CurrentForwardSpeedMs, bool bIsBoosting, float DeltaTime);

	// Sets limp mode (locks to Clutch 1)
	UFUNCTION(BlueprintCallable, Category = "Hover|Clutch")
	void SetLimpMode(bool bInLimpMode);

	// Forces a specific clutch level (e.g. for remote proxy sync)
	void SetClutchLevelDirect(int32 NewLevel);

	// Current active clutch level (1 to 5)
	UFUNCTION(BlueprintPure, Category = "Hover|Clutch")
	int32 GetCurrentClutchLevel() const { return CurrentClutchLevel; }

	// Gets the active band data
	const FClutchBand& GetCurrentBand() const;

	// Checks if clutch is active
	UFUNCTION(BlueprintPure, Category = "Hover|Clutch")
	bool IsClutchEngaged() const { return bIsEngaged; }

	// Checks if locked in limp mode
	UFUNCTION(BlueprintPure, Category = "Hover|Clutch")
	bool IsInLimpMode() const { return bIsLimpMode; }

	UPROPERTY(BlueprintAssignable, Category = "Hover|Clutch")
	FOnClutchChangedSignature OnClutchChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Clutch")
	int32 CurrentClutchLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Clutch")
	bool bIsEngaged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Clutch")
	bool bIsLimpMode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Clutch")
	float UpshiftDwellTimer = 0.0f;

	UPROPERTY()
	TArray<FClutchBand> CachedBands;

	float UpshiftThresholdRatio = 0.97f;
	float UpshiftDwellSec = 0.15f;
	float DownshiftThresholdRatio = 0.90f;

private:
	static FClutchBand FallbackBand;
};
