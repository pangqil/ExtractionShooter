// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSet/PDAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Data/PDBodyPartConfig.h"

void UPDAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	HandleAttributeClamp(Attribute, NewValue);
}

void UPDAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute==GetDamageAttribute())
	{
		const float LocalDamage=GetDamage();
		SetDamage(0.f);

		if (LocalDamage<=0.f) return;
		
		EBodyPart Part=EBodyPart::Torso;
		if (const FHitResult* HitResult=Data.EffectSpec.GetContext().GetHitResult())
		{//HitResult�� EffectContext �ȿ�
			if (BodyPartConfig)
			{
				Part=BodyPartConfig->GetBodyPartFromName(HitResult->BoneName);
			}
		}
		const float NewHP=FMath::Max(GetHPByPart(Part) - LocalDamage, 0.f);
		SetHPByPart(Part, NewHP);
		
		if (GetTorsoHP()<=0.f||GetHeadHP()<=0.f)
		{
			if (APDCharacterBase* Owner=Cast<APDCharacterBase>(GetOwningActor()))
			{
				Owner->HandleDeath(Data.EffectSpec.GetContext().GetInstigator());
			}
		}
	}
}

void UPDAttributeSet::HandleAttributeClamp(const FGameplayAttribute& Attribute, float& NewValue) const
{
	NewValue=FMath::Max(NewValue, 0.f);

	if (Attribute==GetHeadHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxHeadHP());
	else if (Attribute==GetTorsoHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxTorsoHP());
	else if (Attribute==GetArmLHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxArmLHP());
	else if (Attribute==GetArmRHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxArmRHP());
	else if (Attribute==GetLegLHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxLegLHP());
	else if (Attribute==GetLegRHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxLegRHP());
	else if (Attribute==GetStaminaAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxStamina());
}

FGameplayAttribute UPDAttributeSet::GetAttributeByPart(EBodyPart Part) const
{
	switch (Part)
	{
		case EBodyPart::Head:  return GetHeadHPAttribute();
		case EBodyPart::Arm_L: return GetArmLHPAttribute();
		case EBodyPart::Arm_R: return GetArmRHPAttribute();
		case EBodyPart::Leg_L: return GetLegLHPAttribute();
		case EBodyPart::Leg_R: return GetLegRHPAttribute();
		default:               return GetTorsoHPAttribute();
	}
}
void UPDAttributeSet::ApplyDamageToPart(EBodyPart Part, float DamageAmount)
{
	switch (Part)
	{
		case EBodyPart::Head:
			SetHeadHP(FMath::Max(GetHeadHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Arm_L:
			SetArmLHP(FMath::Max(GetArmLHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Arm_R:
			SetArmRHP(FMath::Max(GetArmRHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Leg_L:
			SetLegLHP(FMath::Max(GetLegLHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Leg_R:
			SetLegRHP(FMath::Max(GetLegRHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Torso:
		default:
			SetTorsoHP(FMath::Max(GetTorsoHP() - DamageAmount, 0.f));
			break;
	}
}

float UPDAttributeSet::GetHPByPart(EBodyPart Part) const
{
	switch (Part)
	{
		case EBodyPart::Head:  return GetHeadHP();
		case EBodyPart::Torso: return GetTorsoHP();
		case EBodyPart::Arm_L: return GetArmLHP();
		case EBodyPart::Arm_R: return GetArmRHP();
		case EBodyPart::Leg_L: return GetLegLHP();
		case EBodyPart::Leg_R: return GetLegRHP();
		default:               return 0.f;
	}
}

void UPDAttributeSet::SetHPByPart(EBodyPart Part, float NewValue)
{
	switch (Part)
	{
		case EBodyPart::Head:  SetHeadHP(NewValue);  break;
		case EBodyPart::Torso: SetTorsoHP(NewValue); break;
		case EBodyPart::Arm_L: SetArmLHP(NewValue);  break;
		case EBodyPart::Arm_R: SetArmRHP(NewValue);  break;
		case EBodyPart::Leg_L: SetLegLHP(NewValue);  break;
		case EBodyPart::Leg_R: SetLegRHP(NewValue);  break;
	}
}
