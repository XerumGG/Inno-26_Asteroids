// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoPlayerController.h"
#include "Inno_Racing/Race/InnoPlayerState.h"
#include "Inno_Racing/Race/InnoLobbyGameMode.h"
#include "Inno_Racing/Race/InnoRaceGameMode.h"
#include "Engine/World.h"

AInnoPlayerController::AInnoPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AInnoPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);

		// Automatically notify server that map is loaded when entering a race level
		Server_ReportLoaded();
	}
}

void AInnoPlayerController::Server_SetReady_Implementation(bool bReady)
{
	if (AInnoPlayerState* InnoPS = GetPlayerState<AInnoPlayerState>())
	{
		InnoPS->bIsReady = bReady;
	}

	if (UWorld* World = GetWorld())
	{
		if (AInnoLobbyGameMode* LobbyGM = World->GetAuthGameMode<AInnoLobbyGameMode>())
		{
			LobbyGM->CheckAllPlayersReady();
		}
	}
}

void AInnoPlayerController::Server_ReportLoaded_Implementation()
{
	if (AInnoPlayerState* InnoPS = GetPlayerState<AInnoPlayerState>())
	{
		InnoPS->bHasLoadedMap = true;
	}

	if (UWorld* World = GetWorld())
	{
		if (AInnoRaceGameMode* RaceGM = World->GetAuthGameMode<AInnoRaceGameMode>())
		{
			RaceGM->OnPlayerReportedLoaded(this);
		}
	}
}

void AInnoPlayerController::Server_AckSummaryReceived_Implementation()
{
	// Acknowledged by client; host uses this to safely proceed to Detach state
}

void AInnoPlayerController::Client_PlacementBanner_Implementation(int32 Rank, int64 FinishTimeMs, const FString& BannerText)
{
	OnPlacementBannerReceived.Broadcast(Rank, FinishTimeMs, BannerText);
}

void AInnoPlayerController::Client_ReceiveRaceResult_Implementation(const FInnoRaceResult& Result)
{
	OnRaceResultReceived.Broadcast(Result);
}

void AInnoPlayerController::Client_ReceiveFinalSummary_Implementation(const FInnoMatchSummary& Summary)
{
	OnFinalSummaryReceived.Broadcast(Summary);
}
