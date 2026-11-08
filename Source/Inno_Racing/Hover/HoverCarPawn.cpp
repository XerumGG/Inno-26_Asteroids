// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Hover/HoverCarPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Inno_Racing/Hover/HoverClutchComponent.h"
#include "Inno_Racing/Hover/HoverFuelComponent.h"
#include "Inno_Racing/Hover/HoverBoostComponent.h"
#include "Inno_Racing/Hover/HoverAssistComponent.h"
#include "Inno_Racing/Race/InnoRaceGameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AHoverCarPawn::AHoverCarPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false); // Custom movement sample replication handles smooth hover sync

	// Auto possess player 0 in PIE or standalone placement
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// 1. Root Collision Box with Gravity Off
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;
	CollisionBox->SetBoxExtent(FVector(160.0f, 90.0f, 40.0f));
	CollisionBox->SetSimulatePhysics(true);
	CollisionBox->SetEnableGravity(false); // Gravity is OFF; HoverAssist manages height & airborne fall
	CollisionBox->SetCollisionProfileName(TEXT("Vehicle"));
	CollisionBox->SetGenerateOverlapEvents(true); // Required for pickups and checkpoint triggers
	CollisionBox->SetLinearDamping(0.05f);
	CollisionBox->SetAngularDamping(20.0f);

	// 2. Visual Mesh Root (tilted for roll/pitch bank while Collision stays aligned to track normal)
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	ChassisVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChassisVisualMesh"));
	ChassisVisualMesh->SetupAttachment(VisualRoot);
	ChassisVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 4 Hover Nozzle ray origins
	HoverNozzle_FL = CreateDefaultSubobject<USceneComponent>(TEXT("HoverNozzle_FL"));
	HoverNozzle_FL->SetupAttachment(RootComponent);
	HoverNozzle_FL->SetRelativeLocation(FVector(130.0f, -75.0f, -25.0f));

	HoverNozzle_FR = CreateDefaultSubobject<USceneComponent>(TEXT("HoverNozzle_FR"));
	HoverNozzle_FR->SetupAttachment(RootComponent);
	HoverNozzle_FR->SetRelativeLocation(FVector(130.0f, 75.0f, -25.0f));

	HoverNozzle_RL = CreateDefaultSubobject<USceneComponent>(TEXT("HoverNozzle_RL"));
	HoverNozzle_RL->SetupAttachment(RootComponent);
	HoverNozzle_RL->SetRelativeLocation(FVector(-130.0f, -75.0f, -25.0f));

	HoverNozzle_RR = CreateDefaultSubobject<USceneComponent>(TEXT("HoverNozzle_RR"));
	HoverNozzle_RR->SetupAttachment(RootComponent);
	HoverNozzle_RR->SetRelativeLocation(FVector(-130.0f, 75.0f, -25.0f));

	// 4. 2 Rear Speed Nozzles
	SpeedNozzle_L = CreateDefaultSubobject<USceneComponent>(TEXT("SpeedNozzle_L"));
	SpeedNozzle_L->SetupAttachment(VisualRoot);
	SpeedNozzle_L->SetRelativeLocation(FVector(-165.0f, -40.0f, 10.0f));

	SpeedNozzle_R = CreateDefaultSubobject<USceneComponent>(TEXT("SpeedNozzle_R"));
	SpeedNozzle_R->SetupAttachment(VisualRoot);
	SpeedNozzle_R->SetRelativeLocation(FVector(-165.0f, 40.0f, 10.0f));

	// 5. Chase Camera
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->TargetArmLength = 550.0f;
	CameraSpringArm->SocketOffset = FVector(0.0f, 0.0f, 120.0f);
	CameraSpringArm->bEnableCameraLag = true;
	CameraSpringArm->CameraLagSpeed = 14.0f;
	CameraSpringArm->bInheritPitch = true;
	CameraSpringArm->bInheritYaw = true;
	CameraSpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 6. Subsystem Components
	ClutchComp = CreateDefaultSubobject<UHoverClutchComponent>(TEXT("ClutchComp"));
	FuelComp = CreateDefaultSubobject<UHoverFuelComponent>(TEXT("FuelComp"));
	BoostComp = CreateDefaultSubobject<UHoverBoostComponent>(TEXT("BoostComp"));
	AssistComp = CreateDefaultSubobject<UHoverAssistComponent>(TEXT("AssistComp"));

	CurrentForwardSpeedMs = 0.0f;
	SmoothedTurnRateDegs = 0.0f;
	TrackedVerticalVelocity = FVector::ZeroVector;
	NetSampleAccumulator = 0.0f;
	LastAcceptedClientTimestamp = -1.0;
	bHasExplicitCheckpoint = false;
	SafeGroundRecordTimer = 0.0f;
}

void AHoverCarPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHoverCarPawn, ReplicatedClutchLevel);
	DOREPLIFETIME(AHoverCarPawn, bReplicatedBoosting);
	DOREPLIFETIME(AHoverCarPawn, bReplicatedLimpMode);
	DOREPLIFETIME(AHoverCarPawn, ReplicatedSample);
}

void AHoverCarPawn::BeginPlay()
{
	Super::BeginPlay();

	HoverNozzles.Empty();
	HoverNozzles.Add(HoverNozzle_FL);
	HoverNozzles.Add(HoverNozzle_FR);
	HoverNozzles.Add(HoverNozzle_RL);
	HoverNozzles.Add(HoverNozzle_RR);

	LastCheckpointTransform = GetActorTransform();
	LastSafeGroundedTransform = GetActorTransform();
	ReplicatedSample.Location = GetActorLocation();
	ReplicatedSample.Rotation = GetActorRotation();

	// Fallback CarData default if unassigned in Blueprint
	if (!CarData)
	{
		CarData = NewObject<UHoverCarData>(this, TEXT("DefaultCarDataFallback"));
	}

	if (CarData)
	{
		ClutchComp->InitializeClutch(CarData);
		FuelComp->InitializeFuel(CarData->FuelCapacity, CarData->LowFuelAlertThreshold);
		BoostComp->InitializeBoost(
			CarData->BoostDurationSec,
			CarData->BoostAccelMultiplier,
			CarData->BoostTopSpeedMultiplier,
			CarData->BoostTurnRateMultiplier,
			CarData->MaxBoostCharges
		);
		AssistComp->InitializeAssist(
			CarData->HoverHeightCm,
			CarData->RayTraceLengthCm,
			CarData->OffTrackRespawnSec,
			CarData->VisualRollTiltMaxDeg,
			CarData->VisualPitchTiltMaxDeg,
			CarData->HoverStiffnessGain,
			CarData->HoverDampingSpeed,
			CarData->AirborneGravityCms2,
			CarData->MaxFallSpeedCms
		);
	}

	FuelComp->OnLimpModeToggled.RemoveDynamic(this, &AHoverCarPawn::HandleLimpModeToggled);
	FuelComp->OnLimpModeToggled.AddDynamic(this, &AHoverCarPawn::HandleLimpModeToggled);

	AssistComp->OnOffTrackDetected.RemoveDynamic(this, &AHoverCarPawn::HandleOffTrack);
	AssistComp->OnOffTrackDetected.AddDynamic(this, &AHoverCarPawn::HandleOffTrack);

	UpdatePhysicsRoleState();

	// Subscribe to GameState phase transitions if in a formal race map
	if (UWorld* World = GetWorld())
	{
		if (AInnoRaceGameState* InnoGS = World->GetGameState<AInnoRaceGameState>())
		{
			InnoGS->OnMatchPhaseChanged.RemoveDynamic(this, &AHoverCarPawn::HandleMatchPhaseChanged);
			InnoGS->OnMatchPhaseChanged.AddDynamic(this, &AHoverCarPawn::HandleMatchPhaseChanged);
		}
	}

	// Auto-engage clutch and hover takeoff if allowed to drive immediately (e.g. standalone test map or already in Racing phase)
	if (CanDriveInCurrentPhase())
	{
		OnRaceStarted();
	}
	else if (AssistComp)
	{
		AssistComp->StartTakeoff();
	}
}

void AHoverCarPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UpdatePhysicsRoleState();
}

void AHoverCarPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	UpdatePhysicsRoleState();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AHoverCarPawn::UpdatePhysicsRoleState()
{
	if (!CollisionBox)
	{
		return;
	}

	if (IsLocallyControlled())
	{
		CollisionBox->SetSimulatePhysics(true);
		CollisionBox->SetEnableGravity(false);
		CollisionBox->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	}
	else
	{
		// Remote proxies are driven by network interpolation so local Chaos doesn't fight replicated transforms
		CollisionBox->SetSimulatePhysics(false);
	}
}

bool AHoverCarPawn::CanDriveInCurrentPhase() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AInnoRaceGameState* InnoGS = World->GetGameState<AInnoRaceGameState>())
		{
			return (InnoGS->MatchPhase == EMatchPhase::Racing || InnoGS->MatchPhase == EMatchPhase::Finishing);
		}
	}
	// If no AInnoRaceGameState is present (e.g., standalone test map), allow driving immediately
	return true;
}

void AHoverCarPawn::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
	if (NewPhase == EMatchPhase::Racing)
	{
		OnRaceStarted();
	}
	else if (NewPhase == EMatchPhase::RaceResults || NewPhase == EMatchPhase::MatchComplete || NewPhase == EMatchPhase::Detach)
	{
		if (ClutchComp)
		{
			ClutchComp->DisengageClutch();
		}
	}
}

void AHoverCarPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		SimulateHoverMovement(DeltaTime);

		// Synchronize state for remote visuals
		if (ClutchComp && BoostComp && FuelComp)
		{
			ReplicatedClutchLevel = static_cast<uint8>(ClutchComp->GetCurrentClutchLevel());
			bReplicatedBoosting = BoostComp->IsBoosting();
			bReplicatedLimpMode = FuelComp->IsInLimpMode();
		}

		// Send movement samples to host at 30 Hz
		NetSampleAccumulator += DeltaTime;
		const float SendInterval = 1.0f / 30.0f;
		if (NetSampleAccumulator >= SendInterval)
		{
			NetSampleAccumulator = 0.0f;

			FInnoMovementSample Sample;
			Sample.ClientTimestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			Sample.Location = GetActorLocation();
			Sample.Rotation = GetActorRotation();
			Sample.Velocity = CollisionBox ? CollisionBox->GetPhysicsLinearVelocity() : FVector::ZeroVector;
			Sample.SteerInput = SteerInput;
			Sample.AccelInput = AccelInput;
			Sample.BrakeInput = BrakeInput;
			Sample.ClutchLevel = ReplicatedClutchLevel;
			Sample.bIsBoosting = bReplicatedBoosting;
			Sample.bIsInLimpMode = bReplicatedLimpMode;

			if (HasAuthority())
			{
				ReplicatedSample = Sample;
			}
			else
			{
				Server_SendMovementSample(Sample);
			}
		}
	}
	else
	{
		InterpolateRemoteMovement(DeltaTime);
	}
}

