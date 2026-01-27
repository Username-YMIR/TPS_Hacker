// TPS_TagRelationshipMap.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "TPS_TagRelationshipMap.generated.h"

USTRUCT(BlueprintType)
struct FTPS_TagRelationshipRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer RequiredTags;

	// true면 RequiredTags 중 하나라도 있으면 발동(OR)
	// false면 RequiredTags 모두 있어야 발동(AND)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bMatchAny = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer BlockTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer CancelTags;
};

UCLASS(BlueprintType)
class UTPS_TagRelationshipMap : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTPS_TagRelationshipRule> Rules;

	// Rules가 비어있을 때 자동으로 기본 룰을 채울지 여부(에디터 전용)
	UPROPERTY(EditAnywhere, Category="Defaults")
	bool bAutoPopulateDefaults = true;

	void Evaluate(const FGameplayTagContainer& ActiveStateTags,
				  FGameplayTagContainer& OutBlockTags,
				  FGameplayTagContainer& OutCancelTags) const;

protected:
#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
#endif

private:
#if WITH_EDITOR
	void EnsureDefaultRules();
#endif
};
