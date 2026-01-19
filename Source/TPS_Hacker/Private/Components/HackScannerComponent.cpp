// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/HackScannerComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

#include "Interfaces/HackableInterface.h"

UHackScannerComponent::UHackScannerComponent()
{
	// 타이머 기반이라 Tick은 필요 없음
	PrimaryComponentTick.bCanEverTick = false;
}

void UHackScannerComponent::Initialize(UCameraComponent* InCamera)
{
	// 카메라 기준으로 트레이스를 쏘기 위해 참조 저장
	CameraRef = InCamera;
}

void UHackScannerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 설정에 따라 시작과 동시에 스캔 루프를 돌림
	if (bAutoStart)
	{
		StartScan();
	}
}

void UHackScannerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 시 타이머 정리
	// (레벨 전환/파괴 등에서 누수 방지)
	StopScan();
	Super::EndPlay(EndPlayReason);
}

void UHackScannerComponent::StartScan()
{
	// 타이머를 걸기 위해 월드 확인
	// (에디터/종료 타이밍)
	if (!GetWorld())
		return;

	// 중복으로 타이머가 잡히는 것 방지
	if (GetWorld()->GetTimerManager().IsTimerActive(ScanTimerHandle))
		return;

	// 즉시 1회 스캔 후, 주기적으로 반복
	ScanOnce();
	GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UHackScannerComponent::ScanOnce, ScanInterval, true);
}

void UHackScannerComponent::StopScan()
{
	if (!GetWorld())
		return;

	// 반복 스캔 중지
	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
}

void UHackScannerComponent::GetViewPoint(FVector& OutLoc, FRotator& OutRot) const
{
	// 카메라가 있으면 카메라 기준으로 스캔(보통 1인칭/3인칭 공용 처리)
	if (CameraRef)
	{
		OutLoc = CameraRef->GetComponentLocation();
		OutRot = CameraRef->GetComponentRotation();
		return;
	}

	// 카메라가 없으면 Owner 기준으로 처리(세팅 누락 대비)
	const AActor* Owner = GetOwner();
	OutLoc = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	OutRot = Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;
}

bool UHackScannerComponent::IsValidHackCandidate(AActor* Candidate) const
{
	// 후보 자체가 없으면 제외
	if (!Candidate)
		return false;

	// 해킹 인터페이스를 구현하지 않으면 제외
	if (!Candidate->GetClass()->ImplementsInterface(UHackableInterface::StaticClass()))
		return false;

	// 실제 가능 여부는 대상이 판단(Owner를 인터랙터로 전달)
	AActor* Owner = GetOwner();
	return IHackableInterface::Execute_CanHack(Candidate, Owner);
}

void UHackScannerComponent::ScanOnce()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
		return;

	// 현재 시점(카메라 우선) 가져오기
	FVector ViewLoc;
	FRotator ViewRot;
	GetViewPoint(ViewLoc, ViewRot);

	// 전방으로 MaxRange만큼 라인트레이스
	const FVector Start = ViewLoc;
	const FVector End = Start + (ViewRot.Vector() * MaxRange);

	FHitResult Hit;
	// Owner는 트레이스에서 제외(자기 자신 맞는 것 방지)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HackScan), false, Owner);

	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	// 히트했고 && 인터페이스 구현 && CanHack 통과한 경우만 타겟 인정
	AActor* NewTarget = nullptr;
	if (bHit && Hit.GetActor() && IsValidHackCandidate(Hit.GetActor()))
	{
		NewTarget = Hit.GetActor();
	}

	// 타겟 변경 처리(이벤트 브로드캐스트 포함)
	SetCurrentTarget(NewTarget);

	// 디버그 옵션 켜진 경우만 시각화
	if (bDebugDraw)
	{
		DrawDebugLine(
			World,
			Start,
			End,
			bHit ? FColor::Green : FColor::Red,
			false,
			ScanInterval,
			0,
			1.0f
		);

		if (CurrentTarget)
		{
			const float Dist = FVector::Distance(Start, CurrentTarget->GetActorLocation());
			const FString Text = FString::Printf(
				TEXT("Q Target: %s (%.0f)"),
				*CurrentTarget->GetName(),
				Dist
			);

			// 히트 지점 근처에 현재 타겟 표시
			DrawDebugString(
				World,
				Hit.ImpactPoint + FVector(0, 0, 20.f),
				Text,
				nullptr,
				FColor::White,
				ScanInterval,
				true
			);
		}
	}
}

void UHackScannerComponent::SetCurrentTarget(AActor* NewTarget)
{
	// 같은 타겟이면 이벤트/할당 생략
	if (NewTarget == CurrentTarget)
		return;

	// 변경 전/후를 함께 전달하기 위해 Old 보관
	AActor* Old = CurrentTarget;
	CurrentTarget = NewTarget;

	// UI/사운드/하이라이트 등 외부 반응용
	OnTargetChanged.Broadcast(Old, CurrentTarget);
}
