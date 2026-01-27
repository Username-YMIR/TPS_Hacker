// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TimelineComponent.h"
#include "TPS_Hacker_Character.generated.h"


// 플레이어 캐릭터
// Interact/Hack 스캐너 컴포넌트를 보유
//  - E/Q 입력을 해당 타겟에게 인터페이스 호출로 라우팅


class UTPS_TagRelationshipMap;

UCLASS()
class TPS_HACKER_API ATPS_Hacker_Character : public ACharacter
{
	GENERATED_BODY()

#pragma region Components
    private:
    	//Camera
    	UPROPERTY(VisibleAnywhere, Category = "Camera")
    	TObjectPtr<class USpringArmComponent> SpringArm = nullptr;
    
    	UPROPERTY(VisibleAnywhere, Category = "Camera")
    	TObjectPtr<class UCameraComponent> FollowCamera = nullptr;
    	
    	//FocusVFX
    	UPROPERTY(VisibleAnywhere, Category="Focus|VFX")
    	class UPostProcessComponent* FocusPostProcess = nullptr;
	
		UPROPERTY(EditDefaultsOnly, Category="Focus|VFX")
		float FocusVFX_OnWeight = 1.0f;

		UPROPERTY(EditDefaultsOnly, Category="Focus|VFX")
		float FocusVFX_FadeTime  = 0.15f;
	
		FTimerHandle FocusVFXFadeTimer;

		float FocusVFX_CurrentWeight = 0.0f;
		float FocusVFX_TargetWeight = 0.0f;
		float FocusVFX_FadeSpeedPerSec = 0.0f;
    
    	//Components
    	UPROPERTY(VisibleAnywhere, Category = "Interact")
    	TObjectPtr<class UInteractScannerComponent> InteractScanner = nullptr;
    
    	UPROPERTY(VisibleAnywhere, Category = "Intercat")
    	TObjectPtr<class UHackScannerComponent> HackScanner = nullptr;
    	
    	
    
    protected:
    	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pool")
    	TObjectPtr<class UObjectPoolComponent> PoolComp = nullptr;
	
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
		TObjectPtr<class UGunComponent> GunComp = nullptr;
    #pragma endregion
	
#pragma region Overrided
public:
	// Sets default values for this character's properties
	ATPS_Hacker_Character();
	// AActor
	virtual void Tick(float DeltaSeconds) override;
	
	inline bool IsWantsAim() const { return bWantsAim; }
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
#pragma endregion

	
	
	
	
#pragma region Temp_WeaponComponentProperties
public:
	// GunComponent가 사용(마이그레이션용)
	class UObjectPoolComponent* GetPoolComponent() const { return PoolComp; }

	ECollisionChannel GetOwnerCollisionChannel() const { return MyCollisionChannel; }

	// 기존 캐릭터 임시 ProjectileClass fallback
	TSubclassOf<AActor> GetFallbackProjectileClass() const { return ProjectileClass; }

	
	//전투
	UPROPERTY(EditAnywhere, Category="Combat")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Category="Combat")
	FName MuzzleSocketName = "Muzzle";
	
	//콜리전 /추후 총기 컴포넌트로 옮기고 총기 컴포넌트에서 오너로 접근해 가져올 예정
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
	// TEnumAsByte<ECollisionChannel> MyCollisionChannel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	TEnumAsByte<ECollisionChannel> MyCollisionChannel = ECC_MAX;
#pragma endregion

#pragma region Input
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	//Do - Input 시 실제 작동 함수
	void Do_RunWalkToggle(); // Shift 키, 달리기/걷기 토글
	void Do_Hack(); // Q 키, 원거리 상호작용(해킹)
	void Do_Interact(); // E 키, 근거리 상호작용
	void Do_Fire(); // 좌클릭, 발사
	void Do_FireReleased(); // 좌클릭 떼기
	void Do_Takedown(); // F키, 처형
	
	// 포커스 모드
	void ToggleFocusMode();
	void EnterFocusMode();
	void ExitFocusMode();
	void ForceExitFocusMode();
	

public:
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	void SetShoulder(bool bRight);
	
	void OnToggleWeapon(const FInputActionValue& Value);
	
	
