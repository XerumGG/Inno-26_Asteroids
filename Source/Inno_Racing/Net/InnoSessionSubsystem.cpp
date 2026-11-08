// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Net/InnoSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"

void UInnoSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UInnoSessionSubsystem::OnCreateSessionComplete);
	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UInnoSessionSubsystem::OnFindSessionsComplete);
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UInnoSessionSubsystem::OnJoinSessionComplete);

	LocalSessionId = FMath::RandRange(1000000, 9999999);
}

void UInnoSessionSubsystem::FindMatch(const FString& MatchMode, const FString& BuildId)
{
	PendingMode = MatchMode;
	PendingBuildId = BuildId;

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		OnSearchCompleted.Broadcast(false);
		return;
	}

	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSearchCompleted.Broadcast(false);
		return;
	}

	FindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->bIsLanQuery = true;
	LastSessionSearch->MaxSearchResults = 10;
	LastSessionSearch->QuerySettings.Set(FName(TEXT("SEARCH_LOBBIES")), true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	Sessions->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());
}

void UInnoSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		}
	}

	if (bWasSuccessful && LastSessionSearch.IsValid() && LastSessionSearch->SearchResults.Num() > 0)
	{
		// Open lobby found on LAN
		OnSearchCompleted.Broadcast(true);
	}
	else
	{
		// No open lobby found -> Station will become Host
		OnSearchCompleted.Broadcast(false);
	}
}

void UInnoSessionSubsystem::HostSession(const FString& MatchMode, const FString& BuildId, int32 MaxPlayers)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		OnHostCompleted.Broadcast(false);
		return;
	}

	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnHostCompleted.Broadcast(false);
		return;
	}

	CreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinInProgress = false;
	SessionSettings.NumPublicConnections = MaxPlayers;

	SessionSettings.Set(FName(TEXT("MAPNAME")), FString(TEXT("L_Lobby")), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName(TEXT("MatchMode")), MatchMode, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName(TEXT("BuildId")), BuildId, EOnlineDataAdvertisementType::ViaOnlineService);

	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	Sessions->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionSettings);
}

void UInnoSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		}
	}

	if (bWasSuccessful)
	{
		// Travel host into L_Lobby as a listen server
		UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("L_Lobby")), true, TEXT("listen"));
	}

	OnHostCompleted.Broadcast(bWasSuccessful);
}

void UInnoSessionSubsystem::JoinSession()
{
	if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() == 0)
	{
		OnJoinCompleted.Broadcast(false);
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		OnJoinCompleted.Broadcast(false);
		return;
	}

	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnJoinCompleted.Broadcast(false);
		return;
	}

	JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	Sessions->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, LastSessionSearch->SearchResults[0]);
}

void UInnoSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		}
	}

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
		FString ConnectString;
		if (Sessions->GetResolvedConnectString(SessionName, ConnectString))
		{
			APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
			if (PC)
			{
				PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
		}
		OnJoinCompleted.Broadcast(true);
	}
	else
	{
		OnJoinCompleted.Broadcast(false);
	}
}

void UInnoSessionSubsystem::LockSession()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
		if (Sessions.IsValid())
		{
			FOnlineSessionSettings* Settings = Sessions->GetSessionSettings(NAME_GameSession);
			if (Settings)
			{
				Settings->bShouldAdvertise = false;
				Settings->bAllowJoinInProgress = false;
				Sessions->UpdateSession(NAME_GameSession, *Settings, true);
			}
		}
	}
}
