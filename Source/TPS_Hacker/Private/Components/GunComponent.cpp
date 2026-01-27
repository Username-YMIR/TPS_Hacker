// GunComponent.cpp

#include "Components/GunComponent.h"

#include "Characters/TPS_Hacker_Character.h"
#include "Components/ObjectPoolComponent.h"
#include "Actors/Projectile.h"

#include "TPS_HackerPlayerController.h"
#include "Kismet/GameplayStatics.h"

UGunComponent::UGunComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGunComponent::BeginPlay()
{
	Super::BeginPlay();

	ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar) return;

	USkeletalMeshComponent* OwnerMesh = OwnerChar->GetMesh();
	if (!OwnerMesh) return;

	WeaponMeshComp = NewObject<USkeletalMeshComponent>(OwnerChar, TEXT("WeaponMeshComp"));
	WeaponMeshComp->RegisterComponent();
	WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComp->SetGenerateOverlapEvents(false);

	// 캐릭터에 붙여서 월드에 존재하게 만들기
	WeaponMeshComp->AttachToComponent(OwnerMesh, FAttachmentTransformRules::KeepRelativeTransform);
	WeaponMeshComp->SetHiddenInGame(true);
}

ATPS_Hacker_Character* UGunComponent::GetOwnerCharacter() const
{
	return Cast<ATPS_Hacker_Character>(GetOwner());
}

void UGunComponent::RequestEquipPrimary()
{
	TryEquip();
}

void UGunComponent::RequestUnequip()
{
	// 이미 비무장이면 종료
	if (EquipState == EEquipState::Unarmed)
		return;

	// 1) 진행 중 액션 중단 (연사/장전)
	StopAutoFire();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_AutoFire);
		World->GetTimerManager().ClearTimer(TimerHandle_Reload);
	}

	// 2) 입력/상태 플래그 리셋
	bWantsToFire = false;
	bIsReloading = false;

	// 3) 무기 메시 처리: 손에서 떼고 숨김
	if (WeaponMeshComp)
	{
		// 손에서 분리 (월드에 남아있되 부모에서 분리)
		WeaponMeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// 숨김(렌더/틱 부담 최소화)
		WeaponMeshComp->SetHiddenInGame(true);

		// 충돌 꺼두기(혹시 켜져 있으면)
		WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMeshComp->SetGenerateOverlapEvents(false);

		// 완전 비우기: 메시 레퍼런스 제거
		//  - 장착할 때 다시 SetSkeletalMesh 하므로, 여기서 비워도 됨.
		WeaponMeshComp->SetSkeletalMesh(nullptr);
	}

	// 4) 무기 데이터/상태 정리
	CurrentWeaponData = nullptr;
	EquipState = EEquipState::Unarmed;
	
	if (ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter())
	{
		OwnerChar->NotifyWeaponArmed(false);
	}

}

bool UGunComponent::TryEquip()
{
	if (EquipState == EEquipState::Armed) return true;
	if (!PrimaryWeaponData) return false;

	ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar || !WeaponMeshComp) return false;

	CurrentWeaponData = PrimaryWeaponData;
	EquipState = EEquipState::Armed;
	// 무장 태그 통지
	OwnerChar->NotifyWeaponArmed(true);


	// 무기 메시 적용
	WeaponMeshComp->SetSkeletalMesh(CurrentWeaponData->WeaponMesh);
	WeaponMeshComp->SetHiddenInGame(false);

	// 손 소켓에 부착
	const FName Socket = CurrentWeaponData->AttachSocketName; // 예: "WeaponSocket"
	WeaponMeshComp->AttachToComponent(
		OwnerChar->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		Socket
	);

	// 탄약 초기화(정책에 따라)
	CurrentAmmoInMag = CurrentWeaponData->MagCapacity;
	ReserveAmmo = CurrentWeaponData->MaxReserveAmmo;

	return true;
}

void UGunComponent::RequestFirePressed()
{
	bWantsToFire = true;

	// 비무장 상태에서 좌클릭 => 장착 시도
	if (EquipState == EEquipState::Unarmed)
	{
		TryEquip();
		return;
	}

	// 장전 중엔 발사 불가
	if (bIsReloading)
		return;

	// 탄창 0이면 장전 시도
	if (CurrentAmmoInMag <= 0)
	{
		RequestReload();
		return;
	}

	// SemiAuto/FullAuto 분기 (DataAsset 기반)
	if (ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter())
	{
		OwnerChar->NotifyFirePressed();
	}

	if (!CurrentWeaponData)
		return;

	if (CurrentWeaponData->FireMode == EWeaponFireMode::SemiAuto)
	{
		FireOnce();
	}
	else
	{
		StartAutoFire();
	}
}

void UGunComponent::RequestFireReleased()
{
	bWantsToFire = false;
	StopAutoFire();
	if (ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter())
	{
		OwnerChar->NotifyFireReleased();
	}
}

bool UGunComponent::CanFire() const
{
	if (EquipState != EEquipState::Armed) return false;
	if (!CurrentWeaponData) return false;
	if (bIsReloading) return false;
	if (CurrentAmmoInMag <= 0) return false;
	return true;
}

void UGunComponent::StartAutoFire()
{
	if (!CanFire())
		return;

	// 즉시 1발
	FireOnce();

	// 이후 반복
	const float Interval = 1.0f / FMath::Max(0.1f, CurrentWeaponData->FireRate);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AutoFire,
		this,
		&UGunComponent::FireOnce,
		Interval,
		true
	);
}

void UGunComponent::StopAutoFire()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);
}

