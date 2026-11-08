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
	TraceLength = 600.0f;
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
	TraceLength = FMath::Max(InTraceLengthCm, 600.0f);
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

	// Always clear physics angular velocity so physical collisions never spin or tumble the car
	PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	int32 HitCount = 0;
	float TotalDistance = 0.0f;
	FVector NormalSum = FVector::ZeroVector;

	const FVector DownVector = -PhysicsBody->GetUpVector();

	// Raycast from 4 corner nozzles
	for (const USceneComponent* Nozzle : Nozzles)
	{
		if (!Nozzle)
		{
			continue;
		}

		const FVector Start = Nozzle->GetComponentLocation();
		const FVector End = Start + (DownVector * TraceLength);

		FHitResult Hit;
		bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, QueryParams);
		if (!bHit)
		{
			bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
		}

		if (bHit)
		{
			HitCount++;
			TotalDistance += Hit.Distance;
			NormalSum += Hit.ImpactNormal;
		}
	}

	// Fallback central raycast if nozzles array was empty or missed
	if (HitCount == 0)
	{
		const FVector CenterStart = PhysicsBody->GetComponentLocation();
		const FVector CenterEnd = CenterStart + (DownVector * TraceLength);

		FHitResult Hit;
		bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, CenterStart, CenterEnd, ObjectParams, QueryParams);
		if (!bHit)
		{
			bHit = GetWorld()->LineTraceSingleByChannel(Hit, CenterStart, CenterEnd, ECC_Visibility, QueryParams);
		}

		if (bHit)
		{
			HitCount = 1;
			TotalDistance = Hit.Distance;
			NormalSum = Hit.ImpactNormal;
		}
	}

	// Airborne / Off-track State
	if (HitCount == 0)
	{
		bIsGrounded = false;
		GroundedRatio = 0.0f;
		OffTrackTimer += DeltaTime;

		// Gentle artificial hover gravity downwards in world space
		FVector CurrentVel = PhysicsBody->GetPhysicsLinearVelocity();
		CurrentVel.Z = FMath::FInterpTo(CurrentVel.Z, -600.0f, DeltaTime, 2.0f);
		PhysicsBody->SetPhysicsLinearVelocity(CurrentVel);

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

	const float AvgDistance = TotalDistance / HitCount;
	const FVector TargetNormal = (NormalSum / HitCount).GetSafeNormal();
	SmoothedTrackNormal = FMath::VInterpTo(SmoothedTrackNormal, TargetNormal, DeltaTime, 10.0f);

	float HeightMultiplier = 1.0f;
	if (bInTakeoff)
	{
		TakeoffElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(TakeoffElapsed / TakeoffDuration, 0.0f, 1.0f);
		HeightMultiplier = FMath::InterpEaseOut(0.2f, 1.0f, Alpha, 2.0f);
		if (Alpha >= 1.0f)
		{
			bInTakeoff = false;
		}
	}

	const float DesiredHeight = TargetHoverHeight * HeightMultiplier;
	const float HeightDelta = DesiredHeight - AvgDistance;

	// Damped vertical velocity along track normal
	FVector CurrentLinearVelocity = PhysicsBody->GetPhysicsLinearVelocity();
	const float VerticalVel = FVector::DotProduct(CurrentLinearVelocity, SmoothedTrackNormal);
	const float TargetVerticalVel = FMath::Clamp(HeightDelta * 8.0f, -400.0f, 400.0f);
	const float CorrectionVel = FMath::FInterpTo(VerticalVel, TargetVerticalVel, DeltaTime, 14.0f);

	CurrentLinearVelocity += SmoothedTrackNormal * (CorrectionVel - VerticalVel);
	PhysicsBody->SetPhysicsLinearVelocity(CurrentLinearVelocity);

	// Smoothly align vehicle Pitch and Roll with track normal (kinematic, non-chaotic)
	if (AActor* OwnerActor = GetOwner())
	{
		const FRotator ActorRot = OwnerActor->GetActorRotation();
		const FVector Forward = OwnerActor->GetActorForwardVector();
		FVector ProjectedForward = FVector::VectorPlaneProject(Forward, SmoothedTrackNormal).GetSafeNormal();
		if (ProjectedForward.IsNearlyZero())
		{
			ProjectedForward = Forward;
		}

		const FRotator TargetRot = FRotationMatrix::MakeFromZX(SmoothedTrackNormal, ProjectedForward).Rotator();
		const FRotator NewRot(
			FMath::FInterpTo(ActorRot.Pitch, TargetRot.Pitch, DeltaTime, 10.0f),
			ActorRot.Yaw, // Steer controls Yaw
			FMath::FInterpTo(ActorRot.Roll, TargetRot.Roll, DeltaTime, 10.0f)
		);
		OwnerActor->SetActorRotation(NewRot);
	}

	// Visual-only Mesh Banking & Pitch (collision body stays level)
	if (VisualMesh)
	{
		const float TargetRoll = -SteerInput * VisualRollTiltMaxDeg;
		const float TargetPitch = -AccelInput * VisualPitchTiltMaxDeg;
		const FRotator TargetRot(TargetPitch, 0.0f, TargetRoll);

		CurrentVisualTilt = FMath::RInterpTo(CurrentVisualTilt, TargetRot, DeltaTime, 10.0f);
		VisualMesh->SetRelativeRotation(CurrentVisualTilt);
	}
}
