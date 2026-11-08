// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoverAssistComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOffTrackDetectedSignature);

/**
 * Game-based hover assist:
 * - 4 downward raycasts from nozzle sockets to track surface (with upward start offset to prevent floor penetration misses)
 * - Smooth damped height hold along track surface normal
 * - Quaternion-based track normal alignment + yaw steering (gimbal-lock free on banked/sloped tracks)
 * - Visual-only mesh banking (roll into turn on steer, pitch on accel/brake)
 * - 1.5s off-track timeout and respawn trigger
 * - Smooth landed-to-hover takeoff transition (0.4s)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INNO_RACING_API UHoverAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoverAssistComponent();

	void InitializeAssist(
		float InHoverHeightCm,
		float InTraceLengthCm,
		float InOffTrackSec,
		float InMaxRollDeg,
		float InMaxPitchDeg,
		float InStiffnessGain = 8.0f,
		float InDampingSpeed = 14.0f,
		float InAirborneGravityCms2 = 980.0f,
		float InMaxFallSpeedCms = 1200.0f
	);

	// Initiates takeoff sequence at GO (lifts from landed to hover height over 0.4s)
	UFUNCTION(BlueprintCallable, Category = "Hover|Assist")
	void StartTakeoff();

	// Resets timers, smoothed normals, and visual tilt after a checkpoint respawn
	UFUNCTION(BlueprintCallable, Category = "Hover|Assist")
	void ResetAssistState();

	/**
	 * Updates hover assist raycasts, quaternion orientation (yaw + track normal alignment),
	 * visual mesh banking, and computes the vertical/normal velocity component.
	 */
	void UpdateHoverAssist(
		UPrimitiveComponent* PhysicsBody,
		USceneComponent* VisualMesh,
		const TArray<USceneComponent*>& Nozzles,
		float YawDeltaDeg,
		float SteerInput,
		float AccelInput,
		float BrakeInput,
		float DeltaTime,
		FVector& InOutVerticalVelocity
	);

	// Updates only the visual mesh tilt (used on remote/simulated proxies)
	void UpdateVisualTiltOnly(USceneComponent* VisualMesh, float SteerInput, float AccelInput, float BrakeInput, float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Hover|Assist")
	bool IsGrounded() const { return bIsGrounded; }

	UFUNCTION(BlueprintPure, Category = "Hover|Assist")
	float GetGroundedRatio() const { return GroundedRatio; }

	UFUNCTION(BlueprintPure, Category = "Hover|Assist")
	FVector GetSmoothedTrackNormal() const { return SmoothedTrackNormal; }

	UPROPERTY(BlueprintAssignable, Category = "Hover|Assist")
	FOnOffTrackDetectedSignature OnOffTrackDetected;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float TargetHoverHeight = 120.0f; // 1.2 m in cm

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float TraceLength = 500.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float OffTrackTimeoutSec = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float OffTrackTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	bool bIsGrounded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float GroundedRatio = 0.0f;

	// Hover dynamics tuning
	float HoverStiffnessGain = 8.0f;
	float HoverDampingSpeed = 14.0f;
	float AirborneGravityCms2 = 980.0f;
	float MaxFallSpeedCms = 1200.0f;

	// Visual banking limits
	float VisualRollTiltMaxDeg = 22.0f;
	float VisualPitchTiltMaxDeg = 8.0f;

	// Takeoff interpolation
	float CurrentAltitudeOffset = 0.0f;
	float TakeoffDuration = 0.4f;
	float TakeoffElapsed = 0.0f;
	bool bInTakeoff = false;

	// Smoothed values
	FRotator CurrentVisualTilt = FRotator::ZeroRotator;
	FVector SmoothedTrackNormal = FVector::UpVector;
};
