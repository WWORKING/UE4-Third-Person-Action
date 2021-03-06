// Fill out your copyright notice in the Description page of Project Settings.

#include "BTDecorator_Distance.h"
#include "GruxAIController.h"
#include "RaidPlayer.h"
#include "Grux.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_Distance::UBTDecorator_Distance()
{
	NodeName = TEXT("Distance");
}

bool UBTDecorator_Distance::CalculateRawConditionValue(UBehaviorTreeComponent & OwnerComp, uint8 * NodeMemory) const
{
	bool bResult = Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	auto ControllingPawn = Cast<AGrux>(OwnerComp.GetAIOwner()->GetPawn());
	if (nullptr == ControllingPawn)
		return false;

	auto Target = Cast<ARaidPlayer>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(AGruxAIController::TargetKey));
	if (nullptr == Target)
		return false;

	bResult = (Target->GetDistanceTo(ControllingPawn) >= 1000.0f);
	// UE_LOG(LogTemp, Log, TEXT("can Attack: %s"),bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}






