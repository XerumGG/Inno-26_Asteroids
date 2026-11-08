// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlacementBannerReceivedSignature, int32, Rank, int64, TimeMs, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaceResultReceivedSignature, const FInnoRaceResult&, RaceResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFinalSummaryReceivedSignature, const FInnoMatchSummary&, Summary);

/**
 * Persists across seamless travel.
 * Manages player input contexts and high-level match RPCs.
 */
UCLASS()
class INNO_RACING_API AInnoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AInnoPlayerController();

	virtual void BeginPlay() override;

	// --- Server RPCs ---
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inno|Net")
	void Server_SetReady(bool bReady);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inno|Net")
	void Server_ReportLoaded();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inno|Net")
	void Server_AckSummaryReceived();

	// --- Client RPCs ---
	UFUNCTION(Client, Reliable)
	void Client_PlacementBanner(int32 Rank, int64 FinishTimeMs, const FString& BannerText);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveRaceResult(const FInnoRaceResult& Result);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveFinalSummary(const FInnoMatchSummary& Summary);

	// Client-side UI Event Dispatchers
	UPROPERTY(BlueprintAssignable, Category = "Inno|Events")
	FOnPlacementBannerReceivedSignature OnPlacementBannerReceived;

	UPROPERTY(BlueprintAssignable, Category = "Inno|Events")
	FOnRaceResultReceivedSignature OnRaceResultReceived;

	UPROPERTY(BlueprintAssignable, Category = "Inno|Events")
	FOnFinalSummaryReceivedSignature OnFinalSummaryReceived;
};
