// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HoverCarData.generated.h"

/**
 * Defines a single automatic clutch band with its speed cap, acceleration,
 * turning agility, and fuel consumption rate.
 */
USTRUCT(BlueprintType)
struct FClutchBand
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch")
	FText Name = FText::GetEmpty();

	// Maximum speed achievable in this band in meters/second
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "5.0"))
	float TopSpeedMs = 20.0f;

	// Forward acceleration in m/s^2
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "1.0"))
	float AccelerationMs2 = 18.0f;

	// Maximum yaw turn rate in degrees/second
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "10.0"))
	float TurnRateDegs = 120.0f;

	// Fuel burned per second while accelerating in this band
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "0.0"))
	float FuelBurnPerSec = 0.6f;
};

/**
 * Data Asset defining physical tunings, 5-level clutch tables,
 * fuel capacity, and boost behavior for a hover car archetype.
 */
UCLASS(BlueprintType)
class INNO_RACING_API UHoverCarData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UHoverCarData();

	// --- Visual & Identity ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	// --- 5-Level Automatic Clutch Table ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (TitleProperty = "Name"))
	TArray<FClutchBand> ClutchBands;

	// Coasting deceleration when no throttle is applied (m/s^2)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "1.0"))
	float CoastDecelMs2 = 8.0f;

	// Active braking deceleration (m/s^2)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "5.0"))
	float BrakeDecelMs2 = 40.0f;

	// Upshift speed ratio threshold (e.g. 0.97 = 97% of current band top speed)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "0.8", ClampMax = "1.0"))
	float UpshiftThresholdRatio = 0.97f;

	// Time required at or above upshift threshold before upshifting (seconds)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UpshiftDwellSec = 0.15f;

	// Downshift speed ratio threshold (e.g. 0.90 = below 90% of PREVIOUS band top speed)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clutch", meta = (ClampMin = "0.7", ClampMax = "1.0"))
	float DownshiftThresholdRatio = 0.90f;

	// Rate at which lateral sideways slip decays (gives light drift feel)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "1.0"))
	float LateralGripDecayRate = 6.0f;

	// --- Hover Assist ---
	// Target hover height from track in cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "30.0"))
	float HoverHeightCm = 120.0f;

	// Maximum raycast length downward in cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "100.0"))
	float RayTraceLengthCm = 500.0f;

	// Time without track surface beneath before off-track respawn is triggered
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "0.5"))
	float OffTrackRespawnSec = 1.5f;

	// Vertical height correction gain (stiffness)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "1.0"))
	float HoverStiffnessGain = 8.0f;

	// Vertical velocity damping interpolation speed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "1.0"))
	float HoverDampingSpeed = 14.0f;

	// Downward acceleration when airborne (cm/s^2)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "100.0"))
	float AirborneGravityCms2 = 980.0f;

	// Maximum downward fall speed when airborne (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist", meta = (ClampMin = "200.0"))
	float MaxFallSpeedCms = 1200.0f;

	// Maximum visual mesh roll banking in degrees during turns
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist|Visuals")
	float VisualRollTiltMaxDeg = 22.0f;

	// Maximum visual mesh pitch tilt in degrees during accel/brake
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HoverAssist|Visuals")
	float VisualPitchTiltMaxDeg = 8.0f;

	// --- Fuel System ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fuel", meta = (ClampMin = "10.0"))
	float FuelCapacity = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fuel", meta = (ClampMin = "1.0"))
	float LowFuelAlertThreshold = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fuel", meta = (ClampMin = "1.0"))
	float FuelPickupAmount = 20.0f;

	// --- Boost System ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost", meta = (ClampMin = "1"))
	int32 MaxBoostCharges = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost", meta = (ClampMin = "0.5"))
	float BoostDurationSec = 2.5f;

	// Acceleration multiplier during boost
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost", meta = (ClampMin = "1.1"))
	float BoostAccelMultiplier = 2.0f;

	// Multiplier over Clutch 5 top speed (e.g. 1.35 * 80 m/s = 108 m/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost", meta = (ClampMin = "1.0"))
	float BoostTopSpeedMultiplier = 1.35f;

	// Turn agility multiplier while boosting (narrower turning radius)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float BoostTurnRateMultiplier = 0.85f;
};
