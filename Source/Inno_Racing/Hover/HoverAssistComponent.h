// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoverAssistComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOffTrackDetectedSignature);

/**
 * Game-based hover assist:
 * - 4 downward raycasts from nozzle sockets to track surface
 * - Smooth damped height hold (no physics springs)
 * - Track normal alignment for banked/curved track geometry
 * - Visual-only mesh banking (roll on steer, pitch on accel/brake)
 * - 1.5s off-track timeout and respawn trigger
 * - Smooth landed-to-hover takeoff transition (0.4s)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INNO_RACING_API UHoverAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoverAssistComponent();

	void InitializeAssist(float InHoverHeightCm, float InTraceLengthCm, float InOffTrackSec, float InMaxRollDeg, float InMaxPitchDeg);

	// Initiates takeoff sequence at GO (lifts from landed to hover height over 0.4s)
	UFUNCTION(BlueprintCallable, Category = "Hover|Assist")
	void StartTakeoff();

	// Updates hover assist, track alignment, visual mesh banking, and off-track detection
	void UpdateHoverAssist(
		UPrimitiveComponent* PhysicsBody,
		USceneComponent* VisualMesh,
		const TArray<USceneComponent*>& Nozzles,
		float SteerInput,
		float AccelInput,
		float DeltaTime
	);

	UFUNCTION(BlueprintPure, Category = "Hover|Assist")
	bool IsGrounded() const { return bIsGrounded; }

	UFUNCTION(BlueprintPure, Category = "Hover|Assist")
	float GetGroundedRatio() const { return GroundedRatio; }

	UPROPERTY(BlueprintAssignable, Category = "Hover|Assist")
	FOnOffTrackDetectedSignature OnOffTrackDetected;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float TargetHoverHeight = 120.0f; // 1.2 m in cm

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float TraceLength = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float OffTrackTimeoutSec = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float OffTrackTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	bool bIsGrounded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hover|Assist")
	float GroundedRatio = 0.0f;

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
