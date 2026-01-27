// TPS_TagRelationshipMap.cpp
#include "Data/TPS_TagRelationshipMap.h"
#include "TPSGameplayTags.h"          // TPSGameplayTags::Character_* 태그들
#include "UObject/Package.h"

void UTPS_TagRelationshipMap::Evaluate(const FGameplayTagContainer& ActiveStateTags,
									   FGameplayTagContainer& OutBlockTags,
									   FGameplayTagContainer& OutCancelTags) const
{
	OutBlockTags.Reset();
	OutCancelTags.Reset();

	for (const FTPS_TagRelationshipRule& Rule : Rules)
	{
		if (Rule.RequiredTags.IsEmpty())
			continue;

		const bool bTriggered = Rule.bMatchAny
			? ActiveStateTags.HasAny(Rule.RequiredTags)
			: ActiveStateTags.HasAll(Rule.RequiredTags);

		if (!bTriggered)
			continue;

		OutBlockTags.AppendTags(Rule.BlockTags);
		OutCancelTags.AppendTags(Rule.CancelTags);
	}
}

#if WITH_EDITOR

void UTPS_TagRelationshipMap::PostInitProperties()
{
	Super::PostInitProperties();

	// CDO(클래스 기본 객체)에는 적용하지 않음
	if (HasAnyFlags(RF_ClassDefaultObject))
		return;

	EnsureDefaultRules();
}

void UTPS_TagRelationshipMap::PostLoad()
{
	Super::PostLoad();

	EnsureDefaultRules();
}

static FTPS_TagRelationshipRule MakeRule_All(const TArray<FGameplayTag>& Required,
											 const TArray<FGameplayTag>& Blocks,
											 const TArray<FGameplayTag>& Cancels)
{
	FTPS_TagRelationshipRule R;
	R.bMatchAny = false;
	for (const FGameplayTag& T : Required) R.RequiredTags.AddTag(T);
	for (const FGameplayTag& T : Blocks)   R.BlockTags.AddTag(T);
	for (const FGameplayTag& T : Cancels)  R.CancelTags.AddTag(T);
	return R;
}

static FTPS_TagRelationshipRule MakeRule_Any(const TArray<FGameplayTag>& RequiredAny,
											 const TArray<FGameplayTag>& Blocks,
											 const TArray<FGameplayTag>& Cancels)
{
	FTPS_TagRelationshipRule R;
	R.bMatchAny = true;
	for (const FGameplayTag& T : RequiredAny) R.RequiredTags.AddTag(T);
	for (const FGameplayTag& T : Blocks)      R.BlockTags.AddTag(T);
	for (const FGameplayTag& T : Cancels)     R.CancelTags.AddTag(T);
	return R;
}

void UTPS_TagRelationshipMap::EnsureDefaultRules()
{
	// 옵션 off면 아무 것도 안 함
	if (!bAutoPopulateDefaults)
		return;

	// 이미 룰이 있으면 “자동 주입”은 하지 않음(사용자 커스텀 보존)
	if (!Rules.IsEmpty())
		return;

	using namespace TPSGameplayTags;

	// =========================
	// 룰 세트 (변경사항 반영)
	// =========================
	// R01) Unarmed: Reload만 Block, Fire/ADS는 Block하지 않음
	Rules.Add(MakeRule_All(
		{ Character_State_Weapon_Unarmed },
		{ Character_Block_Combat_Reload },
		{ Character_State_Action_Reloading } // 선택: 혹시 꼬인 리로드 강제 종료
	));

	// R02) Equipping OR Unequipping: Fire/ADS/Reload Block + Cancel(Firing/ADS/Reloading)
	Rules.Add(MakeRule_Any(
		{ Character_State_Action_Equipping, Character_State_Action_Unequipping },
		{ Character_Block_Combat_Fire, Character_Block_Combat_ADS, Character_Block_Combat_Reload },
		{ Character_State_Combat_Firing, Character_State_Combat_ADS, Character_State_Action_Reloading }
	));

	// R03) Reloading: Fire/ADS/Equip Block + Cancel(Firing/ADS)
	Rules.Add(MakeRule_All(
		{ Character_State_Action_Reloading },
		{ Character_Block_Combat_Fire, Character_Block_Combat_ADS, Character_Block_Combat_Equip },
		{ Character_State_Combat_Firing, Character_State_Combat_ADS }
	));

	// R04-A) Firing: Equip/Reload Block
	Rules.Add(MakeRule_All(
		{ Character_State_Combat_Firing },
		{ Character_Block_Combat_Equip, Character_Block_Combat_Reload },
		{ } // Cancel 없음
	));

	// 에디터에서 “이 에셋이 변경됨” 표시(저장하면 디폴트 룰이 파일에 기록됨)
	MarkPackageDirty();
}

#endif // WITH_EDITOR