#pragma region Input|Aim
protected:
	//----------------------
	// 조준 관련 (Aim)
	//----------------------

	// (Enhanced Input에서 Started/Completed에 바인딩)
	void AimPressed();
	void AimReleased();

	// Timeline Callbacks 
	UFUNCTION()
	void OnAimTimelineUpdate(float Alpha);

	UFUNCTION()
	void OnAimTimelineFinished();

	// Helpers 
	void StartAimTimelineForward(); // 빠른 줌인
	void StartAimTimelineReverse(); // 느린 줌아웃

	//----------------------
	// Config: Aim Zoom 
	//----------------------
	// 0..1 알파를 만드는 커브(시간 0~1, 값 0~1 권장)
	UPROPERTY(EditDefaultsOnly, Category="Aim|Curve")
	TObjectPtr<UCurveFloat> AimCurve = nullptr;

	// 기본/조준 FOV
	UPROPERTY(EditDefaultsOnly, Category="Aim|Camera")
	float DefaultFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, Category="Aim|Camera")
	float AimFOV = 60.f;

	// 줌 속도(PlayRate로 조절)
	UPROPERTY(EditDefaultsOnly, Category="Aim|Speed")
	float AimInPlayRate = 3.0f; // 누를 때 빠름

	UPROPERTY(EditDefaultsOnly, Category="Aim|Speed")
	float AimOutPlayRate = 1.0f; // 뗄 때 느림
	
private:
	// Timeline 인스턴스(코드 기반 타임라인)
	FTimeline AimTimeline;

	// 내부 상태
	bool bWantsAim = false;
#pragma endregion
#pragma endregion




	



#pragma region FocusMode
protected:
	// Focus Mode
	//포커스 모드 게임0.2배속
	UPROPERTY(EditDefaultsOnly, Category="Focus")
	float FocusModeTimeDilation = 0.2f;

	//일반 속도
	UPROPERTY(EditDefaultsOnly, Category="Focus")
	float NormalModeTimeDilation = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="Focus")
	bool bIsFocusMode = false;
	
	// VFX Fade
	void StartFocusVFXFade(float TargetWeight);
	void StopFocusVFXFade();
	void TickFocusVFXFade();
#pragma endregion
	
#pragma region InputActions
protected:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* MouseLookAction;

	// 달리기, 걷기 토글
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* RunWalkToggleAction;

	//앉기 서기 토글
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* ToggleCrouchAction;
	
	// 해킹
	UPROPERTY(EditAnywhere, Category="Input|Interact")
	UInputAction* HackAction;

	// 상호작용 (줍기)
	UPROPERTY(EditAnywhere, Category="Input|Interact")
	UInputAction* InteractAction;
	
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* ToggleWeaponAction;
	
	// 총 발사
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* FireAction;

	// 조준
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* AimAction;

	//처형
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* TakedownAction;
	
	//포커스모드
	UPROPERTY(EditAnywhere, Category="Input|Ability")
	UInputAction* FocusModeAction;
#pragma endregion

#pragma region GameplayTags
	UPROPERTY(VisibleAnywhere, Category="Tags")
	FGameplayTagContainer ActiveStateTags;

	UPROPERTY(VisibleAnywhere, Category="Tags")
	FGameplayTagContainer BlockTags;

	UPROPERTY(EditDefaultsOnly, Category="Tags")
	TObjectPtr<UTPS_TagRelationshipMap> RelationshipMap;
	
	// 상태 강제 종료(=Cancel)에서 실제 로직도 끊어줘야 한다.
	void CancelStateTag(const FGameplayTag& Tag);
	void RebuildBlocksAndApplyCancels();
	
	// =========================
	// Gameplay Tags - public API (GunComponent -> Character notify)
	// =========================
public:
	// 상태/블락 조회
	bool HasStateTag(const FGameplayTag& StateTag) const;
	bool IsBlockedByTag(const FGameplayTag& BlockTag) const;

	// 상태 변경(내부에서 Rebuild까지 처리)
	void AddStateTag(const FGameplayTag& StateTag);
	void RemoveStateTag(const FGameplayTag& StateTag);

	// GunComponent가 호출하는 통지 함수들
	void NotifyWeaponArmed(bool bArmed);
	void NotifyFirePressed();     // 실제 발사 시작(조건 통과 후) 시점
	void NotifyFireReleased();    // 발사 종료(트리거 뗌/강제 종료) 시점
	void NotifyReloadStarted();
	void NotifyReloadFinished();

#pragma endregion
};
