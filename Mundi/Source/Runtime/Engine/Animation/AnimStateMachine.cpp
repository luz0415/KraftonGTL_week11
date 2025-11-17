#include "pch.h"
#include "AnimStateMachine.h"
#include "AnimSequence.h"

IMPLEMENT_CLASS(UAnimStateMachine)

UAnimStateMachine::UAnimStateMachine()
{
	// 상태별 애니메이션 배열 초기화
	StateAnimations.SetNum(static_cast<int32>(EAnimState::MAX));
	for (int32 i = 0; i < StateAnimations.Num(); ++i)
	{
		StateAnimations[i] = nullptr;
	}
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
 * @brief 특정 상태의 애니메이션 가져오기
 */
UAnimSequence* UAnimStateMachine::GetStateAnimation(EAnimState State) const
{
	int32 StateIndex = static_cast<int32>(State);
	if (StateIndex >= 0 && StateIndex < StateAnimations.Num())
	{
		return StateAnimations[StateIndex];
	}
	return nullptr;
}

/**
 * @brief 특정 전환의 블렌드 시간 찾기
 */
float UAnimStateMachine::FindTransitionBlendTime(EAnimState FromState, EAnimState ToState) const
{
	for (const FAnimStateTransition& Transition : Transitions)
	{
		if (Transition.FromState == FromState && Transition.ToState == ToState)
		{
			return Transition.BlendTime;
		}
	}

	// 전환 규칙이 없으면 기본값
	return 0.2f;
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
