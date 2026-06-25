// Fill out your copyright notice in the Description page of Project Settings.


#include "test1.h"

// Sets default values
Atest1::Atest1()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void Atest1::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void Atest1::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void Atest1::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

