#include "pch.h"
#include "AnimStateMachine.h"
#include "AnimationRuntime.h"
#include "AnimSequence.h"
#include "AnimDataModel.h"
#include "Source/Runtime/Engine/Animation/PoseContext.h"
#include "Source/Runtime/Engine/GameFramework/Pawn.h"
#include "Source/Runtime/Engine/GameFramework/Character.h"
#include "Source/Runtime/Engine/Components/CharacterMovementComponent.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"

IMPLEMENT_CLASS(UAnimStateMachine)

UAnimStateMachine::UAnimStateMachine()
	: CurrentState(EAnimState::Idle)
	, PreviousState(EAnimState::Idle)
	, bIsTransitioning(false)
	, TransitionTime(0.0f)
	, TransitionDuration(0.3f)
	, CurrentAnimTime(0.0f)
	, PreviousAnimTime(0.0f)
	, OwnerPawn(nullptr)
	, OwnerCharacter(nullptr)
	, MovementComponent(nullptr)
	, WalkSpeed(30.0f)
	, RunSpeed(60.0f)
{
	// 상태별 애니메이션 배열 초기화
	StateAnimations.SetNum(static_cast<int32>(EAnimState::MAX));
	for (int32 i = 0; i < StateAnimations.Num(); ++i)
	{
		StateAnimations[i] = nullptr;
	}
}

/**
 * @brief State Machine 초기화
 */
void UAnimStateMachine::Initialize(APawn* InPawn)
{
	OwnerPawn = InPawn;

	// Character로 캐스팅 시도
	OwnerCharacter = Cast<ACharacter>(InPawn);
	if (OwnerCharacter)
	{
		// MovementComponent 캐싱
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}

	// 초기 상태 결정
	CurrentState = DetermineStateFromMovement();
	PreviousState = CurrentState;
}

/**
 * @brief 매 프레임 상태 업데이트
 */
void UAnimStateMachine::UpdateState(float DeltaSeconds)
{
	if (!OwnerPawn)
	{
		return;
	}

	// 현재 애니메이션 시간 업데이트
	UAnimSequence* CurrentAnim = GetCurrentAnimation();
	if (CurrentAnim)
	{
		CurrentAnimTime += DeltaSeconds;

		// 루프 처리
		float AnimLength = CurrentAnim->GetPlayLength();
		while (CurrentAnimTime >= AnimLength)
		{
			CurrentAnimTime -= AnimLength;
		}
	}

	// 전환 중이면 블렌딩 진행
	if (bIsTransitioning)
	{
		TransitionTime += DeltaSeconds;

		// 이전 애니메이션 시간도 업데이트 (블렌딩용)
		int32 PrevStateIndex = static_cast<int32>(PreviousState);
		UAnimSequence* PrevAnim = (PrevStateIndex >= 0 && PrevStateIndex < StateAnimations.Num())
			? StateAnimations[PrevStateIndex]
			: nullptr;

		if (PrevAnim)
		{
			PreviousAnimTime += DeltaSeconds;
			float PrevAnimLength = PrevAnim->GetPlayLength();
			while (PreviousAnimTime >= PrevAnimLength)
			{
				PreviousAnimTime -= PrevAnimLength;
			}
		}

		// 전환 완료
		if (TransitionTime >= TransitionDuration)
		{
			bIsTransitioning = false;
			PreviousState = CurrentState;
			TransitionTime = 0.0f;
		}

		return;
	}

	// 전환 가능한 상태 체크
	CheckTransitions();
}

/**
 * @brief 상태별 애니메이션 등록
 */
void UAnimStateMachine::RegisterStateAnimation(EAnimState State, UAnimSequence* Animation)
{
	int32 StateIndex = static_cast<int32>(State);
	if (StateIndex >= 0 && StateIndex < StateAnimations.Num())
	{
		StateAnimations[StateIndex] = Animation;
	}
}

/**
 * @brief 전환 규칙 추가
 */
void UAnimStateMachine::AddTransition(const FAnimStateTransition& Transition)
{
	Transitions.Add(Transition);
}

/**
 * @brief 현재 재생 중인 애니메이션 가져오기
 */
UAnimSequence* UAnimStateMachine::GetCurrentAnimation() const
{
	int32 StateIndex = static_cast<int32>(CurrentState);
	if (StateIndex >= 0 && StateIndex < StateAnimations.Num())
	{
		return StateAnimations[StateIndex];
	}
	return nullptr;
}

/**
 * @brief 전환 진행도 가져오기 (0~1)
 */
float UAnimStateMachine::GetTransitionAlpha() const
{
	if (!bIsTransitioning || TransitionDuration <= 0.0f)
	{
		return 1.0f;
	}

	return FMath::Clamp(TransitionTime / TransitionDuration, 0.0f, 1.0f);
}

/**
 * @brief 현재 포즈 평가 (블렌딩 적용)
 *
 * Phase 1의 BlendTwoPosesTogether를 사용하여 부드러운 전환.
 */
