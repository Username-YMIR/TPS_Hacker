#include "Anim/CharacterAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagAssetInterface.h"
#include "KismetAnimationLibrary.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 초기화 시점에 오너 캐싱
	// (PIE/리스폰/소유권 변경 상황 대비해서 Update에서도 재캐싱 가능)
	CacheOwner();
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 오너가 유효하지 않으면 재캐싱 시도
	// (죽고 리스폰하거나 Pawn이 바뀌는 경우 TryGetPawnOwner가 바뀔 수 있음)
	if (!OwnerCharacter || !MoveComp)
	{
		CacheOwner();
		if (!OwnerCharacter || !MoveComp)
			return; // 오너가 없으면 애님 업데이트 불가
	}

	// 1) 이동/점프 상태 갱신
	UpdateLocomotion(DeltaSeconds);

	// 2) 조준(컨트롤 회전) 기반 AimOffset 입력값 갱신
	UpdateAim(DeltaSeconds);

	// 3) 태그 상태 갱신 (ASC/컨테이너 구현과 무관하게 인터페이스로 통일)
	UpdateGameplayTags();

	// 4) 태그를 ABP 친화적인 bool 상태로 투영
	ProjectStateFromTags();
}

void UCharacterAnimInstance::CacheOwner()
{
	// AnimInstance가 소유한 Pawn을 가져와 캐릭터로 캐스팅
	APawn* PawnOwner = TryGetPawnOwner();
	OwnerCharacter = Cast<ACharacter>(PawnOwner);

	// 이동 상태 조회를 위해 CharacterMovementComponent 캐싱
	MoveComp = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
}

void UCharacterAnimInstance::UpdateLocomotion(float /*DeltaSeconds*/)
{
	// 애니메이션 입력은 대개 "평면 속도"가 필요하므로 Z를 제거한 velocity 사용
	const FVector Vel = OwnerCharacter->GetVelocity();
	const FVector PlanarVel(Vel.X, Vel.Y, 0.f);

	// 이동 속도: BlendSpace, Idle/Move 분기 등 핵심 입력
	Speed = PlanarVel.Size();

	// 미세한 진동으로 Idle/Move가 깜빡이지 않도록 임계값 적용
	bIsMoving = Speed > 3.f;

	// 점프/낙하 판정: Falling이면 공중 상태로 처리
	bIsInAir = MoveComp->IsFalling();

	// crouch 상태는 캐릭터 플래그를 그대로 사용
	bIsCrouched = OwnerCharacter->bIsCrouched;

	// 현재 가속: 입력이 실제로 들어오는지(정지/감속/피벗 등 확장에 사용)
	const FVector Accel = MoveComp->GetCurrentAcceleration();
	bHasAcceleration = Accel.SizeSquared() > 1.f;

	// 방향 값(-180~180): "이동 벡터"가 "캐릭터 바라보는 방향" 기준으로 얼마나 꺾였는지 계산
	// 2D BlendSpace(Forward/Backward/Left/Right) 등에 사용
	Direction = UKismetAnimationLibrary::CalculateDirection(
		PlanarVel,
		OwnerCharacter->GetActorRotation()
	);
}

void UCharacterAnimInstance::UpdateAim(float /*DeltaSeconds*/)
{
	// TPS에서 AimOffset은 보통 "ControlRotation" 기반으로 계산
	// - 캐릭터 회전과는 독립적으로 카메라/에임 방향을 표현할 수 있음
	const FRotator ControlRot = OwnerCharacter->GetBaseAimRotation();
	const FRotator ActorRot = OwnerCharacter->GetActorRotation();

	// Control - Actor 의 차이를 정규화하여 -180~180 범위로 안정적으로 유지
	AimDeltaRot = (ControlRot - ActorRot).GetNormalized();

	// AimOffset Yaw 입력
	AimYaw = AimDeltaRot.Yaw;

	// Pitch는 엔진 내부 표현에 따라 0~360으로 튈 수 있어 보정
	// (에셋이 -90~90을 기대하는 경우가 많아 흔히 사용하는 정규화)
	float Pitch = AimDeltaRot.Pitch;
	if (Pitch > 90.f) Pitch -= 360.f;
	AimPitch = Pitch;
}

void UCharacterAnimInstance::UpdateGameplayTags()
{
	// 매 프레임 최신 상태로 유지하기 위해 초기화 후 채움
	CurrentTags.Reset();

	// 오너가 태그 인터페이스를 구현한 경우:
	// - ASC 기반이든 자체 컨테이너든 공통으로 GetOwnedGameplayTags 제공 가능
	if (const IGameplayTagAssetInterface* TagIf = Cast<IGameplayTagAssetInterface>(OwnerCharacter))
	{
		TagIf->GetOwnedGameplayTags(CurrentTags);
	}

	// 만약 프로젝트가 ASC에만 태그가 있다면:
	// - 여기서 OwnerCharacter의 ASC를 캐싱 후 ASC->GetOwnedGameplayTags로 채우도록 확장 가능
}

bool UCharacterAnimInstance::HasTag(const FGameplayTag& Tag) const
{
	// 태그가 유효할 때만 체크(설정 누락 방어)
	return Tag.IsValid() && CurrentTags.HasTag(Tag);
}

void UCharacterAnimInstance::ProjectStateFromTags()
{
	// ABP 그래프를 단순화하기 위해, 태그 기반 상태를 bool로 투영한다.
	// (ABP에서 HasTag를 반복 호출하면 그래프가 복잡해지고 디버깅 난이도가 상승하기 때문)
	
	bIsArmed     = HasTag(Tag_State_Armed);
	bIsADS       = HasTag(Tag_Combat_ADS);
	bIsFiring    = HasTag(Tag_Combat_Firing);
	bIsReloading = HasTag(Tag_Combat_Reloading);
	bIsSprinting = HasTag(Tag_Move_Sprint);
}
