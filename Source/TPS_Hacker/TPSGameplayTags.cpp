// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameplayTags.h"

namespace TPSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Grounded, "Character.State.Movement.Grounded");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_InAir,	"Character.State.Movement.InAir");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Crouch,	"Character.State.Movement.Crouch");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Movement_Sprint,	"Character.State.Movement.Sprint");
	
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Weapon_Unarmed,    "Character.State.Weapon.Unarmed");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Weapon_Armed,      "Character.State.Weapon.Armed");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Weapon_Type_Pistol,"Character.State.Weapon.Type.Pistol");
	
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Combat_ADS,        "Character.State.Combat.ADS");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Combat_Firing,     "Character.State.Combat.Firing");
	
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Action_Reloading,  "Character.State.Action.Reloading");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Action_Equipping,  "Character.State.Action.Equipping");
	UE_DEFINE_GAMEPLAY_TAG(Character_State_Action_Unequipping,"Character.State.Action.Unequipping");

	UE_DEFINE_GAMEPLAY_TAG(Character_Block_Combat_Fire,       "Character.Block.Combat.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Character_Block_Combat_ADS,        "Character.Block.Combat.ADS");
	UE_DEFINE_GAMEPLAY_TAG(Character_Block_Combat_Reload,     "Character.Block.Combat.Reload");
	UE_DEFINE_GAMEPLAY_TAG(Character_Block_Combat_Equip,      "Character.Block.Combat.Equip");
}