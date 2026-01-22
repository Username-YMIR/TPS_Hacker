// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/TPS_Hacker_Character.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"

#include "Components/InteractScannerComponent.h"
#include "Components/HackScannerComponent.h"

//interfaces 헤더
#include "EnhancedInputComponent.h"
#include "TPS_Hacker.h"
#include "TPS_HackerPlayerController.h"
#include "Actors/Projectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/ObjectPoolComponent.h"
#include "Components/PostProcessComponent.h"
#include "Framework/TPS_Hacker_PlayerController.h"
#include "Interfaces/InteractableInterface.h"
#include "Interfaces/HackableInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ATPS_Hacker_Character::ATPS_Hacker_Character()
{
	// 줌인을 위해 사용
	PrimaryActorTick.bCanEverTick = true;
	
	

	//Camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(FName("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 300.f;
	SpringArm->bUsePawnControlRotation = true;

	// 어깨 오프셋: 카메라가 오른쪽 어깨에 있음
	SpringArm->SocketOffset = FVector(80.f, 80.f, 80.f); // 우측 어깨 예시

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false; // SpringArm이 회전 담당함
	
	// DefaultFOV 캐싱
	if (FollowCamera)
	{
		DefaultFOV = FollowCamera->FieldOfView;
	}

	// --- Scanners
	InteractScanner = CreateDefaultSubobject<UInteractScannerComponent>(TEXT("InteractScanner"));
	HackScanner = CreateDefaultSubobject<UHackScannerComponent>(TEXT("HackScanner"));

	// PoolComponent
	PoolComp = CreateDefaultSubobject<UObjectPoolComponent>(TEXT("ObjectPool"));
	
	
	// PostProcess Component 생성
	FocusPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("FocusPostProcess"));
	FocusPostProcess->SetupAttachment(GetRootComponent());

	// 기본은 꺼짐(BlendWeight = 0)
	FocusPostProcess->bUnbound = false;     // 카메라 주변/컴포넌트 위치 기반(일반적)
	FocusPostProcess->BlendWeight = 0.0f;

	// 포커스 모드 기본 효과값
	// 추후 에디터에서 튜닝 권장
	{
		auto& PPS = FocusPostProcess->Settings;

		// 비네팅(집중 느낌)
		PPS.bOverride_VignetteIntensity = true;
		PPS.VignetteIntensity = 0.6f;

		// 채도 낮추기(세상 느려짐 느낌)
		PPS.bOverride_ColorSaturation = true;
		PPS.ColorSaturation = FVector4(0.75f, 0.75f, 0.75f, 1.0f);

		// 콘트라스트 살짝
		PPS.bOverride_ColorContrast = true;
		PPS.ColorContrast = FVector4(1.05f, 1.05f, 1.05f, 1.0f);

		// 크로마틱 어버레이션 약간(과하면 멀미)
		PPS.bOverride_SceneFringeIntensity = true;
		PPS.SceneFringeIntensity = 0.3f;

		// 블룸 조금(선택)
		PPS.bOverride_BloomIntensity = true;
		PPS.BloomIntensity = 0.25f;
	}
}


// Called when the game starts or when spawned
void ATPS_Hacker_Character::BeginPlay()
{
	Super::BeginPlay();
	
	// 평소에는 Tick 꺼두고, 줌 재생 중에만 켠다.
	SetActorTickEnabled(false);

	// 프로젝타일에 전달할 콜리전 채널
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		MyCollisionChannel = Capsule->GetCollisionObjectType(); // 내 ObjectType(=ECollisionChannel)
	}
	else
	{
		MyCollisionChannel = ECC_MAX;
	}
	
	// 타임라인 바인딩
	if (AimCurve)
	{
		FOnTimelineFloat Update;
		Update.BindUFunction(this, FName("OnAimTimelineUpdate"));
		AimTimeline.AddInterpFloat(AimCurve, Update);

		FOnTimelineEvent Finished;
		Finished.BindUFunction(this, FName("OnAimTimelineFinished"));
		AimTimeline.SetTimelineFinishedFunc(Finished);

		AimTimeline.SetLooping(false);
	}

	//  카메라 기준으로 라인트레이스를 쏘기 위해 카메라 참조를 HackScanner에게 넘긴다
	if (HackScanner)
	{
		HackScanner->Initialize(FollowCamera);
	}
	
	// 포커스모드 효과 확실하게 끄기
	if (FocusPostProcess)
	{
		FocusVFX_CurrentWeight = 0.0f;
		FocusPostProcess->BlendWeight = 0.0f;
	}
}

