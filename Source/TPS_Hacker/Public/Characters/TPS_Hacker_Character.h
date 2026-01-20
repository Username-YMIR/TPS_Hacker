// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TPS_Hacker_Character.generated.h"


// 플레이어 캐릭터
// Interact/Hack 스캐너 컴포넌트를 보유
//  - E/Q 입력을 해당 타겟에게 인터페이스 호출로 라우팅

class UInputAction;   

UCLASS()
class TPS_HACKER_API ATPS_Hacker_Character : public ACharacter
{
	GENERATED_BODY()

protected:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* MouseLookAction;
	
	// 달리기, 걷기 토글
	UPROPERTY(EditAnywhere, Category="Input|Movement")
	UInputAction* RunWalkToggleAction;
	
	// 해킹
	UPROPERTY(EditAnywhere, Category="Input|Interact")
	UInputAction* HackAction; 
	
	// 상호작용 (줍기)
	UPROPERTY(EditAnywhere, Category="Input|Interact")
	UInputAction* InteractAction; 
	
	// 총 발사
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* FireAction;
	
	// 조준
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* AimAction;
	
	//처형
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* TakedownAction;
	
	//전투
	UPROPERTY(EditAnywhere, Category="Combat")
	TSubclassOf<AActor> ProjectileClass;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	FName MuzzleSocketName = "Muzzle";
	
	
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	
	//Do - Input 시 실제 작동 함수
	void Do_RunWalkToggle();// Shift 키, 달리기/걷기 토글
	void Do_Hack();	    	// Q 키, 원거리 상호작용(해킹)
	void Do_Interact();     // E 키, 근거리 상호작용
	void Do_Fire();			// 좌클릭, 발사
	void Do_Aim();			// 우클릭, 조준
	void Do_Takedown();		// F키, 처형
	
public:
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
public:
	// Sets default values for this character's properties
	ATPS_Hacker_Character();
	
public:
	void SetShoulder(bool bRight);

private:
	
	//Camera
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> FollowCamera;
	
	//Components
	UPROPERTY(VisibleAnywhere, Category = "Interact")
	TObjectPtr<class UInteractScannerComponent> InteractScanner;
	
	UPROPERTY(VisibleAnywhere, Category = "Intercat")
	TObjectPtr<class UHackScannerComponent> HackScanner;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pool")
	TObjectPtr<class UObjectPoolComponent> PoolComp;
	
	
};
