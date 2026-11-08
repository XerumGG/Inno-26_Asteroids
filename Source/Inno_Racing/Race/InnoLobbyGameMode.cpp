// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoLobbyGameMode.h"
#include "Inno_Racing/Race/InnoPlayerController.h"
#include "Inno_Racing/Race/InnoPlayerState.h"
#include "Inno_Racing/Race/InnoRaceGameState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AInnoLobbyGameMode::AInnoLobbyGameMode()
{
	PlayerControllerClass = AInnoPlayerController::StaticClass();
	PlayerStateClass = AInnoPlayerState::StaticClass();
	GameStateClass = AInnoRaceGameState::StaticClass();
	DefaultPawnClass = nullptr;

	MinPlayersToStart = 2;
	MaxPlayers = 2;
	bSessionLocked = false;
	bUseSeamlessTravel = true;
}

void AInnoLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 1. Reject if session is locked
	if (bSessionLocked)
	{
		ErrorMessage = TEXT("Session is locked. Match is currently starting.");
		return;
	}

	// 2. Reject if lobby is already full
	if (GetNumPlayers() >= MaxPlayers)
	{
		ErrorMessage = TEXT("Lobby is full.");
		return;
	}

	// 3. BuildId Validation
	const FString ClientBuildId = UGameplayStatics::ParseOption(Options, TEXT("BuildId"));
	if (!ClientBuildId.IsEmpty() && ClientBuildId != ExpectedBuildId)
	{
		ErrorMessage = FString::Printf(TEXT("BuildId mismatch. Server: %s, Client: %s"), *ExpectedBuildId, *ClientBuildId);
		return;
	}
}

void AInnoLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AInnoPlayerState* NewPS = NewPlayer->GetPlayerState<AInnoPlayerState>();
	if (!NewPS)
	{
		return;
	}

	// Auto-suffix duplicate participant names inside the lobby (e.g. "Alex (2)")
	FString CandidateName = NewPS->ParticipantName.IsEmpty() ? TEXT("Pilot") : NewPS->ParticipantName;
	int32 Suffix = 2;

	bool bNameCollision = true;
	while (bNameCollision)
	{
		bNameCollision = false;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC != NewPlayer)
			{
				if (AInnoPlayerState* OtherPS = PC->GetPlayerState<AInnoPlayerState>())
				{
					if (OtherPS->ParticipantName.Equals(CandidateName, ESearchCase::IgnoreCase))
					{
						bNameCollision = true;
						CandidateName = FString::Printf(TEXT("%s (%d)"), *NewPS->ParticipantName, Suffix);
						Suffix++;
						break;
					}
				}
			}
		}
	}

	NewPS->ParticipantName = CandidateName;
	CheckAllPlayersReady();
}

void AInnoLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	CheckAllPlayersReady();
}

void AInnoLobbyGameMode::CheckAllPlayersReady()
{
	if (bSessionLocked || GetNumPlayers() < MinPlayersToStart)
	{
		return;
	}

	bool bAllReady = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			AInnoPlayerState* PS = PC->GetPlayerState<AInnoPlayerState>();
			if (!PS || !PS->bIsReady)
			{
				bAllReady = false;
				break;
			}
		}
	}

	if (bAllReady)
	{
		StartRaceTravel(TEXT("/Game/InnoRace/Maps/L_Track1"));
	}
}

void AInnoLobbyGameMode::StartRaceTravel(const FString& FirstTrackMapPath)
{
	bSessionLocked = true;
	GetWorld()->ServerTravel(FString::Printf(TEXT("%s?listen"), *FirstTrackMapPath), true);
}
