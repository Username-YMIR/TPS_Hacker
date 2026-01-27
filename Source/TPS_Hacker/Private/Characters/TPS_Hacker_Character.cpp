// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/TPS_Hacker_Character.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"

#include "Components/InteractScannerComponent.h"
#include "Components/HackScannerComponent.h"

//interfaces 헤더
#include "EnhancedInputComponent.h"
#include "TPSGameplayTags.h"
#include "TPS_Hacker.h"
#include "TPS_HackerPlayerController.h"
#include "Actors/Projectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/GunComponent.h"
#include "Components/ObjectPoolComponent.h"
#include "Components/PostProcessComponent.h"
#include "Data/TPS_TagRelationshipMap.h"
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
	
	//GunComponent
	GunComp = CreateDefaultSubobject<UGunComponent>(TEXT("GunComponent"));

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
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ATPS_Hacker_Character::Do_FireReleased);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::AimPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this,
		                                   &ATPS_Hacker_Character::AimReleased);
		EnhancedInputComponent->BindAction(TakedownAction, ETriggerEvent::Started, this,
		                                   &ATPS_Hacker_Character::Do_Takedown);
		EnhancedInputComponent->BindAction(ToggleWeaponAction, ETriggerEvent::Started, this,
											&ATPS_Hacker_Character::OnToggleWeapon);

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
	AddStateTag(TPSGameplayTags::Character_State_Combat_ADS);

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
	RemoveStateTag(TPSGameplayTags::Character_State_Combat_ADS);

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

void ATPS_Hacker_Character::Do_Fire()
{
	if (GunComp)
		GunComp->RequestFirePressed();
}

