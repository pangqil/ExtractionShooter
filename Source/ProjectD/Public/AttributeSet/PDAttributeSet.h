// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Type/Types.h"
#include "PDAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

class UPDBodyPartConfig;
class APDCharacterBase;

UCLASS()
class PROJECTD_API UPDAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData HeadHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, HeadHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxHeadHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxHeadHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData TorsoHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, TorsoHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxTorsoHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxTorsoHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData ArmLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, ArmLHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxArmLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxArmLHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData ArmRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, ArmRHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxArmRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxArmRHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData LegLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, LegLHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxLegLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxLegLHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData LegRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, LegRHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxLegRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxLegRHP)
	
	UPROPERTY(BlueprintReadOnly, Category = "PD|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Stamina)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxStamina)
	
	//Meta Attribute
	UPROPERTY(BlueprintReadOnly, Category = "PD|Meta")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Damage)
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPDBodyPartConfig> BodyPartConfig;
protected:
	//Pre -> Clamping 
	//Post -> GE (Death, Apply Damage)
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
private:
	void HandleAttributeClamp(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ApplyDamageToPart(EBodyPart Part, float DamageAmount);
	
	float GetHPByPart(EBodyPart Part) const;
	void SetHPByPart(EBodyPart Part, float NewValue);
	
	FGameplayAttribute GetAttributeByPart(EBodyPart Part) const;
	
	
};
