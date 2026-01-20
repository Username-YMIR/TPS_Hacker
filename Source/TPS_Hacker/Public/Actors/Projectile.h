// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ObjectPoolComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/PoolableInterface.h"
#include "Projectile.generated.h"

UCLASS()
class TPS_HACKER_API AProjectile : public AActor, public IPoolableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

public:
	// 풀 매니저가 Acquire 직호 세팅해주는 소유 풀
	void SetOwningPool(UObjectPoolComponent* InPool);
	
	// 발사 직후 초기 속도, 방향 설정
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Launch(const FVector& Direction, float SpeedOverride = -1.f);
	
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void ReturnToPool();
	
public:
	// IPoolableInterface
	virtual void OnAcquireFromPool_Implementation() override;	// 풀에서 꺼낼 때 호출 (활성화 직전/직후)
	virtual void OnReleaseToPool_Implementation() override;		// 풀로 반납할 때 호출 (비활성화 직전/직후)

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	
	
	
	//충돌 처리
	UFUNCTION()
	void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);
	
	UFUNCTION()
	void BeginOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult
	);
	
	// 수명 타임아웃
	void OnLifeExpired();
	
	// 내부 상태 리셋(풀링용)
	void ResetStateForAcquire();
	void ResetStateForRelease();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> Collision;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> Movement;
	
	// 기본 속도(Launch에서 override 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Config", meta=(ClampMin="0.0"));
	float DefaultSpeed = 3000.f;
	
	// 풀에서 꺼낸 후 자동 반납까지의 시간(0이면 비활성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Config", meta=(ClampMin="0.0"));
	float LifeSeconds = 2.5f;
	
	// 자기 자신 무시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Config")
	bool bIgnoreOwner =true;
	
private:
	//풀 소유자
	UPROPERTY()
	TWeakObjectPtr<UObjectPoolComponent> OwningPool;
	
	// 중복 반납, 중복 히트 방지
	bool bIsReturning = false;
	
	FTimerHandle LifeTimerHandle;
	

};