void UAnimStateMachine::EvaluateCurrentPose(FPoseContext& OutPose)
{
	static int32 LogCounter = 0;

	// 전환 중이 아니면 현재 상태 애니메이션만 반환
	if (!bIsTransitioning)
	{
		UAnimSequence* CurrentAnim = GetCurrentAnimation();
		if (CurrentAnim)
		{
			if (LogCounter++ % 60 == 0)
			{
				UE_LOG("[AnimStateMachine] Evaluating pose - State: %s, Time: %.2f",
					GetStateName(CurrentState), CurrentAnimTime);
			}

			const FSkeleton* Skeleton = CurrentAnim->GetDataModel()->GetSkeleton();
			OutPose.Initialize(Skeleton);

			// 현재 시간에서 포즈 샘플링
			const int32 NumBones = Skeleton->Bones.Num();
			for (int32 BoneIdx = 0; BoneIdx < NumBones; ++BoneIdx)
			{
				const FBone& Bone = Skeleton->Bones[BoneIdx];
				FVector Position;
				FQuat Rotation;
				FVector Scale;

				if (CurrentAnim->GetBoneTransformAtTime(Bone.Name, CurrentAnimTime, Position, Rotation, Scale))
				{
					OutPose.LocalSpacePose[BoneIdx] = FTransform(Position, Rotation, Scale);
				}
				else
				{
					// 샘플링 실패 시 BindPose 사용
					OutPose.LocalSpacePose[BoneIdx] = FTransform(Bone.BindPose);
				}
			}
		}
		return;
	}

	// 전환 중이면 두 상태 블렌딩
	int32 PrevStateIndex = static_cast<int32>(PreviousState);
	int32 CurrStateIndex = static_cast<int32>(CurrentState);

	UAnimSequence* PrevAnim = (PrevStateIndex >= 0 && PrevStateIndex < StateAnimations.Num())
		? StateAnimations[PrevStateIndex]
		: nullptr;

	UAnimSequence* CurrAnim = (CurrStateIndex >= 0 && CurrStateIndex < StateAnimations.Num())
		? StateAnimations[CurrStateIndex]
		: nullptr;

	if (!PrevAnim || !CurrAnim)
	{
		return;
	}

	const FSkeleton* Skeleton = CurrAnim->GetDataModel()->GetSkeleton();
	const int32 NumBones = Skeleton->Bones.Num();

	// 두 포즈 평가
	FPoseContext PrevPose, CurrPose;
	PrevPose.Initialize(Skeleton);
	CurrPose.Initialize(Skeleton);

	// 이전 애니메이션에서 포즈 샘플링
	for (int32 BoneIdx = 0; BoneIdx < NumBones; ++BoneIdx)
	{
		const FBone& Bone = Skeleton->Bones[BoneIdx];
		FVector Position;
		FQuat Rotation;
		FVector Scale;

		if (PrevAnim->GetBoneTransformAtTime(Bone.Name, PreviousAnimTime, Position, Rotation, Scale))
		{
			PrevPose.LocalSpacePose[BoneIdx] = FTransform(Position, Rotation, Scale);
		}
		else
		{
			PrevPose.LocalSpacePose[BoneIdx] = FTransform(Bone.BindPose);
		}
	}

	// 현재 애니메이션에서 포즈 샘플링
	for (int32 BoneIdx = 0; BoneIdx < NumBones; ++BoneIdx)
	{
		const FBone& Bone = Skeleton->Bones[BoneIdx];
		FVector Position;
		FQuat Rotation;
		FVector Scale;

		if (CurrAnim->GetBoneTransformAtTime(Bone.Name, CurrentAnimTime, Position, Rotation, Scale))
		{
			CurrPose.LocalSpacePose[BoneIdx] = FTransform(Position, Rotation, Scale);
		}
		else
		{
			CurrPose.LocalSpacePose[BoneIdx] = FTransform(Bone.BindPose);
		}
	}

	// Phase 1 블렌딩 사용!
	float BlendAlpha = GetTransitionAlpha();
	FAnimationRuntime::BlendTwoPosesTogether(PrevPose, CurrPose, BlendAlpha, OutPose);
}

/**
 * @brief 전환 가능한 상태 체크
 */
void UAnimStateMachine::CheckTransitions()
{
	// Movement 기반 상태 결정
	EAnimState DesiredState = DetermineStateFromMovement();

	// 상태 변경 필요
	if (DesiredState != CurrentState)
	{
		TransitionToState(DesiredState);
	}
}

/**
 * @brief 특정 상태로 전환
 */
