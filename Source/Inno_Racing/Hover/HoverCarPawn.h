// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Inno_Racing/Data/HoverCarData.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "HoverCarPawn.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class UHoverClutchComponent;
class UHoverFuelComponent;
class UHoverBoostComponent;
class UHoverAssistComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

// Movement sample sent by client to host and replicated to remote proxies
USTRUCT(BlueprintType)
struct FInnoMovementSample
{
	GENERATED_BODY()

	UPROPERTY()
	double ClientTimestamp = 0.0;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY()
	float SteerInput = 0.0f;

	UPROPERTY()
	float AccelInput = 0.0f;

	UPROPERTY()
	float BrakeInput = 0.0f;

	UPROPERTY()
	uint8 ClutchLevel = 1;

	UPROPERTY()
	bool bIsBoosting = false;

	UPROPERTY()
	bool bIsInLimpMode = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHoverSpeedChangedSignature, float, SpeedMs, float, SpeedKmh);

/**
 * Arcade Hover Car Pawn.
 * Gravity disabled; utilizes automatic 5-level clutch bands,
 * hover assist, fuel economy, boost bursts, and light drift physics.
 */
UCLASS()
class INNO_RACING_API AHoverCarPawn : public APawn
{
	GENERATED_BODY()

public:
	AHoverCarPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Root Physics Collision (Level) & Child Visual Mesh (Tilted) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChassisVisualMesh;

	// --- Thruster Nozzles ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover Nozzles")
	TObjectPtr<USceneComponent> HoverNozzle_FL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover Nozzles")
	TObjectPtr<USceneComponent> HoverNozzle_FR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover Nozzles")
	TObjectPtr<USceneComponent> HoverNozzle_RL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover Nozzles")
	TObjectPtr<USceneComponent> HoverNozzle_RR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Speed Nozzles")
	TObjectPtr<USceneComponent> SpeedNozzle_L;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Speed Nozzles")
	TObjectPtr<USceneComponent> SpeedNozzle_R;

	// --- Camera ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera")
	TObjectPtr<USpringArmComponent> CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera")
	TObjectPtr<UCameraComponent> Camera;

	// --- Subsystems / Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover")
	TObjectPtr<UHoverClutchComponent> ClutchComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover")
	TObjectPtr<UHoverFuelComponent> FuelComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover")
	TObjectPtr<UHoverBoostComponent> BoostComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hover")
	TObjectPtr<UHoverAssistComponent> AssistComp;

	// --- Data Asset Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverCar|Config")
	TObjectPtr<UHoverCarData> CarData;

	// --- Enhanced Input Bindings (Auto-bound in C++ or via Blueprint) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverCar|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverCar|Input")
	TObjectPtr<UInputAction> ActionAccelerate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverCar|Input")
	TObjectPtr<UInputAction> ActionBrake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverCar|Input")
	TObjectPtr<UInputAction> ActionSteer;

	// --- Input Actions (Only 3 controls: Accelerate, Brake, Steer) ---
	UFUNCTION(BlueprintCallable, Category = "HoverCar|Input")
	void Input_Accelerate(float AxisValue); // Hold RT / W

	UFUNCTION(BlueprintCallable, Category = "HoverCar|Input")
	void Input_Brake(float AxisValue); // Hold LT / S

	UFUNCTION(BlueprintCallable, Category = "HoverCar|Input")
	void Input_Steer(float AxisValue); // Left Stick / A-D

	// Engages hover takeoff sequence at GO
	UFUNCTION(BlueprintCallable, Category = "HoverCar|Match")
	void OnRaceStarted();

	// Updates the respawn checkpoint transform when passing a track gate
	UFUNCTION(BlueprintCallable, Category = "HoverCar|Match")
	void SetCheckpointTransform(const FTransform& NewCheckpointTransform);

	// Teleports and resets speed to a checkpoint transform
	UFUNCTION(BlueprintCallable, Category = "HoverCar|Match")
	void RespawnAtCheckpoint(const FTransform& CheckpointTransform);

	UFUNCTION(BlueprintPure, Category = "HoverCar|Telemetry")
	float GetCurrentForwardSpeedMs() const { return CurrentForwardSpeedMs; }

	UFUNCTION(BlueprintPure, Category = "HoverCar|Telemetry")
	float GetCurrentSpeedKmh() const { return CurrentForwardSpeedMs * 3.6f; }

	UPROPERTY(BlueprintAssignable, Category = "HoverCar|Telemetry")
	FOnHoverSpeedChangedSignature OnSpeedChanged;

	// --- Replicated Engine Display & Network State ---
	UPROPERTY(ReplicatedUsing = OnRep_VisualEngineState, VisibleAnywhere, BlueprintReadOnly, Category = "HoverCar|Net")
	uint8 ReplicatedClutchLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_VisualEngineState, VisibleAnywhere, BlueprintReadOnly, Category = "HoverCar|Net")
	bool bReplicatedBoosting = false;

	UPROPERTY(ReplicatedUsing = OnRep_VisualEngineState, VisibleAnywhere, BlueprintReadOnly, Category = "HoverCar|Net")
	bool bReplicatedLimpMode = false;

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedMovementSample)
	FInnoMovementSample ReplicatedSample;

	UFUNCTION()
	void OnRep_VisualEngineState();

	UFUNCTION()
	void OnRep_ReplicatedMovementSample();

protected:
	virtual void BeginPlay() override;

	// Unreliable client-to-host movement RPC
	UFUNCTION(Server, Unreliable)
	void Server_SendMovementSample(const FInnoMovementSample& Sample);

	// Core simulation loop for locally controlled pawn
	void SimulateHoverMovement(float DeltaTime);

	// Smooth network interpolation for non-locally controlled remote pawns
	void InterpolateRemoteMovement(float DeltaTime);

	// Syncs physics simulation mode depending on whether this pawn is locally controlled
	void UpdatePhysicsRoleState();

	// Checks GameState match phase so car hovers during Countdown and unlocks driving on Racing
	bool CanDriveInCurrentPhase() const;

	// Component Callbacks
	UFUNCTION()
	void HandleLimpModeToggled(bool bInLimp);

	UFUNCTION()
	void HandleOffTrack();

	UFUNCTION()
	void HandleMatchPhaseChanged(EMatchPhase NewPhase);

	// Enhanced Input Callback Handlers
	void HandleEnhancedAccelerate(const FInputActionValue& Value);
	void HandleEnhancedAccelerateCompleted(const FInputActionValue& Value);
	void HandleEnhancedBrake(const FInputActionValue& Value);
	void HandleEnhancedBrakeCompleted(const FInputActionValue& Value);
	void HandleEnhancedSteer(const FInputActionValue& Value);
	void HandleEnhancedSteerCompleted(const FInputActionValue& Value);

	// Current Inputs
	float AccelInput = 0.0f;
	float BrakeInput = 0.0f;
	float SteerInput = 0.0f;

	// Dynamic Physics State
	float CurrentForwardSpeedMs = 0.0f;
	float SmoothedTurnRateDegs = 0.0f;
	FVector TrackedVerticalVelocity = FVector::ZeroVector;

	// Checkpoint & Safe Ground Recovery State
	FTransform LastCheckpointTransform;
	FTransform LastSafeGroundedTransform;
	bool bHasExplicitCheckpoint = false;
	float SafeGroundRecordTimer = 0.0f;

	TArray<USceneComponent*> HoverNozzles;
	float NetSampleAccumulator = 0.0f;
	double LastAcceptedClientTimestamp = -1.0;
};
