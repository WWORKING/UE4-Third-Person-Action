// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Raid.h"
#include "AIController.h"
#include "GruxAIController.generated.h"

/**
 * 
 */
UCLASS()
class RAID_API AGruxAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AGruxAIController();

	virtual void Possess(APawn* InPawn) override;

	void StopAI();
	void StartAI();

	UPROPERTY()
	float Radius;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class ARaidGameMode* GameMode;
	
	static const FName TargetKey;
	static const FName Distance;
	static const FName Direction;
	static const FName SelectAttackNumber;
	static const FName SelectAttackNumber2;

private:
	UPROPERTY()
	class UBehaviorTree* BTGrux;

	UPROPERTY()
	class UBlackboardData* BBGrux;
	

};
