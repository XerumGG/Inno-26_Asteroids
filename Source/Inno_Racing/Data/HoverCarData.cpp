// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Data/HoverCarData.h"

UHoverCarData::UHoverCarData()
{
	DisplayName = NSLOCTEXT("InnoHoverCar", "DefaultCarName", "AeroViper");
	Description = NSLOCTEXT("InnoHoverCar", "DefaultCarDesc", "Agile balanced hover racer for high-speed cosmic circuits.");

	// 5-Level Automatic Clutch Table (v2.1 spec)
	// Band 1: Drift  - 20 m/s (72 km/h),  accel 18 m/s^2, turn 120 deg/s, burn 0.6 /s
	// Band 2: Cruise - 35 m/s (126 km/h), accel 14 m/s^2, turn 100 deg/s, burn 0.9 /s
	// Band 3: Pulse  - 50 m/s (180 km/h), accel 11 m/s^2, turn  85 deg/s, burn 1.3 /s
	// Band 4: Surge  - 65 m/s (234 km/h), accel  9 m/s^2, turn  70 deg/s, burn 1.7 /s
	// Band 5: Nova   - 80 m/s (288 km/h), accel  7 m/s^2, turn  58 deg/s, burn 2.2 /s
	ClutchBands.Empty();

	FClutchBand Band1;
	Band1.Name = NSLOCTEXT("InnoHoverCar", "Band1Name", "Drift");
	Band1.TopSpeedMs = 20.0f;
	Band1.AccelerationMs2 = 18.0f;
	Band1.TurnRateDegs = 120.0f;
	Band1.FuelBurnPerSec = 0.6f;
	ClutchBands.Add(Band1);

	FClutchBand Band2;
	Band2.Name = NSLOCTEXT("InnoHoverCar", "Band2Name", "Cruise");
	Band2.TopSpeedMs = 35.0f;
	Band2.AccelerationMs2 = 14.0f;
	Band2.TurnRateDegs = 100.0f;
	Band2.FuelBurnPerSec = 0.9f;
	ClutchBands.Add(Band2);

	FClutchBand Band3;
	Band3.Name = NSLOCTEXT("InnoHoverCar", "Band3Name", "Pulse");
	Band3.TopSpeedMs = 50.0f;
	Band3.AccelerationMs2 = 11.0f;
	Band3.TurnRateDegs = 85.0f;
	Band3.FuelBurnPerSec = 1.3f;
	ClutchBands.Add(Band3);

	FClutchBand Band4;
	Band4.Name = NSLOCTEXT("InnoHoverCar", "Band4Name", "Surge");
	Band4.TopSpeedMs = 65.0f;
	Band4.AccelerationMs2 = 9.0f;
	Band4.TurnRateDegs = 70.0f;
	Band4.FuelBurnPerSec = 1.7f;
	ClutchBands.Add(Band4);

	FClutchBand Band5;
	Band5.Name = NSLOCTEXT("InnoHoverCar", "Band5Name", "Nova");
	Band5.TopSpeedMs = 80.0f;
	Band5.AccelerationMs2 = 7.0f;
	Band5.TurnRateDegs = 58.0f;
	Band5.FuelBurnPerSec = 2.2f;
	ClutchBands.Add(Band5);

	CoastDecelMs2 = 8.0f;
	BrakeDecelMs2 = 40.0f;
	UpshiftThresholdRatio = 0.97f;
	UpshiftDwellSec = 0.15f;
	DownshiftThresholdRatio = 0.90f;
	LateralGripDecayRate = 6.0f;

	HoverHeightCm = 120.0f;
	RayTraceLengthCm = 500.0f;
	OffTrackRespawnSec = 1.5f;
	HoverStiffnessGain = 8.0f;
	HoverDampingSpeed = 14.0f;
	AirborneGravityCms2 = 980.0f;
	MaxFallSpeedCms = 1200.0f;
	VisualRollTiltMaxDeg = 22.0f;
	VisualPitchTiltMaxDeg = 8.0f;

	FuelCapacity = 100.0f;
	LowFuelAlertThreshold = 20.0f;
	FuelPickupAmount = 20.0f;

	MaxBoostCharges = 3;
	BoostDurationSec = 2.5f;
	BoostAccelMultiplier = 2.0f;
	BoostTopSpeedMultiplier = 1.35f;
	BoostTurnRateMultiplier = 0.85f;
}
