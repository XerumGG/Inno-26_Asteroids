// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Hover/HoverBoostComponent.h"

UHoverBoostComponent::UHoverBoostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BoostDurationSec = 2.5f;
	BoostTimerRemaining = 0.0f;
	bIsBoosting = false;
	BoostAccelMultiplier = 2.0f;
	BoostTopSpeedMultiplier = 1.35f;
	BoostTurnRateMultiplier = 0.85f;
}

void UHoverBoostComponent::InitializeBoost(float InDuration, float InAccelMult, float InTopSpeedMult, float InTurnMult)
{
	BoostDurationSec = InDuration;
	BoostAccelMultiplier = InAccelMult;
	BoostTopSpeedMultiplier = InTopSpeedMult;
	BoostTurnRateMultiplier = InTurnMult;
	BoostTimerRemaining = 0.0f;
	bIsBoosting = false;
}

void UHoverBoostComponent::TriggerInstantBoost(float DurationOverride)
{
	const float Duration = (DurationOverride > 0.0f) ? DurationOverride : BoostDurationSec;

	// If already boosting, extend/refresh timer
	BoostTimerRemaining = FMath::Max(BoostTimerRemaining, Duration);

	if (!bIsBoosting)
	{
		bIsBoosting = true;
		OnBoostActivated.Broadcast();
	}
}

void UHoverBoostComponent::UpdateBoost(float DeltaTime)
{
	if (!bIsBoosting)
	{
		return;
	}

	BoostTimerRemaining -= DeltaTime;
	if (BoostTimerRemaining <= 0.0f)
	{
		bIsBoosting = false;
		BoostTimerRemaining = 0.0f;
		OnBoostEnded.Broadcast();
	}
}
