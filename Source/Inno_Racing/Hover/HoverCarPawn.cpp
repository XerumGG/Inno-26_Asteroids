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
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AHoverCarPawn::AHoverCarPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Auto possess player 0 in PIE or standalone placement
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// 1. Root Collision Box with Gravity Off
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;
	CollisionBox->SetBoxExtent(FVector(160.0f, 90.0f, 40.0f));
	CollisionBox->SetSimulatePhysics(true);
	CollisionBox->SetEnableGravity(false); // Gravity is OFF!
	CollisionBox->SetCollisionProfileName(TEXT("Vehicle"));
	CollisionBox->SetLinearDamping(0.5f);
	CollisionBox->SetAngularDamping(10.0f);

	// Lock rotation in physics constraints so physical hits never spin or flip the car
	CollisionBox->BodyInstance.bLockXRotation = true;
	CollisionBox->BodyInstance.bLockYRotation = true;
	CollisionBox->BodyInstance.bLockZRotation = true;

	// 2. Visual Mesh Root (tilted for roll/pitch bank while Collision stays level)
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
	NetSampleAccumulator = 0.0f;
}

void AHoverCarPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHoverCarPawn, ReplicatedClutchLevel);
	DOREPLIFETIME(AHoverCarPawn, bReplicatedBoosting);
	DOREPLIFETIME(AHoverCarPawn, bReplicatedLimpMode);
}

void AHoverCarPawn::BeginPlay()
{
	Super::BeginPlay();

	HoverNozzles.Empty();
	HoverNozzles.Add(HoverNozzle_FL);
	HoverNozzles.Add(HoverNozzle_FR);
	HoverNozzles.Add(HoverNozzle_RL);
	HoverNozzles.Add(HoverNozzle_RR);

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
			CarData->BoostTurnRateMultiplier
		);
		AssistComp->InitializeAssist(
			CarData->HoverHeightCm,
			CarData->RayTraceLengthCm,
			CarData->OffTrackRespawnSec,
			CarData->VisualRollTiltMaxDeg,
			CarData->VisualPitchTiltMaxDeg
		);
	}

	FuelComp->OnLimpModeToggled.AddDynamic(this, &AHoverCarPawn::HandleLimpModeToggled);
	AssistComp->OnOffTrackDetected.AddDynamic(this, &AHoverCarPawn::HandleOffTrack);

	// Zero out any initial angular velocity
	if (CollisionBox)
	{
		CollisionBox->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	}

	// Auto-engage clutch and hover takeoff for instant drivability
	if (ClutchComp)
	{
		ClutchComp->EngageClutch();
	}
	if (AssistComp)
	{
		AssistComp->StartTakeoff();
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

		// Send movement samples to host at 30 Hz (60 Hz near finish)
		NetSampleAccumulator += DeltaTime;
		const float SendInterval = 1.0f / 30.0f;
		if (NetSampleAccumulator >= SendInterval)
		{
			NetSampleAccumulator = 0.0f;

			FInnoMovementSample Sample;
			Sample.ClientTimestamp = GetWorld()->GetTimeSeconds();
			Sample.Location = GetActorLocation();
			Sample.Rotation = GetActorRotation();
			Sample.Velocity = CollisionBox->GetPhysicsLinearVelocity();
			Sample.ClutchLevel = ReplicatedClutchLevel;
			Sample.bIsBoosting = bReplicatedBoosting;
			Sample.bIsInLimpMode = bReplicatedLimpMode;

			Server_SendMovementSample(Sample);
		}
	}
}

