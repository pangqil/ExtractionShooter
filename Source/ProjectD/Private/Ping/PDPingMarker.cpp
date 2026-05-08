// Fill out your copyright notice in the Description page of Project Settings.


#include "Ping/PDPingMarker.h"

// Sets default values
APDPingMarker::APDPingMarker()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APDPingMarker::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APDPingMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

