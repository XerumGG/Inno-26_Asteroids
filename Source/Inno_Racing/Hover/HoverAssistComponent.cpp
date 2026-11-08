// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Hover/HoverAssistComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UHoverAssistComponent::UHoverAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TargetHoverHeight = 120.0f;
	TraceLength = 300.0f;
	OffTrackTimeoutSec = 1.5f;
	OffTrackTimer = 0.0f;
	bIsGrounded = false;
	GroundedRatio = 0.0f;
	VisualRollTiltMaxDeg = 22.0f;
	VisualPitchTiltMaxDeg = 8.0f;
	CurrentAltitudeOffset = 0.0f;
	TakeoffDuration = 0.4f;
	TakeoffElapsed = 0.0f;
	bInTakeoff = false;
	CurrentVisualTilt = FRotator::ZeroRotator;
	SmoothedTrackNormal = FVector::UpVector;
}

void UHoverAssistComponent::InitializeAssist(float InHoverHeightCm, float InTraceLengthCm, float InOffTrackSec, float InMaxRollDeg, float InMaxPitchDeg)
{
	TargetHoverHeight = InHoverHeightCm;
	TraceLength = InTraceLengthCm;
	OffTrackTimeoutSec = InOffTrackSec;
	VisualRollTiltMaxDeg = InMaxRollDeg;
	VisualPitchTiltMaxDeg = InMaxPitchDeg;

	OffTrackTimer = 0.0f;
	CurrentAltitudeOffset = 0.0f;
	TakeoffElapsed = 0.0f;
	bInTakeoff = false;
}

void UHoverAssistComponent::StartTakeoff()
{
	bInTakeoff = true;
	TakeoffElapsed = 0.0f;
	CurrentAltitudeOffset = 0.0f;
}

void UHoverAssistComponent::UpdateHoverAssist(
	UPrimitiveComponent* PhysicsBody,
	USceneComponent* VisualMesh,
	const TArray<USceneComponent*>& Nozzles,
	float SteerInput,
	float AccelInput,
	float DeltaTime)
{
	if (!PhysicsBody || !GetWorld())
	{
		return;
	}

	// 1. Cast 4 short downward rays from nozzles
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	int32 HitCount = 0;
	float TotalDistance = 0.0f;
	FVector NormalSum = FVector::ZeroVector;

	const FVector DownVector = -PhysicsBody->GetUpVector();

	for (const USceneComponent* Nozzle : Nozzles)
	{
		if (!Nozzle)
		{
			continue;
		}

		const FVector Start = Nozzle->GetComponentLocation();
		const FVector End = Start + (DownVector * TraceLength);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
		{
			HitCount++;
			TotalDistance += Hit.Distance;
			NormalSum += Hit.ImpactNormal;
		}
	}

	// 2. Off-track Detection (no rays hit track)
	if (HitCount == 0)
	{
		bIsGrounded = false;
		GroundedRatio = 0.0f;
		OffTrackTimer += DeltaTime;

		if (OffTrackTimer >= OffTrackTimeoutSec)
		{
			OffTrackTimer = 0.0f;
			OnOffTrackDetected.Broadcast();
		}
		return;
	}

	// Track surface contact confirmed
	OffTrackTimer = 0.0f;
	bIsGrounded = true;
	GroundedRatio = static_cast<float>(HitCount) / static_cast<float>(FMath::Max(1, Nozzles.Num()));

	// 3. Compute Average Distance and Surface Normal
	const float AvgDistance = TotalDistance / HitCount;
	const FVector TargetNormal = (NormalSum / HitCount).GetSafeNormal();

	SmoothedTrackNormal = FMath::VInterpTo(SmoothedTrackNormal, TargetNormal, DeltaTime, 10.0f);

	// 4. Smooth Takeoff Progress (0.0 to 1.0)
	float HeightMultiplier = 1.0f;
	if (bInTakeoff)
	{
		TakeoffElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(TakeoffElapsed / TakeoffDuration, 0.0f, 1.0f);
		HeightMultiplier = FMath::InterpEaseOut(0.15f, 1.0f, Alpha, 2.0f);
		if (Alpha >= 1.0f)
		{
			bInTakeoff = false;
		}
	}

	const float DesiredHeight = TargetHoverHeight * HeightMultiplier;
	const float HeightDelta = DesiredHeight - AvgDistance;

	// Smoothly damp vertical position along the track normal (no bouncy physics springs)
	FVector CurrentLinearVelocity = PhysicsBody->GetPhysicsLinearVelocity();
	const float VerticalVel = FVector::DotProduct(CurrentLinearVelocity, SmoothedTrackNormal);
	const float TargetVerticalVel = FMath::Clamp(HeightDelta * 8.0f, -400.0f, 400.0f);
	const float CorrectionVel = FMath::FInterpTo(VerticalVel, TargetVerticalVel, DeltaTime, 12.0f);

	// Apply corrected velocity along the normal without altering planar movement
	CurrentLinearVelocity += SmoothedTrackNormal * (CorrectionVel - VerticalVel);
	PhysicsBody->SetPhysicsLinearVelocity(CurrentLinearVelocity);

	// 5. Track Alignment (rotate car Up vector toward track normal)
	const FVector CurrentUp = PhysicsBody->GetUpVector();
	const FVector AlignmentAxis = FVector::CrossProduct(CurrentUp, SmoothedTrackNormal);
	const float TiltSin = FMath::Clamp(AlignmentAxis.Size(), -1.0f, 1.0f);

	if (TiltSin > 0.001f)
	{
		const float AlignmentAngle = FMath::Asin(TiltSin);
		const FVector AngularTorque = AlignmentAxis.GetSafeNormal() * (AlignmentAngle * 25.0f);
		const FVector DampingTorque = -PhysicsBody->GetPhysicsAngularVelocityInRadians() * 4.0f;
		PhysicsBody->AddTorqueInRadians(AngularTorque + DampingTorque, NAME_None, true);
	}

	// 6. Visual-only Mesh Banking & Pitch (collision body stays level)
	if (VisualMesh)
	{
		const float TargetRoll = -SteerInput * VisualRollTiltMaxDeg;
		const float TargetPitch = -AccelInput * VisualPitchTiltMaxDeg;
		const FRotator TargetRot(TargetPitch, 0.0f, TargetRoll);

		CurrentVisualTilt = FMath::RInterpTo(CurrentVisualTilt, TargetRot, DeltaTime, 10.0f);
		VisualMesh->SetRelativeRotation(CurrentVisualTilt);
	}
}