void UGunComponent::FireOnce()
{
	if (!CanFire())
	{
		StopAutoFire();

		// 연사 중 탄이 떨어졌으면 자동 장전 시도
		if (EquipState == EEquipState::Armed && CurrentAmmoInMag <= 0)
			RequestReload();

		return;
	}

	ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar)
		return;

	// 1) 조준점 기준 AimPoint 계산: 기존 Do_Fire() 로직 유지
	ATPS_HackerPlayerController* PC = Cast<ATPS_HackerPlayerController>(OwnerChar->GetController());
	if (!PC)
		return;

	constexpr float Range = 100000.f;

	FVector AimPoint;
	if (!PC->GetAimHitPoint(AimPoint, Range, ECC_Visibility, /*bDrawDebug*/ false))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		AimPoint = CamLoc + CamRot.Vector() * Range;
	}

	if (!WeaponMeshComp || !CurrentWeaponData) return;

	const FName MuzzleSocket = CurrentWeaponData->MuzzleSocketName;

	// 총구 위치/회전
	const FVector MuzzleLoc = WeaponMeshComp->DoesSocketExist(MuzzleSocket)
		? WeaponMeshComp->GetSocketLocation(MuzzleSocket)
		: WeaponMeshComp->GetComponentLocation();

	const FRotator MuzzleRot = WeaponMeshComp->DoesSocketExist(MuzzleSocket)
		? WeaponMeshComp->GetSocketRotation(MuzzleSocket)
		: WeaponMeshComp->GetComponentRotation();

	// AimPoint 기준으로 날릴 방향(총구 위치 기준)
	const FVector Dir = (AimPoint - MuzzleLoc).GetSafeNormal();

	// 스폰 회전은 2가지 중 택1
	// (A) AimPoint로 정확히 날림 (추천: 히트스캔/투사체 공통)
	const FRotator SpawnRot = Dir.Rotation();

	// (B) “총구가 보는 방향” 그대로 (비주얼/반동 느낌, 대신 조준 오차 가능)
	// const FRotator SpawnRot = MuzzleRot;
	
	// 4) 탄약 차감
	CurrentAmmoInMag--;

	const FTransform SpawnXForm(SpawnRot, MuzzleLoc);
	SpawnAndLaunchProjectile(Dir, SpawnXForm);




	// // 5) 스폰/런치
	// SpawnAndLaunchProjectile(Dir, SpawnXForm);

	// 6) (옵션) 발사 몽타주/이펙트/사운드
	// - 몽타주: OwnerChar->PlayAnimMontage(CurrentWeaponData->FireMontage);
	// - FX/SFX: UGameplayStatics::SpawnEmitterAtLocation / PlaySoundAtLocation
}

void UGunComponent::SpawnAndLaunchProjectile(const FVector& Dir, const FTransform& SpawnXForm)
{
	ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar)
		return;

	UObjectPoolComponent* PoolComp = OwnerChar->GetPoolComponent();
	if (!PoolComp)
		return;

	// ProjectileClass는 DataAsset 우선, 없으면 캐릭터 임시 변수 fallback(마이그레이션 단계 안정성)
	TSubclassOf<AActor> ProjectileClass = nullptr;
	if (CurrentWeaponData)
		ProjectileClass = CurrentWeaponData->ProjectileClass;

	if (!ProjectileClass)
		ProjectileClass = OwnerChar->GetFallbackProjectileClass();

	if (!ProjectileClass)
		return;

	AActor* Spawned = PoolComp->Acquire(ProjectileClass, SpawnXForm);
	AProjectile* Proj = Cast<AProjectile>(Spawned);
	if (!Proj)
		return;

	Proj->SetOwner(OwnerChar);
	Proj->SetOwningPool(PoolComp);

	Proj->OwnerCollisionChannel = OwnerChar->GetOwnerCollisionChannel();
	Proj->ApplyProjectileCollision();

	Proj->Launch(Dir);
}

void UGunComponent::RequestReload()
{
	if (!CanReload())
		return;

	BeginReload();
}

bool UGunComponent::CanReload() const
{
	if (EquipState != EEquipState::Armed) return false;
	if (!CurrentWeaponData) return false;
	if (bIsReloading) return false;
	if (ReserveAmmo <= 0) return false;
	if (CurrentAmmoInMag >= CurrentWeaponData->MagCapacity) return false;
	return true;
}

void UGunComponent::BeginReload()
{
	bIsReloading = true;
	if (ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter())
	{
		OwnerChar->NotifyReloadStarted();
	}
	StopAutoFire();
	

	// 지금은 “타이머로 즉시 장전 완료” 형태로 최소 구현
	// 나중에 ReloadMontage 종료 타이밍에서 FinishReload 호출로 교체
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_Reload,
		this,
		&UGunComponent::FinishReload,
		0.1f,
		false
	);
}

void UGunComponent::FinishReload()
{
	if (!CurrentWeaponData)
	{
		bIsReloading = false;
		return;
	}

	const int32 Need = CurrentWeaponData->MagCapacity - CurrentAmmoInMag;
	const int32 Load = FMath::Min(Need, ReserveAmmo);

	CurrentAmmoInMag += Load;
	ReserveAmmo -= Load;

	bIsReloading = false;
	if (ATPS_Hacker_Character* OwnerChar = GetOwnerCharacter())
	{
		OwnerChar->NotifyReloadFinished();
	}

	// 자동화: 연사키를 계속 누른 상태면 재발사
	if (bWantsToFire && CurrentWeaponData->FireMode == EWeaponFireMode::FullAuto)
		StartAutoFire();
}