void ATPS_Hacker_Character::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//포커스 모드 강제 종료
	ForceExitFocusMode();
	
	Super::EndPlay(EndPlayReason);
}


// Called to bind functionality to input
void ATPS_Hacker_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//프로젝트 세팅 - 인풋 - 액션매핑에 추가
	// "Interact" : E
	// "Hack" : Q
	// PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ATPS_Hacker_Character::Do_Interact);
	// PlayerInputComponent->BindAction(TEXT("Hack"), IE_Pressed, this, &ATPS_Hacker_Character::Do_Hack);

	//기본 움직임 인풋
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPS_Hacker_Character::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this,
		                                   &ATPS_Hacker_Character::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPS_Hacker_Character::Look);

		// Combat
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Fire);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::AimPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this,
		                                   &ATPS_Hacker_Character::AimReleased);
		EnhancedInputComponent->BindAction(TakedownAction, ETriggerEvent::Started, this,
		                                   &ATPS_Hacker_Character::Do_Takedown);

		//Hack, Interact
		EnhancedInputComponent->BindAction(HackAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Hack);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Interact);
		
		// FocusMode
		EnhancedInputComponent->BindAction(FocusModeAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::ToggleFocusMode);
	}
	else
	{
		UE_LOG(LogTPS, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

// 카메라 좌 우 어깨 전환
void ATPS_Hacker_Character::SetShoulder(bool bRight)
{
	const float Side = bRight ? 80.f : -80.f;
	SpringArm->SocketOffset.Y = Side;
}



void ATPS_Hacker_Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// 줌 중에만 Tick이 켜지므로 여기서 타임라인 갱신
	AimTimeline.TickTimeline(DeltaSeconds);

}


//--------------------------------
//  조준 관련
//--------------------------------
void ATPS_Hacker_Character::AimPressed()
{
	bWantsAim = true;
	StartAimTimelineForward();
}

void ATPS_Hacker_Character::StartAimTimelineForward()
{
	if (!AimCurve || !FollowCamera) return;
	UE_LOG(LogTPS, Log, TEXT("StartAimTimelineForward"));

	SetActorTickEnabled(true);

	AimTimeline.SetPlayRate(AimInPlayRate); // 빠르게 줌인
	AimTimeline.Play();                     // 0 -> 1
}

void ATPS_Hacker_Character::AimReleased()
{
	bWantsAim = false;
	StartAimTimelineReverse();
}

void ATPS_Hacker_Character::StartAimTimelineReverse()
{
	if (!AimCurve || !FollowCamera) return;
	UE_LOG(LogTPS, Log, TEXT("StartAimTimelineReverse"));

	SetActorTickEnabled(true);

	AimTimeline.SetPlayRate(AimOutPlayRate); // 천천히 줌아웃
	AimTimeline.Reverse();                   // 1 -> 0
}



void ATPS_Hacker_Character::OnAimTimelineUpdate(float Alpha)
{
	// Alpha: 커브 결과(0..1). 이 값을 FOV 보간에 사용
	const float NewFOV = FMath::Lerp(DefaultFOV, AimFOV, Alpha);
	FollowCamera->SetFieldOfView(NewFOV);
}

void ATPS_Hacker_Character::OnAimTimelineFinished()
{
	// 재생이 끝났으면 Tick도 끈다(최적화)
	if (!AimTimeline.IsPlaying())
	{
		SetActorTickEnabled(false);
	}
}

// ==============================================================================

#pragma region InputFunctions
void ATPS_Hacker_Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ATPS_Hacker_Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ATPS_Hacker_Character::Do_RunWalkToggle()
{
}

void ATPS_Hacker_Character::Do_Hack()
{
	// 스캐너 유효성 검사
	if (!HackScanner)
	{
		return;
	}

	//타겟 캐싱 및 유효성 검사
	AActor* Target = HackScanner->GetCurrentTarget();
	if (!Target)
	{
		return;
	}

	// 인터페이스 구현 여부 검사
	if (!Target->GetClass()->ImplementsInterface(UHackableInterface::StaticClass()))
	{
		return;
	}

	//해킹 가능할 경우 실행
	if (IHackableInterface::Execute_CanHack(Target, this))
	{
		IHackableInterface::Execute_ExecuteHack(Target, this);
	}
}

void ATPS_Hacker_Character::Do_Interact()
{
	// 스캐너 유효성 검사
	if (!InteractScanner)
	{
		return;
	}

	//타겟 캐싱 및 유효성 섬사
	AActor* Target = InteractScanner->GetCurrentTarget();
	if (!Target)
	{
		return;
	}

	//인터페이스 구현 여부
	if (!Target->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return;
	}

	// 조건 확인 후 실행
	if (IInteractableInterface::Execute_CanInteract(Target, this))
	{
		IInteractableInterface::Execute_OnInteract(Target, this);
	}
}


// Input
//  -> Fire()
//   -> PoolComp.Acquire()
// 	 -> Projectile.Launch()
// 	   -> OnHit / LifeExpired
// 		 -> ReturnToPool()
// 임시로 캐릭터에 구현. 총기 컴포넌트에 추후 이동
// void ATPS_Hacker_Character::Do_Fire()
// {
// 	UE_LOG(LogTPS, Warning, TEXT("Do_Fire()"));
// 	
// 	if (!PoolComp || !ProjectileClass)
// 		return;
//
// 	// FVector TempMuzzleLoc =  GetActorLocation() + (GetActorForwardVector() * 80);
// 	
// 	const FVector MuzzleLoc = GetActorLocation();
// 	const FRotator MuzzleRot = GetActorRotation();
// 	const FTransform XForm(MuzzleRot, MuzzleLoc);
//
// 	AActor* Spawned = PoolComp->Acquire(ProjectileClass, XForm);
// 	AProjectile* Proj = Cast<AProjectile>(Spawned);
// 	if (!Proj) return;
//
// 	Proj->SetOwner(this);
// 	Proj->SetOwningPool(PoolComp);
// 	
// 	//무시할 오브젝트 타입 추가 로직
// 	Proj->OwnerCollisionChannel = MyCollisionChannel;
// 	Proj->ApplyProjectileCollision();
// 	
// 	
// 	const FVector Dir = MuzzleRot.Vector();
// 	Proj->Launch(Dir);
// }

void ATPS_Hacker_Character::Do_Fire()
{
	if (!PoolComp || !ProjectileClass)
	{
		return;
	}

	// 1) 조준점(화면 중앙) 기준으로 목표 지점(AimPoint) 계산
	// ATPS_Hacker_PlayerController* PC = Cast<ATPS_Hacker_PlayerController>(GetController());
	ATPS_HackerPlayerController* PC = Cast<ATPS_HackerPlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	UE_LOG(LogTPS, Warning, TEXT("Do_Fire()"));

	constexpr float Range = 100000.f;

	FVector AimPoint;
	// Deproject/Trace가 실패하더라도 false를 반환하니, 실패 시에는 카메라 forward로 폴백
	if (!PC->GetAimHitPoint(AimPoint, Range, ECC_Visibility, /*bDrawDebug*/ false))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		AimPoint = CamLoc + CamRot.Vector() * Range;
	}

	// 2) 총구 위치(Spawn 위치)
	//    지금은 GetActorLocation 사용
	//    (추후 Muzzle 소켓으로 교체)
	const FVector MuzzleLoc = GetActorLocation();

	// 3) 총구 -> AimPoint 방향으로 발사 방향 계산
	const FVector Dir = (AimPoint - MuzzleLoc).GetSafeNormal();

	// 4) 발사 방향으로 스폰 회전 구성 (프로젝타일이 스폰 시 방향을 사용한다면 유용)
	const FRotator SpawnRot = Dir.Rotation();
	const FTransform XForm(SpawnRot, MuzzleLoc);

	// 5) 풀에서 프로젝타일 획득
	AActor* Spawned = PoolComp->Acquire(ProjectileClass, XForm);
	AProjectile* Proj = Cast<AProjectile>(Spawned);
	if (!Proj)
	{
		return;
	}

	// 6) 소유자/풀 세팅
	Proj->SetOwner(this);
	Proj->SetOwningPool(PoolComp);

	// 7) 무시할 오브젝트 타입(채널) 적용
	Proj->OwnerCollisionChannel = MyCollisionChannel;
	Proj->ApplyProjectileCollision();

	// 8) 조준점 기준 방향으로 발사
	Proj->Launch(Dir);
}

