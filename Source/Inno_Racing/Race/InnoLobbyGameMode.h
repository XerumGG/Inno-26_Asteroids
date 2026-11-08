// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "InnoLobbyGameMode.generated.h"

/**
 * Host-only GameMode for L_Lobby (Match Lobby / VS Screen).
 * Validates joins in PreLogin, auto-suffixes duplicate participant names,
 * tracks player readiness, and initiates seamless travel to Track 1.
 */
UCLASS()
class INNO_RACING_API AInnoLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AInnoLobbyGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// Checks readiness of all connected players
	void CheckAllPlayersReady();

	// Locks lobby and begins seamless travel to the first race track
	UFUNCTION(BlueprintCallable, Category = "Inno|Lobby")
	void StartRaceTravel(const FString& FirstTrackMapPath);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Lobby")
	int32 MinPlayersToStart = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Lobby")
	int32 MaxPlayers = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Lobby")
	FString ExpectedBuildId = TEXT("1.0.0");

protected:
	bool bSessionLocked = false;
};
