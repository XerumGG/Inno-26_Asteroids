// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "InnoSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchCompletedSignature, bool, bFoundLobby);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionHostCompletedSignature, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinCompletedSignature, bool, bSuccess);

/**
 * LAN Session Discovery and Peer-Hosting Subsystem.
 * Manages LAN search, hosting listen server, joining, and deterministic lobby merge.
 */
UCLASS()
class INNO_RACING_API UInnoSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Searches LAN for open lobbies matching active mode and BuildId
	UFUNCTION(BlueprintCallable, Category = "Inno|Session")
	void FindMatch(const FString& MatchMode, const FString& BuildId);

	// Creates a new LAN listen server session as host
	UFUNCTION(BlueprintCallable, Category = "Inno|Session")
	void HostSession(const FString& MatchMode, const FString& BuildId, int32 MaxPlayers = 2);

	// Joins an open discovered lobby
	UFUNCTION(BlueprintCallable, Category = "Inno|Session")
	void JoinSession();

	// Locks session to prevent new joins during travel/racing
	UFUNCTION(BlueprintCallable, Category = "Inno|Session")
	void LockSession();

	UPROPERTY(BlueprintAssignable, Category = "Inno|Session")
	FOnSessionSearchCompletedSignature OnSearchCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Inno|Session")
	FOnSessionHostCompletedSignature OnHostCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Inno|Session")
	FOnSessionJoinCompletedSignature OnJoinCompleted;

protected:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;

	FString PendingMode;
	FString PendingBuildId;
	uint64 LocalSessionId = 0;
};