void ATPS_Hacker_Character::Do_FireReleased()
{
	if (GunComp)
		GunComp->RequestFireReleased();
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

void ATPS_Hacker_Character::OnToggleWeapon(const FInputActionValue& Value)
{
	if (!GunComp) return;

	// “T = 장착해제”만 원하면 무장일 때만 해제
	if (GunComp->IsArmed())
	{
		GunComp->RequestUnequip();
	}
	else if (!GunComp->IsArmed())
	{
		GunComp->RequestEquipPrimary();
	}
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



#pragma region GameplayTags
// ============================================================================
// Gameplay Tags Rule Application
// - RelationshipMap(Evaluate)로 Block/Cancel을 계산
// - Cancel은 "상태 태그 제거" + "실제 로직 종료"까지 여기서 수행
// ============================================================================

void ATPS_Hacker_Character::CancelStateTag(const FGameplayTag& Tag)
{
	using namespace TPSGameplayTags;

	// --------------------------------------------------------------------
	// Cancel은 "태그만 제거"하면 끝이 아니라
	// 해당 상태가 켜져있을 때 시작했던 실제 로직도 같이 종료해야 한다.
	// --------------------------------------------------------------------

	// 1) 발사 강제 종료
	if (Tag == Character_State_Combat_Firing)
	{
		// 현재 캐릭터 입력 구현상 발사 종료는 GunComp로 전달하는 방식
		if (GunComp)
		{
			GunComp->RequestFireReleased();
		}
	}

	// 2) 조준(ADS) 강제 종료
	else if (Tag == Character_State_Combat_ADS)
	{
		// 조준 해제는 캐릭터 쪽 타임라인/카메라 로직이 있으므로
		// AimReleased()로 종료 처리(타임라인 Reverse 등)
		// - bWantsAim 플래그도 여기서 false로 정리됨
		AimReleased();
	}

	// 3) 재장전 강제 종료
	else if (Tag == Character_State_Action_Reloading)
	{
		// GunComp에 "재장전 취소" API가 아직 코드에서 확인되지 않음.
		// (추후 RequestReloadCancel() 같은 함수가 생기면 여기서 호출)
		// 지금은 태그만 내려서 상태 머신이 정리되도록 한다.
	}

	// 4) 장착/해제 강제 종료
	else if (Tag == Character_State_Action_Equipping || Tag == Character_State_Action_Unequipping)
	{
		// 마찬가지로 "장착 취소" 전용 API가 현재 코드에서 확인되지 않음.
		// (추후 CancelEquip / CancelUnequip 같은 함수가 생기면 여기서 호출)
	}

	// 마지막으로 상태 태그 제거
	ActiveStateTags.RemoveTag(Tag);
}

void ATPS_Hacker_Character::RebuildBlocksAndApplyCancels()
{
	// --------------------------------------------------------------
	// RelationshipMap 규칙표를 기반으로
	// 1) BlockTags 계산
	// 2) CancelTags 계산
	// 3) CancelTags에 해당하는 상태 강제 종료 + 태그 제거
	// 4) 최종 BlockTags 재계산(상태가 바뀌었으니까)
	// --------------------------------------------------------------
	BlockTags.Reset();

	if (!RelationshipMap)
	{
		return;
	}

	FGameplayTagContainer CancelTags;
	RelationshipMap->Evaluate(ActiveStateTags, BlockTags, CancelTags);

	// Cancel 목록을 배열로 뽑아서 순회
	TArray<FGameplayTag> CancelArray;
	CancelTags.GetGameplayTagArray(CancelArray);

	bool bAnyCancelled = false;

	for (const FGameplayTag& CancelTag : CancelArray)
	{
		// 실제로 켜져있는 상태만 강제 종료
		if (ActiveStateTags.HasTag(CancelTag))
		{
			CancelStateTag(CancelTag);
			bAnyCancelled = true;
		}
	}

	// Cancel로 ActiveStateTags가 바뀌었으면 Block도 다시 계산해서 최종값을 맞춘다.
	if (bAnyCancelled)
	{
		FGameplayTagContainer DummyCancel;
		RelationshipMap->Evaluate(ActiveStateTags, BlockTags, DummyCancel);
	}
}

bool ATPS_Hacker_Character::HasStateTag(const FGameplayTag& StateTag) const
{
	return ActiveStateTags.HasTag(StateTag);
}

bool ATPS_Hacker_Character::IsBlockedByTag(const FGameplayTag& BlockTag) const
{
	return BlockTags.HasTag(BlockTag);
}

void ATPS_Hacker_Character::AddStateTag(const FGameplayTag& StateTag)
{
	if (!ActiveStateTags.HasTag(StateTag))
	{
		ActiveStateTags.AddTag(StateTag);
		RebuildBlocksAndApplyCancels();
	}
}

void ATPS_Hacker_Character::RemoveStateTag(const FGameplayTag& StateTag)
{
	if (ActiveStateTags.HasTag(StateTag))
	{
		ActiveStateTags.RemoveTag(StateTag);
		RebuildBlocksAndApplyCancels();
	}
}

void ATPS_Hacker_Character::NotifyWeaponArmed(bool bArmed)
{
	using namespace TPSGameplayTags;

	// Armed/Unarmed는 배타적이므로 교체
	ActiveStateTags.RemoveTag(Character_State_Weapon_Armed);
	ActiveStateTags.RemoveTag(Character_State_Weapon_Unarmed);

	ActiveStateTags.AddTag(bArmed ? Character_State_Weapon_Armed : Character_State_Weapon_Unarmed);

	RebuildBlocksAndApplyCancels();
}

void ATPS_Hacker_Character::NotifyFirePressed()
{
	using namespace TPSGameplayTags;

	// 룰: Unarmed에서도 Fire 입력은 허용(장착 시도로 연결됨)
	// 실제 발사로 진입한 시점에만 Firing을 올려야 한다.
	if (!ActiveStateTags.HasTag(Character_State_Combat_Firing))
	{
		ActiveStateTags.AddTag(Character_State_Combat_Firing);
		RebuildBlocksAndApplyCancels();
	}
}

void ATPS_Hacker_Character::NotifyFireReleased()
{
	using namespace TPSGameplayTags;

	if (ActiveStateTags.HasTag(Character_State_Combat_Firing))
	{
		ActiveStateTags.RemoveTag(Character_State_Combat_Firing);
		RebuildBlocksAndApplyCancels();
	}
}

void ATPS_Hacker_Character::NotifyReloadStarted()
{
	using namespace TPSGameplayTags;

	if (!ActiveStateTags.HasTag(Character_State_Action_Reloading))
	{
		ActiveStateTags.AddTag(Character_State_Action_Reloading);
		RebuildBlocksAndApplyCancels();
	}
}

void ATPS_Hacker_Character::NotifyReloadFinished()
{
	using namespace TPSGameplayTags;

	if (ActiveStateTags.HasTag(Character_State_Action_Reloading))
	{
		ActiveStateTags.RemoveTag(Character_State_Action_Reloading);
		RebuildBlocksAndApplyCancels();
	}
}

#pragma endregion