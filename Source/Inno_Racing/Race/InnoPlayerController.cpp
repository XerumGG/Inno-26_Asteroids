// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoPlayerController.h"
#include "Inno_Racing/Race/InnoPlayerState.h"

AInnoPlayerController::AInnoPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AInnoPlayerController::Server_SetReady_Implementation(bool bReady)
{
	if (AInnoPlayerState* InnoPS = GetPlayerState<AInnoPlayerState>())
	{
		InnoPS->bIsReady = bReady;
	}
}

void AInnoPlayerController::Server_ReportLoaded_Implementation()
{
	if (AInnoPlayerState* InnoPS = GetPlayerState<AInnoPlayerState>())
	{
		InnoPS->bHasLoadedMap = true;
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
