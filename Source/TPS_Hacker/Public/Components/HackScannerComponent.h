// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HackScannerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHackTargetChanged, AActor*, OldTarget, AActor*, NewTarget);


// 해킹(Q키) 대상 스캐너
// - LineTrace 기반(카메라 전방)
// - IHackableInterface 구현 + CanHack 통과 대상이면 타겟으로 채택

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPS_HACKER_API  UHackScannerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHackScannerComponent();

	// Public API

	// 스캐너가 참조할 카메라 컴포넌트 설정
	UFUNCTION(BlueprintCallable, Category="Interact|Hack")
	void Initialize(class UCameraComponent* InCamera);

	//스캔 타이머 시작
	UFUNCTION(BlueprintCallable, Category="Interact|Hack")
	void StartScan();

	//스캔 타이머 정지
	UFUNCTION(BlueprintCallable, Category="Interact|Hack")
	void StopScan();

	//현재 타겟 반환
	UFUNCTION(BlueprintCallable, Category="Interact|Hack")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// 타겟 변경 델리게이트 이벤트
	UPROPERTY(BlueprintAssignable, Category="Interact|Hack")
	FOnHackTargetChanged OnTargetChanged;
	

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 스캔 함수
	void ScanOnce();
	// 타겟 세터
	void SetCurrentTarget(AActor* NewTarget);
	// 해킹 후보 유효성 검사
	bool IsValidHackCandidate(AActor* Candidate) const;

	// 카메라가 없으면 Owner 기준으로 대체
	void GetViewPoint(FVector& OutLoc, FRotator& OutRot) const;

private:
	
	// Config

	UPROPERTY(EditAnywhere, Category="Interact|Hack|Config")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, Category="Interact|Hack|Config", meta=(ClampMin="0.01", ClampMax="1.0"))
	float ScanInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category="Interact|Hack|Config", meta=(ClampMin="100.0", ClampMax="5000.0"))
	float MaxRange = 1200.0f;

	// LineTrace 채널(프로젝트에서 Trace_Hack 만들고 넣기
	UPROPERTY(EditAnywhere, Category="Interact|Hack|Config")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel4;  //ECC_GameTraceChannel4 = Hack 채널 

	UPROPERTY(EditAnywhere, Category="Interact|Hack|Debug")
	bool bDebugDraw = true;

private:
	// -----------------------------
	// Runtime
	// -----------------------------
	UPROPERTY(Transient)
	AActor* CurrentTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UCameraComponent> CameraRef = nullptr;

	FTimerHandle ScanTimerHandle;
};