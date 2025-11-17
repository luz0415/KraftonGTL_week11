#pragma once

#include "Object.h"
#include "PoseContext.h"
#include"UAnimStateMachine.generated.h"
class APawn;
class ACharacter;
class UCharacterMovementComponent;
class UAnimSequence;

/**
 * @brief 애니메이션 상태 열거형
 *
 * Character의 이동/액션 상태를 나타냄.
 */
enum class EAnimState : uint8
{
	/** 정지 상태 */
	Idle,

	/** 걷기 */
	Walk,

	/** 달리기 */
	Run,

	/** 점프 (상승 중) */
	Jump,

	/** 낙하 (하강 중) */
	Fall,

	/** 비행 */
	Fly,

	/** 앉기 */
	Crouch,

	/** 상태 개수 (마지막) */
	MAX
};

/**
 * @brief 상태 전환 규칙
 *
 * 한 상태에서 다른 상태로 전환할 때의 조건과 블렌딩 시간.
 */
struct FAnimStateTransition
{
	/** 시작 상태 */
	EAnimState FromState;

	/** 목표 상태 */
	EAnimState ToState;

	/** 전환 블렌딩 시간 (초) */
	float BlendTime;

	FAnimStateTransition()
		: FromState(EAnimState::Idle)
		, ToState(EAnimState::Idle)
		, BlendTime(0.2f)
	{
	}

	FAnimStateTransition(EAnimState From, EAnimState To, float InBlendTime)
		: FromState(From)
		, ToState(To)
		, BlendTime(InBlendTime)
	{
	}
};

/**
 * @brief 애니메이션 State Machine
 *
 * Character의 이동 상태에 따라 자동으로 애니메이션을 전환.
 * Phase 1의 블렌딩 시스템을 활용하여 부드러운 전환 제공.
 *
 * 사용 예시:
 * ```cpp
 * UAnimStateMachine* SM = NewObject<UAnimStateMachine>();
 * SM->Initialize(MyCharacter);
 * SM->RegisterStateAnimation(EAnimState::Idle, IdleAnim);
 * SM->RegisterStateAnimation(EAnimState::Walk, WalkAnim);
 * SM->RegisterStateAnimation(EAnimState::Run, RunAnim);
 * ```
 */
class UAnimStateMachine : public UObject
{
	GENERATED_REFLECTION_BODY()

public:
	UAnimStateMachine();
	virtual ~UAnimStateMachine() override = default;

	// ===== Phase 3: State Machine 메서드 =====

	// ===== 애셋 데이터 편집 (에디터/런타임 공통) =====

	/**
	 * @brief 상태별 애니메이션 등록
	 *
	 * @param State 애니메이션 상태
	 * @param Animation 재생할 애니메이션 시퀀스
	 */
	void RegisterStateAnimation(EAnimState State, UAnimSequence* Animation);

	/**
	 * @brief 전환 규칙 추가
	 *
	 * @param Transition 전환 규칙 구조체
	 */
	void AddTransition(const FAnimStateTransition& Transition);

	// ===== 애셋 데이터 조회 =====

	/**
	 * @brief 특정 상태의 애니메이션 가져오기
	 *
	 * @param State 상태
	 * @return 해당 상태의 애니메이션 시퀀스
	 */
	UAnimSequence* GetStateAnimation(EAnimState State) const;

	/**
	 * @brief 모든 전환 규칙 가져오기
	 */
	const TArray<FAnimStateTransition>& GetTransitions() const { return Transitions; }

	/**
	 * @brief 특정 전환의 블렌드 시간 찾기
	 *
	 * @param FromState 시작 상태
	 * @param ToState 목표 상태
	 * @return 블렌드 시간 (전환 규칙이 없으면 기본값 0.2f)
	 */
	float FindTransitionBlendTime(EAnimState FromState, EAnimState ToState) const;

protected:
	// ===== 애셋 데이터 (공유 가능) =====

	/** 상태 → 애니메이션 매핑 */
	TArray<UAnimSequence*> StateAnimations;  // EAnimState를 인덱스로 사용

	/** 전환 규칙들 */
	TArray<FAnimStateTransition> Transitions;

	// ===== 헬퍼 메서드 =====

	/**
	 * @brief 상태 이름 가져오기 (디버깅용)
	 */
	static const char* GetStateName(EAnimState State);
};
