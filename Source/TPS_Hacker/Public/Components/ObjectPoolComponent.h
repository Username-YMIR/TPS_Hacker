// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectPoolComponent.generated.h"

USTRUCT(BlueprintType)
struct FPoolBucketConfig
{
	GENERATED_BODY()
	
	// 어떤 액터 클래스를 풀링할지
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AActor> ActorClass;
	
	// 시작 시 미리 만들어둘 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 PrewarmCount = 10;
	
	// 모자라면 자동 확장할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAllowExpand = true;
	
	// 확장 시 한 번에 몇 개 더 만들지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin="1"))
	int32 ExpandStep = 5;
	
	//풀 최대 보유 개수 (확장 포함)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin = "1"))
	int32 MaxCount = 200;
	
	// 풀에 들어간 액터는 Hidden/Collision/Tick을 끔
	
};

USTRUCT()
struct FPoolBucketRuntime
{
	GENERATED_BODY()
	
	// 저비용 Array 구조
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Inactive;

	// 디버깅 유리한 TSet
	UPROPERTY()
	TSet<TObjectPtr<AActor>> Active;
	
	// 통계/디버그
	int32 TotalCount() const {return Inactive.Num()+Active.Num();}
	int32 ActiveCount() const {return Active.Num();}
	int32 InactiveCount() const {return Inactive.Num();}
	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPS_HACKER_API UObjectPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UObjectPoolComponent();
	
public:
	// 에디터에서 버킷 설정
	UPROPERTY(EditAnywhere, Category="Pool|Config")
	TArray<FPoolBucketConfig> Buckets;
	
	//풀에서 꺼내기
	UFUNCTION(BlueprintCallable, Category = "Pool")
	AActor* Acquire(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);
	
	// 풀로 반납
	UFUNCTION(BlueprintCallable, Category = "Pool")
	bool Release(AActor* Actor);
	
	// 시작 시 미리 준비 (Cold Start 방지)
	UFUNCTION(BlueprintCallable, Category = "Pool")
	void PrewarmAll();
	
	// 특정 클래스만 Prewarm
	UFUNCTION(BlueprintCallable, Category = "Pool")
	void PrewarmClass(TSubclassOf<AActor> ActorClass, int32 Count);
	
	// 디버그용: 현재 상태
	UFUNCTION(BlueprintCallable, Category="Pool")
	void GetPoolStats(TSubclassOf<AActor> ActorClass, int32& OutTotal, int32& OutActive, int32& OutInactive) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// Config 빠르게 찾기
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FPoolBucketConfig> ConfigMap;
	
	//Runtime 상태
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FPoolBucketRuntime> RuntimeMap;
	
	// 액터 -> 클래스 역추적 (Release 시 버킷 찾기)
	UPROPERTY()
	TMap<TObjectPtr<AActor>, TSubclassOf<AActor>> ReverseLookup;
	
private:
	bool EnsureBucket(TSubclassOf<AActor> ActorClass);				// 해당 클래스용 풀(버킷)이 준비되어 있는지 보장한다
	AActor* SpawnNew(TSubclassOf<AActor> ActorClass);				// 풀에 새 인스턴스를 만든다(보통 버킷이 비었을 때만 호출)
	void SetPooledInactive(AActor* Actor);							// 풀에 반환될 때 호출: 액터를 "비활성(보관)" 상태로 만든다
	void SetPooledActive(AActor* Actor, const FTransform& XForm);	// 풀에서 꺼낼 때 호출: 액터를 "활성(사용)" 상태로 만든다
};
