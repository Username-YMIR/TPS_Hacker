// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Projectile.h"

#include "TPS_Hacker.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	
	// Collision 기본 설정
	Collision->InitSphereRadius(6.0f);
	Collision->SetCollisionProfileName("Projectile");
	
	// Collision->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::BeginOverlap);
	
	// 무브먼트 기본 설정
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision; // 실제 이동할 컴포넌트(콜리전)
	Movement->InitialSpeed = DefaultSpeed;
	Movement->MaxSpeed = DefaultSpeed;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.0f;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}


void AProjectile::SetOwningPool(UObjectPoolComponent* InPool)
{
	OwningPool = InPool;
}

void AProjectile::Launch(const FVector& Direction, float SpeedOverride) //SpeedOverride = -1.0f
{
	
	if (!Movement)
		return;
	
	const float Speed = (SpeedOverride > 0.f) ? SpeedOverride : DefaultSpeed;
	
	// 방향 정규화
	FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Dir = GetActorForwardVector();
	}
	
	Movement->Velocity = Dir*Speed;
	
	Movement->UpdateComponentVelocity();
	
	UE_LOG(LogTPS, Log, TEXT("AProjectile::Launch()"));
	
}

void AProjectile::ReturnToPool()
{
	if (bIsReturning)
		return;
	
	bIsReturning = true;
	
	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifeTimerHandle);
	}
	
	// 풀로 반납
	if (UObjectPoolComponent* Pool = OwningPool.Get())
	{
		Pool->Release(this);
	}
	else
	{
		// 소유자가 없으면 안전하게 비활성화
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
	}
}


void AProjectile::OnAcquireFromPool_Implementation()
{
	UE_LOG(LogTPS, Warning, TEXT("AProjectile::OnAcquireFromPool_Implementation()"))
	ResetStateForAcquire();
	
	// 수명 타이머
	if (LifeSeconds > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(LifeTimerHandle, this, &AProjectile::OnLifeExpired, LifeSeconds, false);
		}
	}
	
	
}

void AProjectile::OnReleaseToPool_Implementation()
{
	UE_LOG(LogTPS, Warning, TEXT("AProjectile::OnReleaseToPool_Implementation()"))
	ResetStateForRelease();
}

void AProjectile::ResetStateForAcquire()
{
	bIsReturning = false;
	
	if (bIgnoreOwner)
	{
		if (AActor* MyOwner = GetOwner())
		{
			Collision->IgnoreActorWhenMoving(MyOwner, true);
		}
	}
	
	//Movement 초기화
	if (Movement)
	{
		Movement->StopMovementImmediately();
		Movement->Velocity = FVector::ZeroVector;
		Movement->ProjectileGravityScale = 0.f;
	}
}


void AProjectile::ResetStateForRelease()
{
	// 수명 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifeTimerHandle);
	}
	
	// 이동 완전 정지
	if (Movement)
	{
		Movement->StopMovementImmediately();
		Movement->Velocity  = FVector::ZeroVector;
	}
	
	//Ignore 해제 (재사용 시 누적 방지)
	if (bIgnoreOwner)
	{
		if (AActor* MyOwner = GetOwner())
		{
			Collision->IgnoreActorWhenMoving(MyOwner, false);
		}
	}
	
	
	// 다음 Acquire에서 다시 세팅될 수 있도록
	// (OwningPool은 유지하거나 발사체마다 다른 경우 Acquire 직후에 다시 SetOwningPool 함)
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTPS, Warning, TEXT("OnHit()"));
	if (bIsReturning)
		return;

	// 자기 자신일 경우
	if (!OtherActor || OtherActor == this)
		return;

	//Owner일 경우
	if (bIgnoreOwner && OtherActor == GetOwner())
		return;

	// TODO: 데미지/이펙트는 여기서 처리
	// UGameplayStatics::ApplyDamage(...);

	ReturnToPool();
}


void AProjectile::BeginOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult
	)
{
	UE_LOG(LogTPS, Warning, TEXT("AProjectile::BeginOverlap()"));
	if (bIsReturning)
		return;

	// 자기 자신일 경우
	if (!OtherActor || OtherActor == this)
		return;

	//Owner일 경우
	if (bIgnoreOwner && OtherActor == GetOwner())
		return;

	// TODO: 데미지/이펙트는 여기서 처리
	// UGameplayStatics::ApplyDamage(...);

	ReturnToPool();
}

void AProjectile::OnLifeExpired()
{
	ReturnToPool();
}
