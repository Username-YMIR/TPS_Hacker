#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "CharacterAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/**
 * 캐릭터 애니메이션을 위한 AnimInstance
 *
 * [설계 의도]
 * - AnimInstance는 "게임플레이 로직 실행"이 아니라, 애니메이션에 필요한 "상태값 계산/캐싱"만 담당한다.
 * - 애니메이션 표현(블렌드/스테이트머신/레이어/슬롯/몽타주)은 AnimBP(BP)에서 처리한다.
 * - Gameplay Tag 기반 상태(무장/ADS/발사/장전/스프린트 등)를 읽어 ABP가 쓰기 쉬운 bool로 투영한다.
 */
UCLASS()
class TPS_HACKER_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** AnimInstance 초기화 시점: 오너/무브먼트 컴포넌트 캐싱 */
	virtual void NativeInitializeAnimation() override;

	/**
	 * 매 프레임 애니메이션 업데이트
	 * - 캐릭터의 이동/조준/태그 상태를 갱신하고 ABP 변수로 제공
	 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// =========================
	// Cached References
	// =========================

	/** 현재 이 AnimInstance를 소유한 캐릭터 (리스폰/소유 변경 대비하여 매 프레임 유효성 체크 가능) */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Anim|Cache")
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	/** 이동 상태값을 얻기 위해 캐싱해둔 CharacterMovementComponent */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Anim|Cache")
	TObjectPtr<UCharacterMovementComponent> MoveComp = nullptr;

	// =========================
	// Locomotion (BlendSpace / StateMachine 입력)
	// =========================

	/** 평면 이동 속도(2D): Idle/Move 블렌딩 및 이동 관련 스테이트 전환에 사용 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Locomotion")
	float Speed = 0.f;
	
	/** 이동 방향(-180~180): 2D BlendSpace에서 방향 전환에 사용 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Locomotion")
	float Direction = 0.f;

	/** 미세한 흔들림으로 Idle<->Move가 토글되지 않도록 Speed 기준으로 판정 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Locomotion")
	bool bIsMoving = false;

	/** 가속 여부: start/stop, pivot 같은 애니메이션 분기(추후 확장) */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Locomotion")
	bool bHasAcceleration = false;

	/** 공중 상태: Jump/Fall/Land 스테이트머신 전환에 사용 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Locomotion")
	bool bIsInAir = false;

	/** 웅크리기 여부(있을 경우): crouch locomotion 분기용 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Locomotion")
	bool bIsCrouched = false;

	// =========================
	// Aim / Rotation (AimOffset 입력)
	// =========================

	/** 컨트롤 회전과 액터 회전의 Delta(Yaw): 조준 시 상체 회전/에임오프셋에 사용 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Aim")
	float AimYaw = 0.f;

	/** 컨트롤 회전과 액터 회전의 Delta(Pitch): AimOffset Pitch 입력 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Aim")
	float AimPitch = 0.f;

	/**
	 * ControlRot - ActorRot을 정규화한 값
	 * - 디버깅 및 후처리(예: TurnInPlace, Strafe Offset) 확장에 사용 가능
	 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Aim")
	FRotator AimDeltaRot = FRotator::ZeroRotator;

	// =========================
	// Gameplay Tags (State)
	// =========================

	/**
	 * 현재 프레임의 태그 컨테이너
	 * - IGameplayTagAssetInterface를 통해 오너가 소유한 태그를 가져온다.
	 * - ASC를 쓰든 자체 컨테이너를 쓰든 인터페이스만 만족하면 공통 처리 가능
	 */
	UPROPERTY(BlueprintReadOnly, Category="Anim|Tags")
	FGameplayTagContainer CurrentTags;

	// ---- ABP 사용 편의성을 위한 "투영값" ----
	// ABP에서 태그를 직접 파싱하면 그래프가 복잡해지므로, bool로 단순화한다.

	UPROPERTY(BlueprintReadOnly, Category="Anim|State")
	bool bIsArmed = false;

	UPROPERTY(BlueprintReadOnly, Category="Anim|State")
	bool bIsADS = false;

	UPROPERTY(BlueprintReadOnly, Category="Anim|State")
	bool bIsFiring = false;

	UPROPERTY(BlueprintReadOnly, Category="Anim|State")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category="Anim|State")
	bool bIsSprinting = false;

protected:
	// =========================
	// Internal Update Steps
	// =========================

	/** TryGetPawnOwner 기반으로 OwnerCharacter/MoveComp를 캐싱 */
	void CacheOwner();

	/** 이동 관련 상태값 갱신(Speed, Direction, InAir 등) */
	void UpdateLocomotion(float DeltaSeconds);

	/** 조준 관련 상태값 갱신(AimYaw, AimPitch 등) */
	void UpdateAim(float DeltaSeconds);

	/** 오너가 소유한 Gameplay Tags를 CurrentTags에 갱신 */
	void UpdateGameplayTags();

	/** CurrentTags를 기반으로 ABP용 bool 상태로 투영 */
	void ProjectStateFromTags();

	/** 태그 존재 여부 헬퍼 */
	bool HasTag(const FGameplayTag& Tag) const;

	// =========================
	// Tag Config (프로젝트 태그 네이밍에 맞춰 지정)
	// =========================
	// [의도] 코드에 하드코딩 문자열을 박지 않고, 에디터에서 태그를 지정할 수 있게 구성

	UPROPERTY(EditDefaultsOnly, Category="Anim|Tags|Config")
	FGameplayTag Tag_State_Armed;

	UPROPERTY(EditDefaultsOnly, Category="Anim|Tags|Config")
	FGameplayTag Tag_Combat_ADS;

	UPROPERTY(EditDefaultsOnly, Category="Anim|Tags|Config")
	FGameplayTag Tag_Combat_Firing;

	UPROPERTY(EditDefaultsOnly, Category="Anim|Tags|Config")
	FGameplayTag Tag_Combat_Reloading;

	UPROPERTY(EditDefaultsOnly, Category="Anim|Tags|Config")
	FGameplayTag Tag_Move_Sprint;
};