void ATPS_Hacker_Character::Do_Takedown()
{
}

void ATPS_Hacker_Character::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ATPS_Hacker_Character::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ATPS_Hacker_Character::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ATPS_Hacker_Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

#pragma endregion

//---------------------------------
// 포커스 모드
//---------------------------------

#pragma region FocusMode

void ATPS_Hacker_Character::ToggleFocusMode()
{
	UE_LOG(LogTPS, Warning, TEXT("ToggleFocusMode()"));	
	
	// 필요하면 여기서 조건 차단:
	// if (bIsDead || bIsInMenu || bIsHackingView) return;

	if (bIsFocusMode)
	{
		ExitFocusMode();
	}
	else
	{
		EnterFocusMode();
	}
}

void ATPS_Hacker_Character::EnterFocusMode()
{
	if (bIsFocusMode) return;

	bIsFocusMode = true;
	
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), FocusModeTimeDilation);
	// VFX ON (부드럽게)
	StartFocusVFXFade(FocusVFX_OnWeight);

}

void ATPS_Hacker_Character::ExitFocusMode()
{
	if (!bIsFocusMode) return;

	bIsFocusMode = false;
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), NormalModeTimeDilation);
	
	// VFX OFF (부드럽게)
	StartFocusVFXFade(0.0f);

}

