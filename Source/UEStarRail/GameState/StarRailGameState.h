// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "StarRailGameState.generated.h"

/**
 * 
 */
UCLASS()
class UESTARRAIL_API AStarRailGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class AStarRailPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<AStarRailPlayerState*> TopScoringPlayers;
private:

	float TopScore = 0.f;
};
