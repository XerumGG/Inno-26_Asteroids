// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoverBoostComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoostActivatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoostEndedSignature);

/**
 * Manages timed speed boost.
 * Automatically triggered when driving over a Boost Pickup orb (no button press needed).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INNO_RACING_API UHoverBoostComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoverBoostComponent();

	void InitializeBoost(float InDuration, float InAccelMult, float InTopSpeedMult, float InTurnMult, int32 InMaxCharges = 3);

	// Automatically triggers immediate rocket boost (called upon collecting a boost pickup)
	UFUNCTION(BlueprintCallable, Category = "Hover|Boost")
	void TriggerInstantBoost(float DurationOverride = -1.0f);

	// Cancels any active boost (e.g. on off-track respawn)
	UFUNCTION(BlueprintCallable, Category = "Hover|Boost")
	void StopBoost();

	// Ticks boost timer
	void UpdateBoost(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Hover|Boost")
	bool IsBoosting() const { return bIsBoosting; }

	UFUNCTION(BlueprintPure, Category = "Hover|Boost")
	float GetBoostTimeRemaining() const { return BoostTimerRemaining; }

	float GetAccelMultiplier() const { return bIsBoosting ? BoostAccelMultiplier : 1.0f; }
	float GetTopSpeedMultiplier() const { return bIsBoosting ? BoostTopSpeedMultiplier : 1.0f; }
	float GetTurnRateMultiplier() const { return bIsBoosting ? BoostTurnRateMultiplier : 1.0f; }

	UPROPERTY(BlueprintAssignable, Category = "Hover|Boost")
	FOnBoostActivatedSignature OnBoostActivated;

	UPROPERTY(BlueprintAssignable, Category = "Hover|Boost")
	FOnBoostEndedSignature OnBoostEnded;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Boost")
	float BoostDurationSec = 2.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Boost")
	float BoostTimerRemaining = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Boost")
	bool bIsBoosting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Boost")
	int32 MaxBoostCharges = 3;

	float BoostAccelMultiplier = 2.0f;
	float BoostTopSpeedMultiplier = 1.35f;
	float BoostTurnRateMultiplier = 0.85f;
};
