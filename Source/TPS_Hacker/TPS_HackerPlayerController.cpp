// Copyright Epic Games, Inc. All Rights Reserved.


#include "TPS_HackerPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "TPS_Hacker.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ATPS_HackerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogTPS, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}

	//HUD 띄우기
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
			UE_LOG(LogTPS, Log, TEXT("HUD Widget created."));
		}
	}
	// 기본 인풋모드 & 커서
	bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
}

void ATPS_HackerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}


// 파라미터 변수
// FVector& OutHitPoint,              // [out] 조준점이 가리키는 월드 지점(히트 시 ImpactPoint, 미히트 시 End)
// float Range,                       // 라인트레이스 최대 거리
// ECollisionChannel TraceChannel,     // 트레이스에 사용할 채널(예: ECC_Visibility)
// bool bDrawDebug                    // 디버그 라인/스피어 표시 여부
bool ATPS_HackerPlayerController::GetAimHitPoint(FVector& OutHitPoint, float Range, ECollisionChannel TraceChannel,
                                                 bool bDrawDebug) const
{
	UE_LOG(LogTPS, Warning, TEXT("ATPS_Hacker_PlayerController::GetAimHitPoint()"));

	// 1) 현재 뷰포트(화면) 크기 획득
	int32 SizeX = 0, SizeY = 0;
	GetViewportSize(SizeX, SizeY);

	// 2) 화면 중앙 좌표 계산 (조준점 위치)
	const float ScreenX = SizeX * 0.5f;
	const float ScreenY = SizeY * 0.5f;

	// 3) 화면 중앙 좌표를 월드 공간의 레이로 변환
	//    RayOrigin: 레이 시작점(보통 카메라 근처)
	//    RayDir: 레이 방향(정규화된 단위 벡터)
	FVector RayOrigin, RayDir;
	if (!DeprojectScreenPositionToWorld(ScreenX, ScreenY, RayOrigin, RayDir))
	{
		// Deproject 실패(드문 케이스: 뷰포트/로컬플레이어 문제 등)
		return false;
	}

	// 4) 레이 시작/끝점 구성
	const FVector Start = RayOrigin;
	const FVector End = RayOrigin + RayDir * Range;

	// 5) 트레이스 파라미터 설정
	// - SCENE_QUERY_STAT: 프로파일링/디버깅용 태그
	// - 두 번째 인자(false): 복잡 충돌(Complex) 여부를 여기서 강제하지 않음
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AimTrace), false);

	// 자기 자신(플레이어 폰)을 무시해서 카메라/캡슐/메쉬에 맞는 상황 방지
	if (APawn* P = GetPawn())
	{
		Params.AddIgnoredActor(P);
	}

	// 6) 라인트레이스 실행
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		TraceChannel,
		Params
	);

	// 7) 결과 목표점 산출
	// - 히트: 실제 충돌 지점(ImpactPoint)
	// - 미히트: 최대 사거리 끝점(End)
	OutHitPoint = bHit ? Hit.ImpactPoint : End;

	// 8) 디버그 시각화(옵션)
	if (bDrawDebug)
	{
		// 라인: 시작점 -> 목표점(히트면 녹색, 미히트면 빨간색)
		DrawDebugLine(GetWorld(), Start, OutHitPoint, bHit ? FColor::Green : FColor::Red, false, 0.05f, 0, 1.0f);

		// 히트 지점은 작은 구로 표시
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), OutHitPoint, 6.f, 8, FColor::Green, false, 0.05f);
		}
	}


	// 함수 자체의 성공/실패는 "Deproject 성공 여부"로만 판단한다.
	// (Trace가 미히트여도 OutHitPoint는 End로 유효한 값이 된다)
	return true;
}
