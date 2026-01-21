// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPoolableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TPS_HACKER_API IPoolableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 풀에서 꺼낼 때 호출 (활성화 직전/직후)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Pool")
	void OnAcquireFromPool();

	// 풀로 반납할 때 호출 (비활성화 직전/직후)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool")
	void OnReleaseToPool();
};
