// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Hover/HoverAssistComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"

UHoverAssistComponent::UHoverAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TargetHoverHeight = 120.0f;
	TraceLength = 500.0f;
	OffTrackTimeoutSec = 1.5f;
	OffTrackTimer = 0.0f;
	bIsGrounded = false;
	GroundedRatio = 0.0f;
	HoverStiffnessGain = 8.0f;
	HoverDampingSpeed = 14.0f;
	AirborneGravityCms2 = 980.0f;
	MaxFallSpeedCms = 1200.0f;
	VisualRollTiltMaxDeg = 22.0f;
	VisualPitchTiltMaxDeg = 8.0f;
	CurrentAltitudeOffset = 0.0f;
	TakeoffDuration = 0.4f;
	TakeoffElapsed = 0.0f;
	bInTakeoff = false;
	CurrentVisualTilt = FRotator::ZeroRotator;
	SmoothedTrackNormal = FVector::UpVector;
}

void UHoverAssistComponent::InitializeAssist(
	float InHoverHeightCm,
	float InTraceLengthCm,
	float InOffTrackSec,
	float InMaxRollDeg,
	float InMaxPitchDeg,
	float InStiffnessGain,
	float InDampingSpeed,
	float InAirborneGravityCms2,
	float InMaxFallSpeedCms)
{
	TargetHoverHeight = FMath::Max(30.0f, InHoverHeightCm);
	TraceLength = FMath::Max(InTraceLengthCm, TargetHoverHeight * 2.5f);
	OffTrackTimeoutSec = FMath::Max(0.25f, InOffTrackSec);
	VisualRollTiltMaxDeg = InMaxRollDeg;
	VisualPitchTiltMaxDeg = InMaxPitchDeg;
	HoverStiffnessGain = FMath::Max(1.0f, InStiffnessGain);
	HoverDampingSpeed = FMath::Max(1.0f, InDampingSpeed);
	AirborneGravityCms2 = FMath::Max(100.0f, InAirborneGravityCms2);
	MaxFallSpeedCms = FMath::Max(200.0f, InMaxFallSpeedCms);

	ResetAssistState();
}

void UHoverAssistComponent::StartTakeoff()
{
	bInTakeoff = true;
	TakeoffElapsed = 0.0f;
	CurrentAltitudeOffset = 0.0f;
}

void UHoverAssistComponent::ResetAssistState()
{
	OffTrackTimer = 0.0f;
	CurrentAltitudeOffset = 0.0f;
	TakeoffElapsed = 0.0f;
	bInTakeoff = false;
	CurrentVisualTilt = FRotator::ZeroRotator;
	if (AActor* OwnerActor = GetOwner())
	{
		SmoothedTrackNormal = OwnerActor->GetActorUpVector();
	}
	else
	{
		SmoothedTrackNormal = FVector::UpVector;
	}
}

void UHoverAssistComponent::UpdateVisualTiltOnly(
	USceneComponent* VisualMesh,
	float SteerInput,
	float AccelInput,
	float BrakeInput,
	float DeltaTime)
{
	if (!VisualMesh)
	{
		return;
	}

	// Positive Roll in UE tilts right wing down (banks into a right turn when SteerInput > 0)
	// Positive Pitch in UE lifts nose up on acceleration and dips nose down on braking
	const float TargetRoll = SteerInput * VisualRollTiltMaxDeg;
	const float TargetPitch = (AccelInput - BrakeInput) * VisualPitchTiltMaxDeg;
	const FRotator TargetRot(TargetPitch, 0.0f, TargetRoll);

	CurrentVisualTilt = FMath::RInterpTo(CurrentVisualTilt, TargetRot, DeltaTime, 10.0f);
	VisualMesh->SetRelativeRotation(CurrentVisualTilt);
}

