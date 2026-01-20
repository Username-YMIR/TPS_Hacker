// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractScannerComponent.generated.h"


// BP에서도 바인딩 가능하게 Dynamic Multicast로 제공
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCloseTargetChanged, AActor*, OldTarget, AActor*, NewTarget);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPS_HACKER_API UInteractScannerComponent : public UActorComponent
{
	GENERATED_BODY()
	// -----------------------------
	// Public API
	// -----------------------------
	
public:
	UInteractScannerComponent();

	// 스캔 시작(타이머 기반). 보통 BeginPlay에서 자동 시작됨
	UFUNCTION(BlueprintCallable, Category="Interact|Close")
	void StartScan();

	// 스캔 정지
	UFUNCTION(BlueprintCallable, Category="Interact|Close")
	void StopScan();

	// 현재 선택된 근거리 타겟 반환
	UFUNCTION(BlueprintCallable, Category="Interact|Close")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// 타겟 변경 이벤트(BP 바인딩 가능)
	UPROPERTY(BlueprintAssignable, Category="Interact|Close")
	FOnCloseTargetChanged OnTargetChanged;

protected:
	// -----------------------------
	// UActorComponent overrides
	// -----------------------------
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// -----------------------------
	// Internal logic
	// -----------------------------

	// 1회 스캔 실행
	void ScanOnce();

	// 타겟 갱신(변경 시 델리게이트 호출)
	void SetCurrentTarget(AActor* NewTarget);

	// 이 액터가 근거리 인터랙션 후보인지 검사
	bool IsValidCloseCandidate(AActor* Candidate) const;

	// 스캔 기준 위치(기본: Owner Actor 위치)
	FVector GetScanOrigin() const;

private:
	// -----------------------------
	// Config (Editor)
	// -----------------------------

	// BeginPlay 시 자동으로 스캔을 시작할지 설정값
	UPROPERTY(EditAnywhere, Category="Interact|Close|Config")
	bool bAutoStart = true;

	// 스캔 주기
	UPROPERTY(EditAnywhere, Category="Interact|Close|Config", meta=(ClampMin="0.01", ClampMax="1.0"))
	float ScanInterval = 0.05f;

	// 근거리 스캔 최대 거리 
	UPROPERTY(EditAnywhere, Category="Interact|Close|Config", meta=(ClampMin="50.0", ClampMax="1000.0"))
	float MaxRange = 250.0f;

	// Sphere 반경(체감용)
	UPROPERTY(EditAnywhere, Category="Interact|Close|Config", meta=(ClampMin="10.0", ClampMax="300.0"))
	float SphereRadius = 100.0f;

	
	// Overlap에 사용할 Object Channel
	// Interactable 오브젝트 채널을 만들어서 지정하기
	UPROPERTY(EditAnywhere, Category="Interact|Close|Config")
	TEnumAsByte<ECollisionChannel> OverlapObjectChannel = ECC_GameTraceChannel3; // ECC_GameTraceChannel3 = Interact

	// 디버그 드로우
	UPROPERTY(EditAnywhere, Category="Interact|Close|Debug")
	bool bDebugDraw = true;

private:
	// -----------------------------
	// Runtime state
	// -----------------------------

	//현재 타겟(근거리)
	UPROPERTY(Transient)
	AActor* CurrentTarget = nullptr;

	//타이머 핸들
	FTimerHandle ScanTimerHandle;	
};
