// Fill out your copyright notice in the Description page of Project Settings.

#include "RaidPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "Components/StaticMeshComponent.h"
#include "RaidPlayerAnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Runtime/Engine/Classes/Components/AudioComponent.h"
#include "RaidPlayerStatComponent.h"
#include "RaidPlayerController.h"
#include "RaidGameMode.h"
#include "Particles/ParticleSystem.h"

// Sets default values
ARaidPlayer::ARaidPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	bReplicates = true;

	Damage = 5000;
	PlayerMaxHP = 10000;
	PlayerHP = 10000;

	GetCharacterMovement()->JumpZVelocity = 350.0f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	bUseControllerRotationYaw = false;

	// �������� ����
	TPSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("TPSpringArm"));
	TPSpringArm->SetupAttachment(RootComponent);
	TPSpringArm->TargetArmLength = 300.0f;
	TPSpringArm->bUsePawnControlRotation = true;
	TPSpringArm->bInheritPitch = true;
	TPSpringArm->bInheritRoll = true;
	TPSpringArm->bInheritYaw = true;
	TPSpringArm->bDoCollisionTest = true;


	// ī�޶� ����
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(TPSpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// �ִϸ��̼� �������Ʈ �Ӽ�����
	static ConstructorHelpers::FClassFinder<UAnimInstance> PlayerAnim(TEXT("AnimBlueprint'/Game/Characters/BP_RaidPlayerAnim.BP_RaidPlayerAnim_C'")); // _C�� �ٿ� Ŭ���������� ������
	if (PlayerAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(PlayerAnim.Class);
	}

	// ���̷�Ż �޽� ����
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PlayerMesh(TEXT("SkeletalMesh'/Game/TwinbladesAnimsetBase/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin'"));
	if (PlayerMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(PlayerMesh.Object);
	}

	// ���� ť ����
	static ConstructorHelpers::FObjectFinder<USoundCue>Hit_Sound(TEXT("SoundCue'/Game/Sound/hit_Cue.hit_Cue'"));
	if (Hit_Sound.Succeeded())
	{
		HitSound = Hit_Sound.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundCue>Hit_Sound2(TEXT("SoundCue'/Game/Sound/CriticalHit_Cue.CriticalHit_Cue'"));
	if (Hit_Sound2.Succeeded())
	{
		HitSound2 = Hit_Sound2.Object;
	}

	// Ÿ�� ��ƼŬ
	static ConstructorHelpers::FObjectFinder<UParticleSystem> PLAYEREFFECT(TEXT("ParticleSystem'/Game/InfinityBladeEffects/Effects/FX_Treasure/Loot/P_KeyPickup_03.P_KeyPickup_03'"));
	if (PLAYEREFFECT.Succeeded())
	{
		PlayerHitEffect = PLAYEREFFECT.Object;
	}

	// ���� ���� ����
	BladeLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BladeLeft"));
	BladeLeft->SetupAttachment(GetMesh(), TEXT("BladeLeft"));

	BladeRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BladeRight"));
	BladeRight->SetupAttachment(GetMesh(), TEXT("BladeRight"));

	// ĸ�� �ø��� ����
	AttackCheck = CreateDefaultSubobject<UCapsuleComponent>(TEXT("AttackCheck"));
	AttackCheck->SetupAttachment(GetMesh(), TEXT("BladeLeft"));

	// ���� ������Ʈ
	CharacterStat = CreateDefaultSubobject<URaidPlayerStatComponent>(TEXT("CharacterStat"));

	// �޸��� �ӵ� ���
	SprintSpeedMultiplier = 2.0f;

	IsAttacking = false;

	MaxCombo = 3;


}

// Called when the game starts or when spawned
void ARaidPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	GameMode = Cast<ARaidGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	
}