void AHoverCarPawn::SimulateHoverMovement(float DeltaTime)
{
	if (!CollisionBox || !CarData || DeltaTime <= 0.0f)
	{
		return;
	}

	const bool bCanDrive = CanDriveInCurrentPhase();
	if (bCanDrive && ClutchComp && !ClutchComp->IsClutchEngaged())
	{
		ClutchComp->EngageClutch();
	}

	const float EffectiveAccel = bCanDrive ? AccelInput : 0.0f;
	const float EffectiveBrake = bCanDrive ? BrakeInput : 1.0f;
	const float EffectiveSteer = bCanDrive ? SteerInput : 0.0f;

	// 0. Reconcile CurrentForwardSpeedMs with actual physical velocity if a wall/barrier collision slowed us down
	const FVector CurrentPhysicsVel = CollisionBox->GetPhysicsLinearVelocity();
	const float PhysicsForwardSpeedMs = FVector::DotProduct(CurrentPhysicsVel, GetActorForwardVector()) / 100.0f;
	if (CurrentForwardSpeedMs > 3.0f && PhysicsForwardSpeedMs < CurrentForwardSpeedMs - 4.0f)
	{
		CurrentForwardSpeedMs = FMath::Max(0.0f, FMath::Lerp(CurrentForwardSpeedMs, PhysicsForwardSpeedMs, 0.65f));
	}

	// Preserve existing lateral and vertical velocity components prior to hover update
	const FVector PreStepUpNormal = AssistComp->IsGrounded() ? AssistComp->GetSmoothedTrackNormal() : FVector::UpVector;
	TrackedVerticalVelocity = PreStepUpNormal * FVector::DotProduct(CurrentPhysicsVel, PreStepUpNormal);
	const float PreStepLateralSpeedCms = FVector::DotProduct(CurrentPhysicsVel, GetActorRightVector());

	// 1. Update Boost
	BoostComp->UpdateBoost(DeltaTime);

	// 2. Update Clutch State Machine
	ClutchComp->UpdateAutoShift(CurrentForwardSpeedMs, BoostComp->IsBoosting(), DeltaTime);
	const FClutchBand& Band = ClutchComp->GetCurrentBand();

	// 3. Speed Target & Acceleration Calculation
	if (bCanDrive && BoostComp->IsBoosting())
	{
		// Boost overrides: Clutch 5 top speed * 1.35 (108 m/s), Accel * 2.0
		const float BoostTopSpeed = (CarData->ClutchBands.Num() > 0 ? CarData->ClutchBands.Last().TopSpeedMs : 80.0f) * CarData->BoostTopSpeedMultiplier;
		const float BoostAccel = (CarData->ClutchBands.Num() > 0 ? CarData->ClutchBands.Last().AccelerationMs2 : 7.0f) * CarData->BoostAccelMultiplier;

		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, BoostTopSpeed, DeltaTime, BoostAccel);
		// Burns no fuel while boosting
	}
	else if (EffectiveBrake > 0.05f)
	{
		// Active Braking: 40 m/s^2 decel down to 0 (no reverse at 0)
		const float BrakeDecel = CarData->BrakeDecelMs2 * EffectiveBrake;
		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, 0.0f, DeltaTime, BrakeDecel);
		CurrentForwardSpeedMs = FMath::Max(0.0f, CurrentForwardSpeedMs);
	}
	else if (EffectiveAccel > 0.05f && ClutchComp->IsClutchEngaged())
	{
		// Forward Acceleration (or coast-down if currently above Band.TopSpeedMs right after boost/downshift)
		const float TargetSpeed = Band.TopSpeedMs;
		const float AccelRate = (CurrentForwardSpeedMs > TargetSpeed)
			? FMath::Max(Band.AccelerationMs2 * EffectiveAccel, CarData->CoastDecelMs2)
			: (Band.AccelerationMs2 * EffectiveAccel);

		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, TargetSpeed, DeltaTime, AccelRate);

		// Fuel burns proportionally to throttle input while accelerating
		FuelComp->BurnFuel(Band.FuelBurnPerSec * EffectiveAccel, DeltaTime);
	}
	else
	{
		// Coasting Deceleration (8 m/s^2)
		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, 0.0f, DeltaTime, CarData->CoastDecelMs2);
		CurrentForwardSpeedMs = FMath::Max(0.0f, CurrentForwardSpeedMs);
	}

	// 4. Steering & Turn Agility
	float TargetTurnRate = Band.TurnRateDegs * EffectiveSteer;
	if (BoostComp->IsBoosting())
	{
		TargetTurnRate *= CarData->BoostTurnRateMultiplier; // 0.85x while boosting
	}
	SmoothedTurnRateDegs = FMath::FInterpTo(SmoothedTurnRateDegs, TargetTurnRate, DeltaTime, 5.0f);
	const float YawDeltaDeg = SmoothedTurnRateDegs * DeltaTime;

	// 5. Hover Assist (Raycasts, Quaternion Yaw + Track Normal Alignment, Visual Mesh Tilt, and Vertical Velocity)
	AssistComp->UpdateHoverAssist(
		CollisionBox,
		VisualRoot,
		HoverNozzles,
		YawDeltaDeg,
		EffectiveSteer,
		EffectiveAccel,
		EffectiveBrake,
		DeltaTime,
		TrackedVerticalVelocity
	);

	// Periodically snapshot a safe grounded transform when all 4 hover nozzles are stably over the track
	if (AssistComp->IsGrounded() && AssistComp->GetGroundedRatio() >= 0.75f)
	{
		SafeGroundRecordTimer += DeltaTime;
		if (SafeGroundRecordTimer >= 0.25f)
		{
			SafeGroundRecordTimer = 0.0f;
			LastSafeGroundedTransform = GetActorTransform();
		}
	}
	else
	{
		SafeGroundRecordTimer = 0.0f;
	}

	// 6. Apply Unified 3D Velocity (Forward along aligned track tangent + Dampened Lateral Drift + Hover/Gravity Vertical Velocity)
	const FVector ForwardVelocity = GetActorForwardVector() * (CurrentForwardSpeedMs * 100.0f); // m/s to cm/s
	const float DampenedLateralSpeedCms = FMath::FInterpTo(PreStepLateralSpeedCms, 0.0f, DeltaTime, CarData->LateralGripDecayRate);
	const FVector LateralVelocity = GetActorRightVector() * DampenedLateralSpeedCms;

	CollisionBox->SetPhysicsLinearVelocity(ForwardVelocity + LateralVelocity + TrackedVerticalVelocity);

	// 7. Telemetry Broadcast
	OnSpeedChanged.Broadcast(CurrentForwardSpeedMs, CurrentForwardSpeedMs * 3.6f);
}

