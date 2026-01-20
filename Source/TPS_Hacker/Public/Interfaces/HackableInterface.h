// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HackableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UHackableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TPS_HACKER_API IHackableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 해킹 가능 여부(전원, 이미 해킹됨, 권한, 쿨다운 등). 조회 전용.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CanHack(AActor* Hacker) const;

	// 해킹 UI 텍스트(예: "해킹", "카메라 접속", "전원 차단").
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FText GetHackText() const;

	// 해킹 표시 기준 컴포넌트(아이콘/라인 연결 기준점).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UPrimitiveComponent* GetHackRoot() const;

	// 해킹 비용/거리 같은 제약을 대상이 제공(데이터 기반 확장용).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	float GetHackRange() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	int32 GetHackCost() const;

	// 실제 해킹 실행(상태 변경 + 이벤트 브로드캐스트).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ExecuteHack(AActor* Hacker);
};