float ARaidPlayer::TakeDamage(float Damage, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		PlayerHP -= ActualDamage;
		if (!IsHit && !IsAttacking && !IsDodge)
		{
		PlayerAnim->PlayHitMontage();
		IsHit = true;
		bCanBeDamaged = false;
		}
		if (PlayerHP <= 0.f)
		{
			GameMode->PlayerDead.Broadcast();
			PlayerDeath();
		}
	}
	return ActualDamage;
}
void ARaidPlayer::PostInitializeComponents()
{	
	Super::PostInitializeComponents();
	PlayerAnim = Cast<URaidPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	
	if (nullptr != PlayerAnim)
	{
		PlayerAnim->OnMontageEnded.AddDynamic(this, &ARaidPlayer::OnAttackMontageEnded);
		PlayerAnim->OnNextAttackCheck.AddUObject(this, &ARaidPlayer::ComboServer);
		PlayerAnim->DodgeEnd.AddUObject(this, &ARaidPlayer::DodgeEndState);
	}
	AttackCheck->OnComponentBeginOverlap.AddDynamic(this, &ARaidPlayer::AttackCheckOverlap);

}

// Called every frame
void ARaidPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARaidPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARaidPlayer::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ARaidPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ARaidPlayer::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARaidPlayer::StartSprintServer);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARaidPlayer::StopSprintServer);

	PlayerInputComponent->BindAction("Attack", IE_Released, this, &ARaidPlayer::AttackServer);
	PlayerInputComponent->BindAction("Attack2", IE_Released, this, &ARaidPlayer::AttackServer2);

	PlayerInputComponent->BindAction("Dodge", IE_Released, this, &ARaidPlayer::DodgeServer);

}

void ARaidPlayer::Jump()
{
	if (IsDodge || IsAttacking|| IsHit ) return;
	bPressedJump = true;
	JumpKeyHoldTime = 0.0f;
}

void ARaidPlayer::StopJumping()
{
	bPressedJump = false;
	ResetJumpState();
}

void ARaidPlayer::PlayerDeath()
{
	IsDeath = true;
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlayerAnim->PlayDead();
	

	
}


void ARaidPlayer::AttackCheckOverlap(UPrimitiveComponent* OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	FVector OverlapLocation = OverlappedComp->GetComponentLocation();
	FTransform OverlapTransform = OtherActor->GetActorTransform();

	if (OtherActor != this)
	{
		ServerApplyDamage(OtherActor, Damage, this);
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, OverlapLocation);
		UGameplayStatics::PlaySoundAtLocation(this, HitSound2, OverlapLocation);
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PlayerHitEffect, OverlapTransform, true);
	}
}



void ARaidPlayer::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ARaidPlayer::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

////////////////Sprint///////////////////////////////////

void ARaidPlayer::StartSprintServer_Implementation()
{
	StartSprintMulticast();
}

bool ARaidPlayer::StartSprintServer_Validate()
{
	return true;
}

void ARaidPlayer::StartSprintMulticast_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed *= SprintSpeedMultiplier;
	IsRun = true;
}

void ARaidPlayer::StopSprintServer_Implementation()
{
	StopSprintMulticast();
}

bool ARaidPlayer::StopSprintServer_Validate()
{
	return true;
}

void ARaidPlayer::StopSprintMulticast_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	IsRun = false;
}

//////////// Attack //////////////////////////////////////////

void ARaidPlayer::AttackServer_Implementation()
{
	AttackMulticast();
}

bool ARaidPlayer::AttackServer_Validate()
{
	return true;
}

void ARaidPlayer::AttackMulticast_Implementation()
{
	IsInAir = GetCharacterMovement()->IsFalling();
	if (IsInAir || IsDeath || IsHit) return;

	if (IsAttacking&&IsDodge) // OnNextAttackCheck ���� ���ݽ� IsComboInputOn = true �� ���� ���ݼ������� �̾���
	{
		//CHECK(FMath::IsWithinInclusive<int32>(CurrentCombo, 1, MaxCombo));
		if (CanNextCombo)
		{
				IsComboInputOn = true;
				LOG(Warning, TEXT("Input:%s"), IsComboInputOn ? TEXT("true") : TEXT("False"));
		}
	}
	else // ù��° �޺� �߻�
	{
		CHECK(CurrentCombo == 0);
		AttackStartComboState();
		PlayerAnim->PlayAttackMontage();
		PlayerAnim->JumpToAttackMontageSection(CurrentCombo);
		IsAttacking = true;
		IsDodge = true;
	}
	

}
///////////Attack2/////////////////////////////////////////////

