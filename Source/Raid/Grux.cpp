// Fill out your copyright notice in the Description page of Project Settings.

#include "Grux.h"
#include "Sound/SoundCue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AudioComponent.h"
#include "ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
AGrux::AGrux()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 150.0f;

	// ���̷�Ż �޽� ����
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GRUXMESH(TEXT("SkeletalMesh'/Game/ParagonGrux/Characters/Heroes/Grux/Skins/Tier_2/Grux_Beetle_Molten/Meshes/GruxMolten.GruxMolten'"));
	if (GRUXMESH.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(GRUXMESH.Object);
	}

	// �ִϸ��̼� �������Ʈ �Ӽ�����
	static ConstructorHelpers::FClassFinder<UAnimInstance> GRUXANIM(TEXT("AnimBlueprint'/Game/Characters/BP_GruxAnim.BP_GruxAnim_C'")); // _C�� �ٿ� Ŭ���������� ������
	if (GRUXANIM.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(GRUXANIM.Class);
	}

	// ����� ������Ʈ �ʱ�ȭ
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("GruxAudio"));
	AudioComponent->bAutoActivate = false;
	AudioComponent->SetupAttachment(GetMesh());

}

// Called when the game starts or when spawned
void AGrux::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGrux::OnAttackMontageEnded(UAnimMontage * Montage, bool bInterrupted)
{

}

// Called every frame
void AGrux::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGrux::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

// Called to bind functionality to input
void AGrux::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

