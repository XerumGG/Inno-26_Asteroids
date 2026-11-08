// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoRaceGameMode.h"
#include "Inno_Racing/Race/InnoPlayerController.h"
#include "Inno_Racing/Race/InnoPlayerState.h"
#include "Inno_Racing/Race/InnoRaceGameState.h"
#include "Inno_Racing/Race/InnoScoring.h"
#include "Inno_Racing/Hover/HoverCarPawn.h"
#include "Inno_Racing/Store/InnoLocalStore.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AInnoRaceGameMode::AInnoRaceGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseSeamlessTravel = true;

	PlayerControllerClass = AInnoPlayerController::StaticClass();
	PlayerStateClass = AInnoPlayerState::StaticClass();
	GameStateClass = AInnoRaceGameState::StaticClass();
	DefaultPawnClass = AHoverCarPawn::StaticClass();

	CountdownSec = 3.0f;
	GracePeriodSec = 30.0f;
	IntermissionSec = 10.0f;
	LoadGateTimeoutSec = 45.0f;
	TrackTimeLimitMs = 180000;

	PhaseTimer = 0.0f;
	bFirstFinisherArrived = false;
	CurrentMatchId = FGuid::NewGuid();
}

void AInnoRaceGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	CurrentRaceEntries.Empty();
	bFirstFinisherArrived = false;
	AdvanceToPhase(EMatchPhase::LoadGate);
}

void AInnoRaceGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	CheckLoadGate();
}

void AInnoRaceGameMode::OnPlayerReportedLoaded(AInnoPlayerController* PC)
{
	if (PC)
	{
		if (AInnoPlayerState* PS = PC->GetPlayerState<AInnoPlayerState>())
		{
			PS->bHasLoadedMap = true;
		}
	}
	CheckLoadGate();
}

void AInnoRaceGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>();
	if (!InnoGS)
	{
		return;
	}

	PhaseTimer += DeltaSeconds;

	switch (InnoGS->MatchPhase)
	{
	case EMatchPhase::LoadGate:
		if (PhaseTimer >= LoadGateTimeoutSec)
		{
			StartCountdown();
		}
		break;

	case EMatchPhase::Countdown:
		if (GetWorld()->GetTimeSeconds() >= InnoGS->StartServerTime)
		{
			StartRacing();
		}
		break;

	case EMatchPhase::Finishing:
		CheckGracePeriod(DeltaSeconds);
		break;

	case EMatchPhase::Intermission:
		if (PhaseTimer >= IntermissionSec)
		{
			CompleteMatch();
		}
		break;

	default:
		break;
	}
}

void AInnoRaceGameMode::AdvanceToPhase(EMatchPhase NewPhase)
{
	PhaseTimer = 0.0f;
	if (AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>())
	{
		InnoGS->MatchPhase = NewPhase;
		InnoGS->OnRep_MatchPhase(); // Ensure listen-server host also fires local phase change delegates
	}
}

void AInnoRaceGameMode::CheckLoadGate()
{
	AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>();
	if (!InnoGS || InnoGS->MatchPhase != EMatchPhase::LoadGate)
	{
		return;
	}

	bool bAllLoaded = true;
	int32 PlayerCount = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			PlayerCount++;
			AInnoPlayerState* PS = PC->GetPlayerState<AInnoPlayerState>();
			if (!PS || !PS->bHasLoadedMap)
			{
				bAllLoaded = false;
				break;
			}
		}
	}

	if (bAllLoaded && PlayerCount > 0)
	{
		StartCountdown();
	}
}

void AInnoRaceGameMode::StartCountdown()
{
	if (AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>())
	{
		InnoGS->StartServerTime = GetWorld()->GetTimeSeconds() + CountdownSec;
	}
	AdvanceToPhase(EMatchPhase::Countdown);
}

void AInnoRaceGameMode::StartRacing()
{
	AdvanceToPhase(EMatchPhase::Racing);

	// Notify all possessed hover pawns that the race has officially started (GO)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			if (AHoverCarPawn* HoverPawn = Cast<AHoverCarPawn>(PC->GetPawn()))
			{
				HoverPawn->OnRaceStarted();
			}
		}
	}
}