void ARaidPlayer::AttackServer2_Implementation()
{
	AttackMulticast2();
}

bool ARaidPlayer::AttackServer2_Validate()
{
	return true;
}

void ARaidPlayer::AttackMulticast2_Implementation()
{
	IsInAir = GetCharacterMovement()->IsFalling();
	if (IsInAir || IsDeath || IsHit) return;

	if (!IsAttacking&&!IsDodge)
	{
		if (IsRun)
		{
			Damage *= 1.5 ;
			LOG(Warning, TEXT("Damage:%f"), Damage);
			PlayerAnim->PlayDashAttack();
		}
		else
		{
			Damage *= 2.8;
			LOG(Warning, TEXT("Damage:%f"), Damage);
			PlayerAnim->PlayAttackMontage2();
		}
	}
	IsAttacking = true;
	IsDodge = true;
}


/////////////Combo///////////////////////////////////////////

void ARaidPlayer::ComboServer_Implementation()
{
	ComboMulticast();
}

bool ARaidPlayer::ComboServer_Validate()
{
	return true;
}

void ARaidPlayer::ComboMulticast_Implementation()
{
	CanNextCombo = false;

	if (IsComboInputOn)
	{
		AttackStartComboState();
		PlayerAnim->JumpToAttackMontageSection(CurrentCombo);
	}

}

////////////////////ServerApplyDamage////////////////////////////

void ARaidPlayer::ServerApplyDamage_Implementation(AActor* DamagedActor ,float Damamge,AActor* DamageCauser)
{
	UGameplayStatics::ApplyDamage(DamagedActor, Damage, nullptr, DamageCauser, nullptr);
	AttackCheck->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ARaidPlayer::ServerApplyDamage_Validate(AActor* DamagedActor, float Damamge, AActor* DamageCauser)
{
	return true;
}

///////////////Dodge////////////////////////////////////////////

void ARaidPlayer::DodgeServer_Implementation()
{
	DodgeMulticast();
}

bool ARaidPlayer::DodgeServer_Validate()
{
	return true;
}

void ARaidPlayer::DodgeMulticast_Implementation()
{
	IsInAir = GetCharacterMovement()->IsFalling();
	if (IsInAir || IsDeath || IsHit) return;
	

	bCanBeDamaged = false;

	if (!IsDodge && !IsAttacking)
	PlayerAnim->PlayDodge();

	
	IsDodge = true;
	IsAttacking = true;
}


///////////////////////////////////////////////////////////////

void ARaidPlayer::AttackStartComboState()
{
	CanNextCombo = true;
	IsComboInputOn = false;
	CHECK(FMath::IsWithinInclusive<int32>(CurrentCombo, 0, MaxCombo - 1));
	
	CurrentCombo = FMath::Clamp<int32>(CurrentCombo + 1, 1, MaxCombo);
	Damage += Damage * 0.2;
	LOG(Warning, TEXT("Damage:%f"), Damage);
	
}



void ARaidPlayer::AttackEndComboState() // ���� ��Ÿ�� ����� �޺����� �ʱ�ȭ
{
	IsComboInputOn = false;
	CanNextCombo = false;
	CurrentCombo = 0;
}

void ARaidPlayer::OnCollStart() // ��Ƽ���� �߻��� ���� �ø��� üũ
{
	AttackCheck->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ARaidPlayer::OnCollEnd()
{
	AttackCheck->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARaidPlayer::DodgeEndState()
{
	bCanBeDamaged = true;
	
	IsDodge = false;
	IsAttacking = false;
}

void ARaidPlayer::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	
	//CHECK(IsAttacking);
	//CHECK(CurrentCombo > 0);
	IsAttacking = false;
	IsDodge = false;
	IsHit = false;
	bCanBeDamaged = true;

	AttackEndComboState();
	AttackCheck->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Damage = 5000;
}



//Property Replicate
void ARaidPlayer::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ARaidPlayer, CanNextCombo);
	//DOREPLIFETIME(ARaidPlayer, IsComboInputOn);
	
	
	
}