// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoUIFlowSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScreenNavigatedSignature, EInnoScreen, NewScreen);

/**
 * GameInstance Subsystem managing UI navigation stack across seamless travel.
 * Maps GameState match phases directly to the appropriate screen widgets.
 */
UCLASS()
class INNO_RACING_API UInnoUIFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Navigates to a target screen
	UFUNCTION(BlueprintCallable, Category = "Inno|UIFlow")
	void PushScreen(EInnoScreen Screen);

	// Pops top screen from the navigation stack
	UFUNCTION(BlueprintCallable, Category = "Inno|UIFlow")
	void PopScreen();

	// Resets stack back to Main Menu
	UFUNCTION(BlueprintCallable, Category = "Inno|UIFlow")
	void ResetToMainMenu();

	// Maps replicated match phases into corresponding screen requests
	UFUNCTION(BlueprintCallable, Category = "Inno|UIFlow")
	void HandleMatchPhaseChanged(EMatchPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Inno|UIFlow")
	EInnoScreen GetCurrentScreen() const;

	UPROPERTY(BlueprintAssignable, Category = "Inno|UIFlow")
	FOnScreenNavigatedSignature OnScreenNavigated;

protected:
	UPROPERTY()
	TArray<EInnoScreen> ScreenStack;
};