void AHoverCarPawn::InterpolateRemoteMovement(float DeltaTime)
{
	if (ReplicatedSample.ClientTimestamp <= 0.0)
	{
		return;
	}

	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), ReplicatedSample.Location, DeltaTime, 15.0f);
	const FQuat NewRotation = FQuat::Slerp(GetActorQuat(), ReplicatedSample.Rotation.Quaternion(), FMath::Clamp(DeltaTime * 15.0f, 0.0f, 1.0f)).GetNormalized();
	SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

	CurrentForwardSpeedMs = FVector::DotProduct(ReplicatedSample.Velocity, GetActorForwardVector()) / 100.0f;

	if (AssistComp)
	{
		AssistComp->UpdateVisualTiltOnly(
			VisualRoot,
			ReplicatedSample.SteerInput,
			ReplicatedSample.AccelInput,
			ReplicatedSample.BrakeInput,
			DeltaTime
		);
	}
}

void AHoverCarPawn::OnRaceStarted()
{
	if (ClutchComp)
	{
		ClutchComp->EngageClutch();
	}
	if (AssistComp)
	{
		AssistComp->StartTakeoff();
	}
}

void AHoverCarPawn::SetCheckpointTransform(const FTransform& NewCheckpointTransform)
{
	LastCheckpointTransform = NewCheckpointTransform;
	bHasExplicitCheckpoint = true;
}

void AHoverCarPawn::RespawnAtCheckpoint(const FTransform& CheckpointTransform)
{
	FTransform SafeTransform = CheckpointTransform;
	// Ensure upright orientation on respawn
	FRotator UprightRot = SafeTransform.GetRotation().Rotator();
	UprightRot.Pitch = 0.0f;
	UprightRot.Roll = 0.0f;
	SafeTransform.SetRotation(UprightRot.Quaternion());

	SetActorTransform(SafeTransform, false, nullptr, ETeleportType::ResetPhysics);
	CurrentForwardSpeedMs = 0.0f;
	SmoothedTurnRateDegs = 0.0f;
	TrackedVerticalVelocity = FVector::ZeroVector;

	if (CollisionBox)
	{
		CollisionBox->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CollisionBox->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	}
	if (BoostComp)
	{
		BoostComp->StopBoost();
	}
	if (AssistComp)
	{
		AssistComp->ResetAssistState();
		AssistComp->StartTakeoff();
	}
	if (ClutchComp && CanDriveInCurrentPhase())
	{
		ClutchComp->EngageClutch();
	}
}

void AHoverCarPawn::HandleLimpModeToggled(bool bInLimp)
{
	if (ClutchComp)
	{
		ClutchComp->SetLimpMode(bInLimp);
	}
}

