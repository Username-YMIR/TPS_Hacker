// GunComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/WeaponDataAsset.h"
#include "GunComponent.generated.h"

UENUM(BlueprintType)
enum class EEquipState : uint8
{
	Unarmed UMETA(DisplayName="Unarmed"),
	Armed   UMETA(DisplayName="Armed"),
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class TPS_HACKER_API UGunComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGunComponent();

	virtual void BeginPlay() override;

	// 캐릭터 입력에서 호출
	void RequestEquipPrimary();
	void RequestUnequip();

	void RequestFirePressed();
	void RequestFireReleased();

	void RequestReload();

	// UI 바인딩용
	UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
	int32 GetMagAmmo() const { return CurrentAmmoInMag; } // 현재 장탄수

	UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
	int32 GetReserveAmmo() const { return ReserveAmmo; } // 전체 장탄수

	UFUNCTION(BlueprintCallable, Category="Weapon|State")
	bool IsArmed() const { return EquipState == EEquipState::Armed; } // 장착 여부

private:
	// 내부 실행
	bool TryEquip(); //
	bool CanFire() const; // 
	bool CanReload() const;

	void FireOnce();
	void StartAutoFire();
	void StopAutoFire();

	void BeginReload();
	void FinishReload();

	// 발사 구현(프로젝트에서 이미 쓰는 "풀링 + 프로젝타일 Launch" 유지)
	void SpawnAndLaunchProjectile(const FVector& Dir, const FTransform& SpawnXForm);

	// Owner 접근 헬퍼
	class ATPS_Hacker_Character* GetOwnerCharacter() const;

private:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComp = nullptr;
	
	// 장착할 기본 무기 데이터(권총/라이플 DataAsset 지정)
	UPROPERTY(EditAnywhere, Category="Weapon|Loadout")
	TObjectPtr<UWeaponDataAsset> PrimaryWeaponData = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category="Weapon|Runtime")
	TObjectPtr<UWeaponDataAsset> CurrentWeaponData = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category="Weapon|State")
	EEquipState EquipState = EEquipState::Unarmed;

	// 입력 상태
	UPROPERTY(VisibleInstanceOnly, Category="Weapon|State")
	bool bWantsToFire = false;

	UPROPERTY(VisibleInstanceOnly, Category="Weapon|State")
	bool bIsReloading = false;

	// 탄약
	UPROPERTY(VisibleInstanceOnly, Category="Weapon|Ammo")
	int32 CurrentAmmoInMag = 0;

	UPROPERTY(VisibleInstanceOnly, Category="Weapon|Ammo")
	int32 ReserveAmmo = 0;

	// 타이머
	FTimerHandle TimerHandle_AutoFire;
	FTimerHandle TimerHandle_Reload;
};
