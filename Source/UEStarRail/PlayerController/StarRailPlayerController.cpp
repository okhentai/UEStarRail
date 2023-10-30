// Fill out your copyright notice in the Description page of Project Settings.


#include "StarRailPlayerController.h"
#include "UEStarRail/HUD/StarRailHUD.h"
#include "UEStarRail/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UEStarRail/Character/StarRailCharacter.h"
#include "Net/UnrealNetwork.h"
#include "UEStarRail/GameMode/StarRailGameMode.h"
#include "UEStarRail/PlayerState/StarRailPlayerState.h"
#include "UEStarRail/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "UEStarRail/StarRailComponents/CombatComponent.h"
#include "UEStarRail/Weapon/Weapon.h"
#include "UEStarRail/GameState/StarRailGameState.h"

void AStarRailPlayerController::BeginPlay()
{
	Super::BeginPlay();

	StarRailHUD = Cast<AStarRailHUD>(GetHUD());
	ServerCheckMatchState();
}

void AStarRailPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AStarRailPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStarRailPlayerController, MatchState);
}

void AStarRailPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AStarRailPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (StarRailHUD && MatchState == MatchState::WaitingToStart)
	{
		StarRailHUD->AddAnnouncement();
	}
}

void AStarRailPlayerController::ServerCheckMatchState_Implementation()
{
	AStarRailGameMode* GameMode = Cast<AStarRailGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AStarRailPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;

	bool bHUDValid = StarRailHUD &&
		StarRailHUD->CharacterOverlay &&
		StarRailHUD->CharacterOverlay->HealthBar &&
		StarRailHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		StarRailHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		StarRailHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AStarRailPlayerController::SetHUDScore(float Score)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	bool bHUDValid = StarRailHUD &&
		StarRailHUD->CharacterOverlay &&
		StarRailHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		StarRailHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AStarRailPlayerController::SetHUDDefeats(int32 Defeats)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	bool bHUDValid = StarRailHUD &&
		StarRailHUD->CharacterOverlay &&
		StarRailHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		StarRailHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AStarRailPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	bool bHUDValid = StarRailHUD &&
		StarRailHUD->CharacterOverlay &&
		StarRailHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		StarRailHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AStarRailPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	bool bHUDValid = StarRailHUD &&
		StarRailHUD->CharacterOverlay &&
		StarRailHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		StarRailHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AStarRailPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AStarRailCharacter* StarRailCharacter = Cast<AStarRailCharacter>(InPawn);
	if (StarRailCharacter)
	{
		SetHUDHealth(StarRailCharacter->GetHealth(), StarRailCharacter->GetMaxHealth());
	}
}

void AStarRailPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	bool bHUDValid = StarRailHUD &&
		StarRailHUD->CharacterOverlay &&
		StarRailHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			StarRailHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		StarRailHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AStarRailPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	bool bHUDValid = StarRailHUD &&
		StarRailHUD->Announcement &&
		StarRailHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			StarRailHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		StarRailHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void AStarRailPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void AStarRailPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (StarRailHUD && StarRailHUD->CharacterOverlay)
		{
			CharacterOverlay = StarRailHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void AStarRailPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AStarRailPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AStarRailPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AStarRailPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AStarRailPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AStarRailPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AStarRailPlayerController::HandleMatchHasStarted()
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	if (StarRailHUD)
	{
		if (StarRailHUD->CharacterOverlay == nullptr) StarRailHUD->AddCharacterOverlay();
		if (StarRailHUD->Announcement)
		{
			StarRailHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AStarRailPlayerController::HandleCooldown()
{
	StarRailHUD = StarRailHUD == nullptr ? Cast<AStarRailHUD>(GetHUD()) : StarRailHUD;
	if (StarRailHUD)
	{
		StarRailHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = StarRailHUD->Announcement &&
			StarRailHUD->Announcement->AnnouncementText &&
			StarRailHUD->Announcement->InfoText;

		if (bHUDValid)
		{
			StarRailHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			StarRailHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			AStarRailGameState* StarRailGameState = Cast<AStarRailGameState>(UGameplayStatics::GetGameState(this));
			AStarRailPlayerState* StarRailPlayerState = GetPlayerState<AStarRailPlayerState>();
			if (StarRailGameState && StarRailPlayerState)
			{
				TArray<AStarRailPlayerState*> TopPlayers = StarRailGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == StarRailPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				StarRailHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	AStarRailCharacter* StarRailCharacter = Cast<AStarRailCharacter>(GetPawn());
	if (StarRailCharacter && StarRailCharacter->GetCombat())
	{
		StarRailCharacter->bDisableGameplay = true;
		StarRailCharacter->GetCombat()->FireButtonPressed(false);
	}
}


