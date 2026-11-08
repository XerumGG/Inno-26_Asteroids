// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/UI/InnoUIFlowSubsystem.h"

void UInnoUIFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ScreenStack.Empty();
	ScreenStack.Add(EInnoScreen::MainMenu);
}

void UInnoUIFlowSubsystem::PushScreen(EInnoScreen Screen)
{
	if (Screen != EInnoScreen::None)
	{
		ScreenStack.Add(Screen);
		OnScreenNavigated.Broadcast(Screen);
	}
}

void UInnoUIFlowSubsystem::PopScreen()
{
	if (ScreenStack.Num() > 1)
	{
		ScreenStack.Pop();
		OnScreenNavigated.Broadcast(GetCurrentScreen());
	}
}

void UInnoUIFlowSubsystem::ResetToMainMenu()
{
	ScreenStack.Empty();
	ScreenStack.Add(EInnoScreen::MainMenu);
	OnScreenNavigated.Broadcast(EInnoScreen::MainMenu);
}

void UInnoUIFlowSubsystem::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
	switch (NewPhase)
	{
	case EMatchPhase::Lobby:
		PushScreen(EInnoScreen::MatchLobby);
		break;
	case EMatchPhase::LoadGate:
		PushScreen(EInnoScreen::Loading);
		break;
	case EMatchPhase::Countdown:
		PushScreen(EInnoScreen::Countdown);
		break;
	case EMatchPhase::Racing:
	case EMatchPhase::Finishing:
		PushScreen(EInnoScreen::RaceHUD);
		break;
	case EMatchPhase::RaceResults:
		PushScreen(EInnoScreen::PlacementBanner);
		break;
	case EMatchPhase::Intermission:
		PushScreen(EInnoScreen::Intermission);
		break;
	case EMatchPhase::MatchComplete:
	case EMatchPhase::Detach:
		PushScreen(EInnoScreen::Results);
		break;
	default:
		break;
	}
}

EInnoScreen UInnoUIFlowSubsystem::GetCurrentScreen() const
{
	return (ScreenStack.Num() > 0) ? ScreenStack.Last() : EInnoScreen::None;
}
