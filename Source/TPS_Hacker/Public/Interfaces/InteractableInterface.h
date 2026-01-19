// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TPS_HACKER_API IInteractableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 상호작용 가능 여부를 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CanInteract(AActor* Interactor) const;

	// 현재 대상에 대해 UI에 표시할 상호작용 문구를 반환
	// - 예: "해킹", "열기", "잠김", "전원 켜기" 등
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FText GetInteractText() const;

	// 상호작용 대상의 "표시 기준 컴포넌트"를 반환.
	// - 월드 스페이스 위젯 부착 위치, 하이라이트, 디버그 표시에 사용한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	USceneComponent* GetInteractRoot() const;

	// 상호작용(또는 해킹) 실행 엔트리 포인트.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnInteract(AActor* Interactor);
};
