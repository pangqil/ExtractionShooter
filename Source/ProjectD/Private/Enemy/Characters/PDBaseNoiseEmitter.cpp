// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Characters/PDBaseNoiseEmitter.h"

#include "Components/CapsuleComponent.h"
#include "Components/StateTreeComponent.h"
#include "Perception/AISense_Hearing.h"

APDBaseNoiseEmitter::APDBaseNoiseEmitter()
{
	PrimaryActorTick.bCanEverTick = false;

	TeamID = 1;
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
}

void APDBaseNoiseEmitter::BeginPlay()
{
	Super::BeginPlay();

	if (StateTreeComponent)
	{
		StateTreeComponent->StartLogic();
	}
}

void APDBaseNoiseEmitter::EmitNoise()
{
	const float NoiseRange = MaxNoiseRange + FMath::RandRange(-NoiseRangeRandomDeviation, NoiseRangeRandomDeviation);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this, NoiseRange, NoiseTag);
}