//사망 / 메뉴 / 시점 전환 / 레벨 전환 시 호출 
void ATPS_Hacker_Character::ForceExitFocusMode()
{
	bIsFocusMode = false;
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), NormalModeTimeDilation);
	StopFocusVFXFade();
}


void ATPS_Hacker_Character::StartFocusVFXFade(float TargetWeight)
{
	if (!FocusPostProcess) return;
	
	UE_LOG(LogTPS, Warning, TEXT("StartFocusVFXFade()"));
	FocusVFX_TargetWeight = FMath::Clamp(TargetWeight, 0.0f, 1.0f);
	FocusVFX_CurrentWeight = FocusPostProcess->BlendWeight;

	const float FadeTime = FMath::Max(0.01f, FocusVFX_FadeTime);
	FocusVFX_FadeSpeedPerSec = FMath::Abs(FocusVFX_TargetWeight - FocusVFX_CurrentWeight) / FadeTime;

	// 기존 페이드 중이면 교체
	GetWorldTimerManager().ClearTimer(FocusVFXFadeTimer);

	// 타임 딜레이션 영향을 받지 않게 "Real Time"로 돌리고 싶으면 Tick 방식이 더 깔끔함.
	// 여기서는 단순히 타이머(월드시간)로 구현. 느려진 시간에 따라 페이드도 살짝 느려지는 효과가 생김.
	GetWorldTimerManager().SetTimer(
		FocusVFXFadeTimer,
		this,
		&ATPS_Hacker_Character::TickFocusVFXFade,
		0.0f,
		true
	);
}

void ATPS_Hacker_Character::StopFocusVFXFade()
{
	GetWorldTimerManager().ClearTimer(FocusVFXFadeTimer);
}

void ATPS_Hacker_Character::TickFocusVFXFade()
{
	if (!FocusPostProcess)
	{
		StopFocusVFXFade();
		return;
	}

	const float Delta = GetWorld()->GetDeltaSeconds();

	const float NewWeight = FMath::FInterpConstantTo(
		FocusPostProcess->BlendWeight,
		FocusVFX_TargetWeight,
		Delta,
		FocusVFX_FadeSpeedPerSec
	);

	FocusPostProcess->BlendWeight = NewWeight;

	if (FMath::IsNearlyEqual(NewWeight, FocusVFX_TargetWeight, 0.001f))
	{
		FocusPostProcess->BlendWeight = FocusVFX_TargetWeight;
		StopFocusVFXFade();
	}
}
#pragma endregion