void AInnoRaceGameMode::OnPlayerCrossedFinishLine(AInnoPlayerController* PC, int64 CrossingTimeMs)
{
	if (!PC)
	{
		return;
	}

	AInnoPlayerState* PS = PC->GetPlayerState<AInnoPlayerState>();
	if (!PS || PS->ParticipantStatus == EParticipantStatus::Finished)
	{
		return;
	}

	PS->ParticipantStatus = EParticipantStatus::Finished;
	PS->FinishTimeMs = CrossingTimeMs;

	FInnoRaceResultEntry Entry;
	Entry.PlayerName = PS->ParticipantName;
	Entry.CarId = PS->SelectedCarId;
	Entry.Status = EParticipantStatus::Finished;
	Entry.TimeMs = CrossingTimeMs;
	CurrentRaceEntries.Add(Entry);

	// First finisher triggers the Grace Period timer for remaining participants
	if (!bFirstFinisherArrived)
	{
		bFirstFinisherArrived = true;
		if (AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>())
		{
			InnoGS->GraceDeadlineServerTime = GetWorld()->GetTimeSeconds() + GracePeriodSec;
		}
		AdvanceToPhase(EMatchPhase::Finishing);
	}

	// Check if all connected players have finished
	bool bAllFinished = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* ConnectedPC = It->Get();
		if (ConnectedPC)
		{
			AInnoPlayerState* ConnectedPS = ConnectedPC->GetPlayerState<AInnoPlayerState>();
			if (ConnectedPS && ConnectedPS->ParticipantStatus == EParticipantStatus::Racing)
			{
				bAllFinished = false;
				break;
			}
		}
	}

	if (bAllFinished)
	{
		FinalizeRace();
	}
}

void AInnoRaceGameMode::CheckGracePeriod(float DeltaSeconds)
{
	AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>();
	if (!InnoGS)
	{
		return;
	}

	if (GetWorld()->GetTimeSeconds() >= InnoGS->GraceDeadlineServerTime)
	{
		// Mark remaining unfinished racers as DNF
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* ConnectedPC = It->Get();
			if (ConnectedPC)
			{
				AInnoPlayerState* ConnectedPS = ConnectedPC->GetPlayerState<AInnoPlayerState>();
				if (ConnectedPS && ConnectedPS->ParticipantStatus == EParticipantStatus::Racing)
				{
					ConnectedPS->ParticipantStatus = EParticipantStatus::DNF;

					FInnoRaceResultEntry DnfEntry;
					DnfEntry.PlayerName = ConnectedPS->ParticipantName;
					DnfEntry.CarId = ConnectedPS->SelectedCarId;
					DnfEntry.Status = EParticipantStatus::DNF;
					CurrentRaceEntries.Add(DnfEntry);
				}
			}
		}

		FinalizeRace();
	}
}

void AInnoRaceGameMode::FinalizeRace()
{
	AdvanceToPhase(EMatchPhase::RaceResults);

	// 1. Authoritative ranking via pure C++ FInnoScoring
	FInnoScoring::RankRace(CurrentRaceEntries, TrackTimeLimitMs, 10000);

	FInnoRaceResult RaceResult;
	if (AInnoRaceGameState* InnoGS = GetGameState<AInnoRaceGameState>())
	{
		RaceResult.RaceIndex = InnoGS->CurrentRaceIndex;
	}
	RaceResult.TrackId = FName(*GetWorld()->GetName());
	RaceResult.Entries = CurrentRaceEntries;
	CompletedRaceResults.Add(RaceResult);

	// 2. Update series stats and broadcast reliable race results to each connected client
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AInnoPlayerController* PC = Cast<AInnoPlayerController>(It->Get()))
		{
			PC->Client_ReceiveRaceResult(RaceResult);

			AInnoPlayerState* PS = PC->GetPlayerState<AInnoPlayerState>();
			if (PS)
			{
				for (const FInnoRaceResultEntry& Entry : CurrentRaceEntries)
				{
					if (Entry.PlayerName.Equals(PS->ParticipantName))
					{
						PS->SeriesTotalTimeMs += Entry.TimeMs;
						if (Entry.Rank == 1 && Entry.Status == EParticipantStatus::Finished)
						{
							PS->SeriesRaceWins++;
						}

						FString BannerText = (Entry.Rank == 1) ? TEXT("YOU WON!") : FString::Printf(TEXT("You finished %d!"), Entry.Rank);
						if (Entry.Status == EParticipantStatus::DNF)
						{
							BannerText = TEXT("DNF");
						}
						PC->Client_PlacementBanner(Entry.Rank, Entry.TimeMs, BannerText);
						break;
					}
				}
			}
		}
	}

	AdvanceToPhase(EMatchPhase::Intermission);
}

void AInnoRaceGameMode::CompleteMatch()
{
	AdvanceToPhase(EMatchPhase::MatchComplete);

	// 1. Compute final series standings
	FInnoMatchSummary FinalSummary;
	FinalSummary.MatchId = CurrentMatchId;
	FinalSummary.Races = CompletedRaceResults;
	FInnoScoring::ComputeStandings(CompletedRaceResults, FinalSummary.Standings);

	// Persist match summary locally on host
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInnoLocalStore* Store = GI->GetSubsystem<UInnoLocalStore>())
		{
			Store->RecordMatchSummary(FinalSummary);
		}
	}

	// 2. Deliver final summary reliably to each client
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AInnoPlayerController* PC = Cast<AInnoPlayerController>(It->Get()))
		{
			PC->Client_ReceiveFinalSummary(FinalSummary);
		}
	}

	AdvanceToPhase(EMatchPhase::Detach);
}