void UAnimStateMachine::TransitionToState(EAnimState NewState)
{
	// 이미 같은 상태면 무시
	if (NewState == CurrentState && !bIsTransitioning)
	{
		return;
	}

	// 전환 시작
	PreviousState = CurrentState;
	PreviousAnimTime = CurrentAnimTime;  // 현재 시간을 이전 시간으로 저장
	CurrentState = NewState;
	CurrentAnimTime = 0.0f;  // 새 애니메이션은 처음부터 시작
	bIsTransitioning = true;
	TransitionTime = 0.0f;

	// 기본 전환 시간 (0.2초)
	TransitionDuration = 0.2f;

	// 등록된 전환 규칙 체크하여 블렌딩 시간 조정
	for (const FAnimStateTransition& Trans : Transitions)
	{
		if (Trans.FromState == PreviousState && Trans.ToState == CurrentState)
		{
			TransitionDuration = Trans.BlendTime;
			break;
		}
	}
}

/**
 * @brief 현재 Movement 상태 기반으로 AnimState 결정
 */
EAnimState UAnimStateMachine::DetermineStateFromMovement()
{
	if (!OwnerPawn)
	{
		return EAnimState::Idle;
	}

	// MovementComponent가 있는 경우 (Character)
	if (MovementComponent && OwnerCharacter)
	{
		FVector Velocity = OwnerCharacter->GetVelocity();
		float Speed = Velocity.Size();

		static int32 SpeedLogCounter = 0;
		if (SpeedLogCounter++ % 60 == 0)
		{
			UE_LOG("[AnimStateMachine] Speed: %.2f (WalkSpeed: %.2f)", Speed, WalkSpeed);
		}

		// Falling 체크를 제외하고 속도로만 판단 (임시 해결책)
		// 속도 기반 상태 결정
		if (Speed < 1.0f)
		{
			return EAnimState::Idle;
		}
		else if (Speed <= WalkSpeed)
		{
			return EAnimState::Walk;
		}
		else
		{
			return EAnimState::Run;
		}

		// [주석처리] MovementMode 기반 판단은 나중에 Falling 처리가 제대로 될 때 사용
		/*
		EMovementMode MovementMode = MovementComponent->GetMovementMode();

		// 이동 모드에 따른 상태 결정
		switch (MovementMode)
		{
		case EMovementMode::Walking:
		{
			// Crouch 체크 (향후 확장)
			// if (MovementComponent->IsCrouching())
			//     return EAnimState::Crouch;

			// 속도 기반 상태 결정
			if (Speed < 1.0f)
			{
				return EAnimState::Idle;
			}
			else if (Speed <= WalkSpeed)
			{
				return EAnimState::Walk;
			}
			else
			{
				return EAnimState::Run;
			}
		}*/

		/* case EMovementMode::Falling:
		{
			// 상승 중이면 Jump, 하강 중이면 Fall
			// 애니메이션이 없으면 Idle로 폴백
			if (Velocity.Z > 0.0f)
			{
				// Jump 애니메이션이 등록되어 있으면 Jump, 아니면 Idle
				int32 JumpIdx = static_cast<int32>(EAnimState::Jump);
				if (JumpIdx < StateAnimations.Num() && StateAnimations[JumpIdx])
					return EAnimState::Jump;
				else
					return EAnimState::Idle;
			}
			else
			{
				// Fall 애니메이션이 등록되어 있으면 Fall, 아니면 Idle
				int32 FallIdx = static_cast<int32>(EAnimState::Fall);
				if (FallIdx < StateAnimations.Num() && StateAnimations[FallIdx])
					return EAnimState::Fall;
				else
					return EAnimState::Idle;
			}
		}

		case EMovementMode::Flying:
		{
			// Fly 애니메이션이 등록되어 있으면 Fly, 아니면 Idle
			int32 FlyIdx = static_cast<int32>(EAnimState::Fly);
			if (FlyIdx < StateAnimations.Num() && StateAnimations[FlyIdx])
				return EAnimState::Fly;
			else
				return EAnimState::Idle;
		}

		case EMovementMode::None:
		default:
		{
			return EAnimState::Idle;
		}
		}*/
	}

	// MovementComponent 없으면 Idle (APawn은 GetVelocity 없음)
	if (!OwnerCharacter)
	{
		return EAnimState::Idle;
	}

	// Character의 속도 기반
	FVector Velocity = OwnerCharacter->GetVelocity();
	float Speed = Velocity.Size();

	if (Speed < 1.0f)
	{
		return EAnimState::Idle;
	}
	else if (Speed <= WalkSpeed)
	{
		return EAnimState::Walk;
	}
	else
	{
		return EAnimState::Run;
	}
}

/**
 * @brief 상태 이름 가져오기 (디버깅용)
 */
const char* UAnimStateMachine::GetStateName(EAnimState State)
{
	switch (State)
	{
	case EAnimState::Idle:   return "Idle";
	case EAnimState::Walk:   return "Walk";
	case EAnimState::Run:    return "Run";
	case EAnimState::Jump:   return "Jump";
	case EAnimState::Fall:   return "Fall";
	case EAnimState::Fly:    return "Fly";
	case EAnimState::Crouch: return "Crouch";
	default:                 return "Unknown";
	}
}
