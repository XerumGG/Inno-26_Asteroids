// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Hover/HoverFuelComponent.h"

UHoverFuelComponent::UHoverFuelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentFuel = 100.0f;
	MaxFuel = 100.0f;
	LowFuelThreshold = 20.0f;
	bInLimpMode = false;
	bAlertActive = false;
}

void UHoverFuelComponent::InitializeFuel(float InCapacity, float InLowThreshold)
{
	MaxFuel = FMath::Max(1.0f, InCapacity);
	CurrentFuel = MaxFuel;
	LowFuelThreshold = FMath::Clamp(InLowThreshold, 0.0f, MaxFuel);
	bInLimpMode = false;
	bAlertActive = false;

	OnFuelChanged.Broadcast(CurrentFuel, MaxFuel);
}

void UHoverFuelComponent::BurnFuel(float BurnRatePerSec, float DeltaTime)
{
	if (bInLimpMode || CurrentFuel <= 0.0f || BurnRatePerSec <= 0.0f)
	{
		return;
	}

	CurrentFuel = FMath::Max(0.0f, CurrentFuel - (BurnRatePerSec * DeltaTime));
	OnFuelChanged.Broadcast(CurrentFuel, MaxFuel);

	// Low Fuel Alert Check
	if (CurrentFuel <= LowFuelThreshold && !bAlertActive)
	{
		bAlertActive = true;
		OnLowFuelAlert.Broadcast(true);
	}

	// Limp Mode Trigger
	if (CurrentFuel <= 0.0f && !bInLimpMode)
	{
		bInLimpMode = true;
		OnLimpModeToggled.Broadcast(true);
	}
}

void UHoverFuelComponent::AddFuel(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentFuel = FMath::Min(MaxFuel, CurrentFuel + Amount);
	OnFuelChanged.Broadcast(CurrentFuel, MaxFuel);

	// Exit Low Fuel Alert if above threshold
	if (CurrentFuel > LowFuelThreshold && bAlertActive)
	{
		bAlertActive = false;
		OnLowFuelAlert.Broadcast(false);
	}

	// Exit Limp Mode if fuel restored
	if (CurrentFuel > 0.0f && bInLimpMode)
	{
		bInLimpMode = false;
		OnLimpModeToggled.Broadcast(false);
	}
}