void AHoverCarPawn::HandleOffTrack()
{
	// Respawn at the last explicit checkpoint if available, otherwise at the last verified safe grounded position on the track
	const FTransform TargetRespawn = bHasExplicitCheckpoint ? LastCheckpointTransform : LastSafeGroundedTransform;
	RespawnAtCheckpoint(TargetRespawn);
}

void AHoverCarPawn::Input_Accelerate(float AxisValue)
{
	AccelInput = FMath::Clamp(AxisValue, 0.0f, 1.0f);
}

void AHoverCarPawn::Input_Brake(float AxisValue)
{
	BrakeInput = FMath::Clamp(AxisValue, 0.0f, 1.0f);
}

void AHoverCarPawn::Input_Steer(float AxisValue)
{
	SteerInput = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AHoverCarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ActionAccelerate)
		{
			EnhancedInputComponent->BindAction(ActionAccelerate, ETriggerEvent::Triggered, this, &AHoverCarPawn::HandleEnhancedAccelerate);
			EnhancedInputComponent->BindAction(ActionAccelerate, ETriggerEvent::Completed, this, &AHoverCarPawn::HandleEnhancedAccelerateCompleted);
		}
		if (ActionBrake)
		{
			EnhancedInputComponent->BindAction(ActionBrake, ETriggerEvent::Triggered, this, &AHoverCarPawn::HandleEnhancedBrake);
			EnhancedInputComponent->BindAction(ActionBrake, ETriggerEvent::Completed, this, &AHoverCarPawn::HandleEnhancedBrakeCompleted);
		}
		if (ActionSteer)
		{
			EnhancedInputComponent->BindAction(ActionSteer, ETriggerEvent::Triggered, this, &AHoverCarPawn::HandleEnhancedSteer);
			EnhancedInputComponent->BindAction(ActionSteer, ETriggerEvent::Completed, this, &AHoverCarPawn::HandleEnhancedSteerCompleted);
		}
	}
}

void AHoverCarPawn::HandleEnhancedAccelerate(const FInputActionValue& Value)
{
	Input_Accelerate(Value.Get<float>());
}

void AHoverCarPawn::HandleEnhancedAccelerateCompleted(const FInputActionValue& Value)
{
	Input_Accelerate(0.0f);
}

void AHoverCarPawn::HandleEnhancedBrake(const FInputActionValue& Value)
{
	Input_Brake(Value.Get<float>());
}

void AHoverCarPawn::HandleEnhancedBrakeCompleted(const FInputActionValue& Value)
{
	Input_Brake(0.0f);
}

void AHoverCarPawn::HandleEnhancedSteer(const FInputActionValue& Value)
{
	Input_Steer(Value.Get<float>());
}

void AHoverCarPawn::HandleEnhancedSteerCompleted(const FInputActionValue& Value)
{
	Input_Steer(0.0f);
}

void AHoverCarPawn::Server_SendMovementSample_Implementation(const FInnoMovementSample& Sample)
{
	// 1. Validate monotonic client timestamp
	if (Sample.ClientTimestamp <= LastAcceptedClientTimestamp)
	{
		return;
	}

	// 2. Validate maximum sanity speed (~124 m/s = 12400 cm/s, allowing slight margin for vertical velocity on ramps)
	constexpr float MaxSanitySpeedSqCms = 13500.0f * 13500.0f;
	if (Sample.Velocity.SizeSquared() > MaxSanitySpeedSqCms)
	{
		return;
	}

	LastAcceptedClientTimestamp = Sample.ClientTimestamp;
	ReplicatedSample = Sample;
	ReplicatedClutchLevel = Sample.ClutchLevel;
	bReplicatedBoosting = Sample.bIsBoosting;
	bReplicatedLimpMode = Sample.bIsInLimpMode;

	if (!IsLocallyControlled())
	{
		SetActorLocationAndRotation(Sample.Location, Sample.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AHoverCarPawn::OnRep_VisualEngineState()
{
	if (ClutchComp && !IsLocallyControlled())
	{
		ClutchComp->SetLimpMode(bReplicatedLimpMode);
		ClutchComp->SetClutchLevelDirect(ReplicatedClutchLevel);
	}
}

void AHoverCarPawn::OnRep_ReplicatedMovementSample()
{
	// Handled smoothly in InterpolateRemoteMovement during Tick
}
