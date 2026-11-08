// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoverFuelComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFuelChangedSignature, float, CurrentFuel, float, MaxFuel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLowFuelAlertSignature, bool, bIsLowFuel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLimpModeToggledSignature, bool, bInLimpMode);

/**
 * Manages fuel capacity, burning during acceleration, limp mode, and fuel pickups.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INNO_RACING_API UHoverFuelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoverFuelComponent();

	void InitializeFuel(float InCapacity, float InLowThreshold);

	// Burns fuel only while accelerating
	void BurnFuel(float BurnRatePerSec, float DeltaTime);

	// Collects fuel from a pickup
	UFUNCTION(BlueprintCallable, Category = "Hover|Fuel")
	void AddFuel(float Amount);

	UFUNCTION(BlueprintPure, Category = "Hover|Fuel")
	float GetCurrentFuel() const { return CurrentFuel; }

	UFUNCTION(BlueprintPure, Category = "Hover|Fuel")
	float GetMaxFuel() const { return MaxFuel; }

	UFUNCTION(BlueprintPure, Category = "Hover|Fuel")
	float GetFuelPercent() const { return (MaxFuel > 0.0f) ? (CurrentFuel / MaxFuel) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Hover|Fuel")
	bool IsInLimpMode() const { return bInLimpMode; }

	UPROPERTY(BlueprintAssignable, Category = "Hover|Fuel")
	FOnFuelChangedSignature OnFuelChanged;

	UPROPERTY(BlueprintAssignable, Category = "Hover|Fuel")
	FOnLowFuelAlertSignature OnLowFuelAlert;

	UPROPERTY(BlueprintAssignable, Category = "Hover|Fuel")
	FOnLimpModeToggledSignature OnLimpModeToggled;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Fuel")
	float CurrentFuel = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Fuel")
	float MaxFuel = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Fuel")
	float LowFuelThreshold = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Fuel")
	bool bInLimpMode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Fuel")
	bool bAlertActive = false;
};
