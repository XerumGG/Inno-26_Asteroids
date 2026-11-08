// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Hover/HoverClutchComponent.h"

FClutchBand UHoverClutchComponent::FallbackBand;

UHoverClutchComponent::UHoverClutchComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	if (FallbackBand.Name.IsEmpty())
	{
		FallbackBand.Name = NSLOCTEXT("InnoHoverCar", "DefaultBand", "Drift");
		FallbackBand.TopSpeedMs = 20.0f;
		FallbackBand.AccelerationMs2 = 18.0f;
		FallbackBand.TurnRateDegs = 120.0f;
		FallbackBand.FuelBurnPerSec = 0.6f;
	}
}

void UHoverClutchComponent::InitializeClutch(const UHoverCarData* InCarData)
{
	if (!InCarData)
	{
		return;
	}

	CachedBands = InCarData->ClutchBands;
	UpshiftThresholdRatio = InCarData->UpshiftThresholdRatio;
	UpshiftDwellSec = InCarData->UpshiftDwellSec;
	DownshiftThresholdRatio = InCarData->DownshiftThresholdRatio;

	CurrentClutchLevel = 1;
	UpshiftDwellTimer = 0.0f;
	bIsLimpMode = false;
	bIsEngaged = false;
}

void UHoverClutchComponent::EngageClutch()
{
	bIsEngaged = true;
	CurrentClutchLevel = 1;
	UpshiftDwellTimer = 0.0f;
	OnClutchChanged.Broadcast(CurrentClutchLevel, GetCurrentBand().Name);
}

void UHoverClutchComponent::DisengageClutch()
{
	bIsEngaged = false;
	CurrentClutchLevel = 1;
	UpshiftDwellTimer = 0.0f;
}

void UHoverClutchComponent::SetLimpMode(bool bInLimpMode)
{
	bIsLimpMode = bInLimpMode;
	if (bIsLimpMode && CurrentClutchLevel > 1)
	{
		CurrentClutchLevel = 1;
		UpshiftDwellTimer = 0.0f;
		OnClutchChanged.Broadcast(CurrentClutchLevel, GetCurrentBand().Name);
	}
}

const FClutchBand& UHoverClutchComponent::GetCurrentBand() const
{
	if (CachedBands.IsValidIndex(CurrentClutchLevel - 1))
	{
		return CachedBands[CurrentClutchLevel - 1];
	}
	return FallbackBand;
}

void UHoverClutchComponent::UpdateAutoShift(float CurrentForwardSpeedMs, bool bIsBoosting, float DeltaTime)
{
	if (!bIsEngaged || CachedBands.Num() == 0)
	{
		return;
	}

	// Limp mode locks the clutch to Band 1
	if (bIsLimpMode)
	{
		if (CurrentClutchLevel != 1)
		{
			CurrentClutchLevel = 1;
			OnClutchChanged.Broadcast(CurrentClutchLevel, GetCurrentBand().Name);
		}
		return;
	}

	const int32 MaxLevel = CachedBands.Num();
	const FClutchBand& ActiveBand = GetCurrentBand();

	// 1. Check for UPSHIFT
	if (CurrentClutchLevel < MaxLevel)
	{
		const float UpshiftTargetSpeed = ActiveBand.TopSpeedMs * UpshiftThresholdRatio;

		if (CurrentForwardSpeedMs >= UpshiftTargetSpeed)
		{
			// Instant upshift while boosting, otherwise respect dwell time
			if (bIsBoosting)
			{
				CurrentClutchLevel++;
				UpshiftDwellTimer = 0.0f;
				OnClutchChanged.Broadcast(CurrentClutchLevel, GetCurrentBand().Name);
				return;
			}
			else
			{
				UpshiftDwellTimer += DeltaTime;
				if (UpshiftDwellTimer >= UpshiftDwellSec)
				{
					CurrentClutchLevel++;
					UpshiftDwellTimer = 0.0f;
					OnClutchChanged.Broadcast(CurrentClutchLevel, GetCurrentBand().Name);
					return;
				}
			}
		}
		else
		{
			UpshiftDwellTimer = 0.0f;
		}
	}
	else
	{
		UpshiftDwellTimer = 0.0f;
	}

	// 2. Check for DOWNSHIFT
	// Downshift when speed drops below 90% of the PREVIOUS band's top speed
	if (CurrentClutchLevel > 1)
	{
		const FClutchBand& PreviousBand = CachedBands[CurrentClutchLevel - 2];
		const float DownshiftThresholdSpeed = PreviousBand.TopSpeedMs * DownshiftThresholdRatio;

		if (CurrentForwardSpeedMs < DownshiftThresholdSpeed)
		{
			CurrentClutchLevel--;
			UpshiftDwellTimer = 0.0f;
			OnClutchChanged.Broadcast(CurrentClutchLevel, GetCurrentBand().Name);
		}
	}
}