void UHoverAssistComponent::UpdateHoverAssist(
	UPrimitiveComponent* PhysicsBody,
	USceneComponent* VisualMesh,
	const TArray<USceneComponent*>& Nozzles,
	float YawDeltaDeg,
	float SteerInput,
	float AccelInput,
	float BrakeInput,
	float DeltaTime,
	FVector& InOutVerticalVelocity)
{
	if (!PhysicsBody || !GetWorld())
	{
		return;
	}

	// Always clear physics angular velocity so physical collisions never spin or tumble the car
	PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HoverAssistTrace), false, GetOwner());
	QueryParams.bReturnPhysicalMaterial = false;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	int32 HitCount = 0;
	float TotalDistance = 0.0f;
	FVector NormalSum = FVector::ZeroVector;

	const FVector UpVector = PhysicsBody->GetUpVector();
	const FVector DownVector = -UpVector;

	// Start raycast slightly above each nozzle so bottoming out on the track never starts the trace below the floor
	constexpr float TraceStartUpOffsetCm = 50.0f;
	constexpr float DefaultNozzleLocalZ = -25.0f;

	// Raycast from 4 corner nozzles
	for (const USceneComponent* Nozzle : Nozzles)
	{
		if (!Nozzle)
		{
			continue;
		}

		const FVector NozzleLoc = Nozzle->GetComponentLocation();
		const FVector Start = NozzleLoc + (UpVector * TraceStartUpOffsetCm);
		const FVector End = NozzleLoc + (DownVector * TraceLength);

		FHitResult Hit;
		bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, QueryParams);
		if (!bHit)
		{
			bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
		}

		if (bHit)
		{
			HitCount++;
			const float DistanceFromNozzle = FMath::Max(0.0f, Hit.Distance - TraceStartUpOffsetCm);
			TotalDistance += DistanceFromNozzle;
			NormalSum += Hit.ImpactNormal;
		}
	}

	// Fallback central raycast if all corner nozzles missed (aligned to the same local Z plane as nozzles)
	if (HitCount == 0)
	{
		const FVector CenterNozzlePlaneLoc = PhysicsBody->GetComponentLocation() + (UpVector * DefaultNozzleLocalZ);
		const FVector CenterStart = CenterNozzlePlaneLoc + (UpVector * TraceStartUpOffsetCm);
		const FVector CenterEnd = CenterNozzlePlaneLoc + (DownVector * TraceLength);

		FHitResult Hit;
		bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, CenterStart, CenterEnd, ObjectParams, QueryParams);
		if (!bHit)
		{
			bHit = GetWorld()->LineTraceSingleByChannel(Hit, CenterStart, CenterEnd, ECC_Visibility, QueryParams);
		}

		if (bHit)
		{
			HitCount = 1;
			TotalDistance = FMath::Max(0.0f, Hit.Distance - TraceStartUpOffsetCm);
			NormalSum = Hit.ImpactNormal;
		}
	}

	// Update visual mesh tilt in both grounded and airborne states
	UpdateVisualTiltOnly(VisualMesh, SteerInput, AccelInput, BrakeInput, DeltaTime);

	// Airborne / Off-track State
	if (HitCount == 0)
	{
		bIsGrounded = false;
		GroundedRatio = 0.0f;
		OffTrackTimer += DeltaTime;

		// Smoothly return track normal toward world up while airborne so the car levels out over jumps
		SmoothedTrackNormal = FMath::VInterpNormalRotationTo(SmoothedTrackNormal, FVector::UpVector, DeltaTime, 45.0f);

		if (AActor* OwnerActor = GetOwner())
		{
			FQuat CurrentQuat = OwnerActor->GetActorQuat();
			if (!FMath::IsNearlyZero(YawDeltaDeg))
			{
				const FQuat YawQuat(CurrentQuat.GetUpVector(), FMath::DegreesToRadians(YawDeltaDeg));
				CurrentQuat = (YawQuat * CurrentQuat).GetNormalized();
			}
			const FQuat AlignQuat = FQuat::FindBetweenNormals(CurrentQuat.GetUpVector(), SmoothedTrackNormal);
			const FQuat TargetQuat = (AlignQuat * CurrentQuat).GetNormalized();
			const FQuat NewQuat = FQuat::Slerp(CurrentQuat, TargetQuat, FMath::Clamp(DeltaTime * 3.0f, 0.0f, 1.0f)).GetNormalized();
			OwnerActor->SetActorRotation(NewQuat);
		}

		// Accumulate downward gravity in world Z without being wiped out between frames
		const float NewFallZ = FMath::Max(-MaxFallSpeedCms, InOutVerticalVelocity.Z - (AirborneGravityCms2 * DeltaTime));
		InOutVerticalVelocity = FVector(0.0f, 0.0f, NewFallZ);

		if (OffTrackTimer >= OffTrackTimeoutSec)
		{
			OffTrackTimer = 0.0f;
			OnOffTrackDetected.Broadcast();
		}
		return;
	}

	// Grounded State
	OffTrackTimer = 0.0f;
	bIsGrounded = true;
	GroundedRatio = static_cast<float>(HitCount) / static_cast<float>(FMath::Max(1, Nozzles.Num()));

	const float AvgDistance = TotalDistance / static_cast<float>(HitCount);
	const FVector TargetNormal = (NormalSum / static_cast<float>(HitCount)).GetSafeNormal();
	SmoothedTrackNormal = FMath::VInterpTo(SmoothedTrackNormal, TargetNormal, DeltaTime, 10.0f).GetSafeNormal();
	if (SmoothedTrackNormal.IsNearlyZero())
	{
		SmoothedTrackNormal = FVector::UpVector;
	}

	// Quaternion-based steering yaw + track surface normal alignment (free of Euler gimbal lock)
	if (AActor* OwnerActor = GetOwner())
	{
		FQuat CurrentQuat = OwnerActor->GetActorQuat();
		if (!FMath::IsNearlyZero(YawDeltaDeg))
		{
			const FQuat YawQuat(CurrentQuat.GetUpVector(), FMath::DegreesToRadians(YawDeltaDeg));
			CurrentQuat = (YawQuat * CurrentQuat).GetNormalized();
		}

		const FQuat AlignQuat = FQuat::FindBetweenNormals(CurrentQuat.GetUpVector(), SmoothedTrackNormal);
		const FQuat TargetQuat = (AlignQuat * CurrentQuat).GetNormalized();
		const FQuat NewQuat = FQuat::Slerp(CurrentQuat, TargetQuat, FMath::Clamp(DeltaTime * 12.0f, 0.0f, 1.0f)).GetNormalized();
		OwnerActor->SetActorRotation(NewQuat);
	}

	float HeightMultiplier = 1.0f;
	if (bInTakeoff)
	{
		TakeoffElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(TakeoffElapsed / FMath::Max(0.01f, TakeoffDuration), 0.0f, 1.0f);
		HeightMultiplier = FMath::InterpEaseOut(0.2f, 1.0f, Alpha, 2.0f);
		if (Alpha >= 1.0f)
		{
			bInTakeoff = false;
		}
	}

	const float DesiredHeight = TargetHoverHeight * HeightMultiplier;
	const float HeightDelta = DesiredHeight - AvgDistance;

	// Damped vertical velocity along track normal
	const float CurrentNormalVel = FVector::DotProduct(InOutVerticalVelocity, SmoothedTrackNormal);
	const float TargetNormalVel = FMath::Clamp(HeightDelta * HoverStiffnessGain, -500.0f, 500.0f);
	const float CorrectedNormalVel = FMath::FInterpTo(CurrentNormalVel, TargetNormalVel, DeltaTime, HoverDampingSpeed);

	InOutVerticalVelocity = SmoothedTrackNormal * CorrectedNormalVel;
}