void AHoverCarPawn::SimulateHoverMovement(float DeltaTime)
{
	if (!CollisionBox || !CarData)
	{
		return;
	}

	// 1. Update Boost
	BoostComp->UpdateBoost(DeltaTime);

	// 2. Update Clutch State Machine
	ClutchComp->UpdateAutoShift(CurrentForwardSpeedMs, BoostComp->IsBoosting(), DeltaTime);
	const FClutchBand& Band = ClutchComp->GetCurrentBand();

	// 3. Speed Target & Acceleration Calculation
	if (BoostComp->IsBoosting())
	{
		// Boost overrides: Clutch 5 top speed * 1.35 (108 m/s), Accel * 2.0
		const float BoostTopSpeed = (CarData->ClutchBands.Num() > 0 ? CarData->ClutchBands.Last().TopSpeedMs : 80.0f) * CarData->BoostTopSpeedMultiplier;
		const float BoostAccel = (CarData->ClutchBands.Num() > 0 ? CarData->ClutchBands.Last().AccelerationMs2 : 7.0f) * CarData->BoostAccelMultiplier;

		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, BoostTopSpeed, DeltaTime, BoostAccel);
		// Burns no fuel while boosting!
	}
	else if (BrakeInput > 0.05f)
	{
		// Active Braking: 40 m/s^2 decel down to 0 (no reverse at 0!)
		const float BrakeDecel = CarData->BrakeDecelMs2 * BrakeInput;
		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, 0.0f, DeltaTime, BrakeDecel);
		CurrentForwardSpeedMs = FMath::Max(0.0f, CurrentForwardSpeedMs);
		// Coasting & braking burn no fuel!
	}
	else if (AccelInput > 0.05f && ClutchComp->IsClutchEngaged())
	{
		// Forward Acceleration
		const float TargetSpeed = Band.TopSpeedMs;
		const float AccelRate = Band.AccelerationMs2 * AccelInput;
		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, TargetSpeed, DeltaTime, AccelRate);

		// Fuel burns only while holding accelerate
		FuelComp->BurnFuel(Band.FuelBurnPerSec, DeltaTime);
	}
	else
	{
		// Coasting Deceleration (8 m/s^2)
		CurrentForwardSpeedMs = FMath::FInterpConstantTo(CurrentForwardSpeedMs, 0.0f, DeltaTime, CarData->CoastDecelMs2);
		CurrentForwardSpeedMs = FMath::Max(0.0f, CurrentForwardSpeedMs);
	}

	// 4. Steering & Turn Agility
	float TargetTurnRate = Band.TurnRateDegs * SteerInput;
	if (BoostComp->IsBoosting())
	{
		TargetTurnRate *= CarData->BoostTurnRateMultiplier; // 0.85x while boosting
	}
	SmoothedTurnRateDegs = FMath::FInterpTo(SmoothedTurnRateDegs, TargetTurnRate, DeltaTime, 5.0f);
	AddActorLocalRotation(FRotator(0.0f, SmoothedTurnRateDegs * DeltaTime, 0.0f));

	// 5. Update Velocity with Light Lateral Drift
	const FVector ForwardVelocity = GetActorForwardVector() * (CurrentForwardSpeedMs * 100.0f); // m/s to cm/s
	const FVector CurrentVel = CollisionBox->GetPhysicsLinearVelocity();
	const FVector RightVector = GetActorRightVector();
	const float LateralSpeed = FVector::DotProduct(CurrentVel, RightVector);

	// Sideways slip decays quickly (grip ~6 /s)
	const float DampenedLateralSpeed = FMath::FInterpTo(LateralSpeed, 0.0f, DeltaTime, CarData->LateralGripDecayRate);
	const FVector LateralVelocity = RightVector * DampenedLateralSpeed;

	CollisionBox->SetPhysicsLinearVelocity(ForwardVelocity + LateralVelocity);

	// 6. Hover Assist (Smooth height hold, track alignment, visual mesh tilt)
	AssistComp->UpdateHoverAssist(CollisionBox, VisualRoot, HoverNozzles, SteerInput, AccelInput, DeltaTime);

	// 7. Telemetry Broadcast
	OnSpeedChanged.Broadcast(CurrentForwardSpeedMs, CurrentForwardSpeedMs * 3.6f);
}

void AHoverCarPawn::OnRaceStarted()
{
	ClutchComp->EngageClutch();
	AssistComp->StartTakeoff();
}

void AHoverCarPawn::RespawnAtCheckpoint(const FTransform& CheckpointTransform)
{
	SetActorTransform(CheckpointTransform, false, nullptr, ETeleportType::ResetPhysics);
	CurrentForwardSpeedMs = 0.0f;
	if (CollisionBox)
	{
		CollisionBox->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CollisionBox->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	}
	if (ClutchComp)
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
	// Trigger respawn at last valid checkpoint
	RespawnAtCheckpoint(GetActorTransform());
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

void AHoverCarPawn::PawnClientRestart()
{
	Super::PawnClientRestart();

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
	// Server/Host authority validation
	// Host will validate timestamp, checkpoint order, and maximum sanity speed (~124 m/s)
}

void AHoverCarPawn::OnRep_VisualEngineState()
{
	// Remote car updates: adjust engine glow / exhaust flame scale according to clutch level
}
