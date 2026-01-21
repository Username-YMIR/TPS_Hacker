// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TPS_HackerGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ATPS_HackerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Constructor */
	ATPS_HackerGameMode();
};
