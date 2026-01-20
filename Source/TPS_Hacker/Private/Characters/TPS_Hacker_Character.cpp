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
#include "Actors/Projectile.h"
#include "Components/ObjectPoolComponent.h"
#include "Interfaces/InteractableInterface.h"
#include "Interfaces/HackableInterface.h"

// Sets default values
ATPS_Hacker_Character::ATPS_Hacker_Character()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	//Camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(FName("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 300.f;
	SpringArm->bUsePawnControlRotation = true;
	
	// 어깨 오프셋: Y가 좌우(오른쪽 +, 왼쪽 -), Z가 위아래
	SpringArm->SocketOffset = FVector(0.f, 120.f, 15.f); // 우측 어깨 예시

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false; // SpringArm이 회전 담당함

	// --- Scanners
	InteractScanner = CreateDefaultSubobject<UInteractScannerComponent>(TEXT("InteractScanner"));
	HackScanner  = CreateDefaultSubobject<UHackScannerComponent>(TEXT("HackScanner"));
	
	// PoolComponent
	PoolComp = CreateDefaultSubobject<UObjectPoolComponent>(TEXT("ObjectPool"));
}

// 카메라 좌 우 어깨 전환
void ATPS_Hacker_Character::SetShoulder(bool bRight)
{
	const float Side = bRight ? 60.f : -60.f;
	SpringArm->SocketOffset.Y = Side;
}

// Called when the game starts or when spawned
void ATPS_Hacker_Character::BeginPlay()
{
	Super::BeginPlay();	
	
	//  카메라 기준으로 라인트레이스를 쏘기 위해 카메라 참조를 HackScanner에게 넘긴다
	if (HackScanner)
	{
		HackScanner->Initialize(FollowCamera);
	}
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
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPS_Hacker_Character::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATPS_Hacker_Character::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPS_Hacker_Character::Look);
		
		// Combat
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Fire);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Aim);
		EnhancedInputComponent->BindAction(TakedownAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Takedown);
		
		//Hack, Interact
		EnhancedInputComponent->BindAction(HackAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Hack);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATPS_Hacker_Character::Do_Interact);
	}
	else
	{
		UE_LOG(LogTPS, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
	
}


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
		return;

	//타겟 캐싱 및 유효성 검사
	AActor* Target = HackScanner->GetCurrentTarget();
	if (!Target)
		return;

	// 인터페이스 구현 여부 검사
	if (!Target->GetClass()->ImplementsInterface(UHackableInterface::StaticClass()))
		return;

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
		return;
	
	//타겟 캐싱 및 유효성 섬사
	AActor* Target = InteractScanner->GetCurrentTarget();
	if (!Target)
		return;

	//인터페이스 구현 여부
	if (!Target->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		return;

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
void ATPS_Hacker_Character::Do_Fire()
{
	UE_LOG(LogTPS, Warning, TEXT("Do_Fire()"));
	
	if (!PoolComp || !ProjectileClass)
		return;

	FVector TempMuzzleLoc =  GetActorLocation() + (GetActorForwardVector() * 80);
	
	const FVector MuzzleLoc = TempMuzzleLoc;
	const FRotator MuzzleRot = GetActorRotation();
	const FTransform XForm(MuzzleRot, MuzzleLoc);

	AActor* Spawned = PoolComp->Acquire(ProjectileClass, XForm);
	AProjectile* Proj = Cast<AProjectile>(Spawned);
	if (!Proj) return;

	Proj->SetOwner(this);
	Proj->SetOwningPool(PoolComp);

	const FVector Dir = MuzzleRot.Vector();
	Proj->Launch(Dir);
}

void ATPS_Hacker_Character::Do_Aim()
{
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