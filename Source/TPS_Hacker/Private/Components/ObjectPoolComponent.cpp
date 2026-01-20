// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ObjectPoolComponent.h"

#include "TPS_Hacker.h"
#include "Interfaces/PoolableInterface.h"

// Sets default values for this component's properties
UObjectPoolComponent::UObjectPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UObjectPoolComponent::BeginPlay()
{
	Super::BeginPlay();

	ConfigMap.Empty();
	for (const FPoolBucketConfig& Cfg : Buckets)
	{
		if (Cfg.ActorClass)
		{
			ConfigMap.Add(Cfg.ActorClass, Cfg);
		}
	}
	
	//RuntimeMap 생성
	RuntimeMap.Empty();
	for (const auto& Pair : ConfigMap)
	{
		RuntimeMap.FindOrAdd(Pair.Key);
	}
	PrewarmAll();
}


void UObjectPoolComponent::PrewarmAll()
{
	for (const auto& Pair : ConfigMap)
	{
		const FPoolBucketConfig& Cfg = Pair.Value;
		PrewarmClass(Cfg.ActorClass, Cfg.PrewarmCount);
	}
}

void UObjectPoolComponent::PrewarmClass(TSubclassOf<AActor> ActorClass, int32 Count)
{
	if (!ActorClass || Count <=0)
		return;
	
	if (!EnsureBucket(ActorClass))
		return;
	
	FPoolBucketRuntime& RT = RuntimeMap.FindChecked(ActorClass);
	const FPoolBucketConfig& Cfg = ConfigMap.FindChecked(ActorClass);
	
	const int32 CanCreate = FMath::Max(0,Cfg.MaxCount - RT.TotalCount());
	const int32 ToCreate = FMath::Min(Count, CanCreate);
	
	for (int32 i =0; i < ToCreate; ++i)
	{
		AActor* NewActor = SpawnNew(ActorClass);
		if (!NewActor) break;
		
		SetPooledInactive(NewActor);
		RT.Inactive.Add(NewActor);
		ReverseLookup.Add(NewActor, ActorClass);
	}
}

AActor* UObjectPoolComponent::Acquire(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	UE_LOG(LogTPS, Log, TEXT("UObjectPoolComponent::Acquire()"))
	
	if (!ActorClass)
		return nullptr;
	if (!EnsureBucket(ActorClass))
		return nullptr;
	
	FPoolBucketRuntime& RT = RuntimeMap.FindChecked(ActorClass);
	const FPoolBucketConfig& Cfg = ConfigMap.FindChecked(ActorClass);
	
	// 1. Inactive에서 하나 꺼내기
	AActor* Actor = nullptr;
	while (RT.Inactive.Num() > 0 && !Actor)
	{
		Actor = RT.Inactive.Pop(EAllowShrinking::No); // 남는 공간 축소 불가
		if (!IsValid(Actor))
		{
			Actor = nullptr;
		}
	}
	
	// 2. 없으면 확장
	if (!Actor)
	{
		if (!Cfg.bAllowExpand)
			return nullptr;
		
		//확장 가능한지 MaxCount와 비교 체크
		const int32 CanCreate = Cfg.MaxCount - RT.TotalCount();
		if (CanCreate <=0)
			return nullptr;
		
		const int32 ExpandCount = FMath::Min(Cfg.ExpandStep, CanCreate);
		for (int32 i = 0; i< ExpandCount; ++i)
		{
			AActor* NewActor = SpawnNew(ActorClass);
			if (!NewActor) break;
			
			SetPooledInactive(NewActor);
			RT.Inactive.Add(NewActor);
			ReverseLookup.Add(NewActor, ActorClass);
		}
		
		// 다시 꺼내기
		while (RT.Inactive.Num()>0&&!Actor)
		{
			Actor = RT.Inactive.Pop(EAllowShrinking::No);
			if (!IsValid(Actor)) Actor = nullptr;
		}
		if (!Actor)
			return nullptr;
	}
	
	// 3. 활성화
	SetPooledActive(Actor, SpawnTransform);
	RT.Active.Add(Actor);
	
	return Actor;
}

bool UObjectPoolComponent::Release(AActor* Actor)
{
	if (!IsValid(Actor))
		return false;
	
	TSubclassOf<AActor>* FoundClass = ReverseLookup.Find(Actor);
	if (!FoundClass ||!(*FoundClass))
		return false;
	
	TSubclassOf<AActor> ActorClass = *FoundClass;
	if (!EnsureBucket(ActorClass))
		return false;
	
	FPoolBucketRuntime& RT = RuntimeMap. FindChecked(ActorClass);
	
	// Active에 없으면 이중 반납 방지
	if (!RT.Active.Contains(Actor))
		return false;
	
	//훅(선택)
	if (Actor->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass()))
	{
		IPoolableInterface::Execute_OnReleaseToPool(Actor);
	}
	
	RT.Active.Remove(Actor);
	
	SetPooledInactive(Actor);
	RT.Inactive.Add(Actor);
	
	return true;
}

void UObjectPoolComponent::GetPoolStats(TSubclassOf<AActor> ActorClass, int32& OutTotal, int32& OutActive, int32& OutInactive) const
{
	OutTotal = OutActive = OutInactive = 0;
	
	if (!ActorClass)
		return;
	
	const FPoolBucketRuntime* RT = RuntimeMap.Find(ActorClass);
	if (!RT) return;
	
	OutTotal = RT->TotalCount();
	OutActive = RT->ActiveCount();
	OutInactive = RT->InactiveCount();
}

bool UObjectPoolComponent::EnsureBucket(TSubclassOf<AActor> ActorClass)
{
	if (!ActorClass)
		return false;
	
	// Config가 없으면 런타임 등록을 막는 정책(권장)
	// 필요하면 여기서 기본값으로 자동 등록하도록 바꿀 수 있음
	if (!ConfigMap.Contains(ActorClass))
	{
		return false;
	}
	
	RuntimeMap.FindOrAdd(ActorClass);
	return true;
}

AActor* UObjectPoolComponent::SpawnNew(TSubclassOf<AActor> ActorClass)
{
	if (!GetWorld()||!ActorClass)
		return nullptr;
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	// 처음 생성은 원점에
	return GetWorld()->SpawnActor<AActor>(ActorClass, FTransform::Identity, Params);
}

void UObjectPoolComponent::SetPooledInactive(AActor* Actor)
{
	if (!IsValid(Actor))
		return;

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// 필요하면 위치를 멀리 보내는 것도 가능하지만, 숨김/콜리전 off면 보통 충분
	// Actor->SetActorLocation(FVector(0,0,-100000.f));
}

void UObjectPoolComponent::SetPooledActive(AActor* Actor, const FTransform& XForm)
{
	if (!IsValid(Actor))
		return;
	
	Actor->SetActorTransform(XForm);
	
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);
	
	// 인터페이스를 구현한 액터 초기화 훅
	if (Actor->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass()))
	{
		UE_LOG(LogTPS, Warning, TEXT("UObjectPoolComponent::SetPooledActive()"));
		IPoolableInterface::Execute_OnAcquireFromPool(Actor);
	}
}
