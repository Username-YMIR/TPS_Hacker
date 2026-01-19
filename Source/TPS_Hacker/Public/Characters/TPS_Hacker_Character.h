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
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;
	
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	
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
	//Input
	void Input_Interact();  // E 키, 근접 상호작용
	void Input_Hack();		// Q 키, 원거리 상호작용(해킹)
	
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
};
