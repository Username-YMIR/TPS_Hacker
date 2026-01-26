// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	SemiAuto UMETA(DisplayName="Semi Auto"),
	FullAuto UMETA(DisplayName="Full Auto"),
};



/**
 * 
 */
UCLASS(BlueprintType)
class TPS_HACKER_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Identity / Visual ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Identity")
	FName WeaponId = NAME_None; // 이름/ID
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Visual")
	USkeletalMesh* WeaponMesh = nullptr; // 무기 메쉬
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Visual")
	FName AttachSocketName = TEXT("WeaponSocket"); // 캐릭터 손 소켓

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Visual")
	FName MuzzleSocketName = TEXT("Muzzle"); // 총구 소켓(무기 메시)

	// --- Fire config ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto; // 총기 발사 모드

	// FullAuto일 때 초당 발사 수(= RPM/60)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire", meta=(ClampMin="0.1"))
	float FireRate = 10.0f; // 10 rps = 600 RPM

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire", meta=(ClampMin="0.0"))
	float Damage = 20.0f; // 대미지

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire", meta=(ClampMin="0.0"))
	float MaxRange = 10000.0f; // 최대 사거리

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire", meta=(ClampMin="0.0"))
	float SpreadDegrees = 1.0f; // 탄퍼짐

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Fire")
	TSubclassOf<AActor> ProjectileClass; // 프로젝타일 클래스

	// --- Ammo ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Ammo", meta=(ClampMin="1"))
	int32 MagCapacity = 30; // 1회 장전 시 최대 장탄 수 (Magazine Capacity)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Ammo", meta=(ClampMin="0"))
	int32 MaxReserveAmmo = 160; // 총 장탄 수

	// --- Anim ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Anim")
	UAnimMontage* FireMontage = nullptr; // 발사 몽타주 

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Anim")
	UAnimMontage* ReloadMontage = nullptr; // 장전 몽타주

	// --- FX/SFX(옵션) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	UParticleSystem* MuzzleFX = nullptr; // 발사 이펙트 - 총구

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|SFX")
	USoundBase* FireSFX = nullptr; // 발사 이펙트 
};