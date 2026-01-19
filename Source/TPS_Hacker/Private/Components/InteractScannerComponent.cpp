// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/InteractScannerComponent.h"

#include "Engine/OverlapResult.h"
#include "Interfaces/InteractableInterface.h"

// 근거리 상호작용(E) 대상 스캐너: Overlap + 거리 기준으로 가장 가까운 후보를 선택
UInteractScannerComponent::UInteractScannerComponent()
{
	// 타이머 기반 반복 스캔이라 Tick은 꺼둠
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractScannerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 설정에 따라 시작 시 자동 스캔
	if (bAutoStart)
	{
		StartScan();
	}
}

void UInteractScannerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 시 타이머 정리(레벨 전환/파괴 시 안전)
	StopScan();

	Super::EndPlay(EndPlayReason);
}

void UInteractScannerComponent::StartScan()
{
	if (!GetWorld())
		return;

	// 중복 실행 방지
	if (GetWorld()->GetTimerManager().IsTimerActive(ScanTimerHandle))
		return;

	// 즉시 1회 스캔 후 주기적으로 반복
	ScanOnce();
	GetWorld()->GetTimerManager().SetTimer(
		ScanTimerHandle,
		this,
		&UInteractScannerComponent::ScanOnce,
		ScanInterval,
		true
	);
}

void UInteractScannerComponent::StopScan()
{
	if (!GetWorld())
		return;

	// 반복 스캔 중지
	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
}

FVector UInteractScannerComponent::GetScanOrigin() const
{
	// 근거리 상호작용은 보통 Owner 위치 기준(필요하면 카메라/캡슐 위치로 바꿀 수 있음)
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

bool UInteractScannerComponent::IsValidCloseCandidate(AActor* Candidate) const
{
	if (!Candidate)
		return false;

	// 상호작용 인터페이스가 없는 액터는 제외
	if (!Candidate->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		return false;

	// 실제 가능 여부는 대상 액터가 판단(Owner를 Interactor로 전달)
	AActor* Owner = GetOwner();
	return IInteractableInterface::Execute_CanInteract(Candidate, Owner);
}

void UInteractScannerComponent::ScanOnce()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
		return;

	const FVector Origin = GetScanOrigin();

	// 오브젝트 채널 기반 오버랩(구체적인 채널로 범위를 제한하는 용도)
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(OverlapObjectChannel);

	// Owner는 검사에서 제외(자기 자신 잡히는 것 방지)
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CloseInteractScan), false, Owner);

	TArray<FOverlapResult> Overlaps;
	const bool bOverlapped = World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(SphereRadius),
		QueryParams
	);

	// 후보 중 가장 가까운 타겟을 선택
	AActor* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	if (bOverlapped)
	{
		for (const FOverlapResult& Res : Overlaps)
		{
			AActor* Candidate = Res.GetActor();
			if (!IsValidCloseCandidate(Candidate))
				continue;

			// 거리 비교는 sqrt 안 쓰려고 DistSquared 사용
			// 거리의 멀고 가까운지만 비교해서 비용절감
			const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());

			// SphereRadius로 걸러졌더라도 MaxRange 정책을 따로 유지하고 싶을 때 여기서 한번 더 컷
			if (DistSq > FMath::Square(MaxRange))
				continue;

			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = Candidate;
			}
		}
	}

	// 타겟 변경 처리(이벤트 브로드캐스트 포함)
	SetCurrentTarget(BestTarget);

	// 디버그 시각화
	if (bDebugDraw)
	{
		DrawDebugSphere(World, Origin, SphereRadius, 16, FColor::Cyan, false, ScanInterval, 0, 1.0f);

		if (CurrentTarget)
		{
			// BestTarget이 없으면 BestDistSq는 FLT_MAX라서, CurrentTarget 있을 때만 사용
			const float Dist = FMath::Sqrt(BestDistSq);
			const FString Text = FString::Printf(TEXT("E Target: %s (%.0f)"), *CurrentTarget->GetName(), Dist);

			DrawDebugString(
				World,
				CurrentTarget->GetActorLocation() + FVector(0, 0, 80.f),
				Text,
				nullptr,
				FColor::White,
				ScanInterval,
				true
			);
		}
	}
}

void UInteractScannerComponent::SetCurrentTarget(AActor* NewTarget)
{
	// 동일 타겟이면 불필요한 이벤트 방지
	if (NewTarget == CurrentTarget)
		return;

	AActor* Old = CurrentTarget;
	CurrentTarget = NewTarget;

	// UI/하이라이트/사운드 등 외부 반응용
	OnTargetChanged.Broadcast(Old, CurrentTarget);
}
