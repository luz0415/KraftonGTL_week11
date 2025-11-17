#pragma once

#include "AnimStateMachine.h"
#include "PoseContext.h"

class APawn;
class ACharacter;
class UCharacterMovementComponent;

/**
 * @brief State Machine 실행 노드
 *
 * UAnimStateMachine 애셋을 실행하는 인스턴스.
 * 현재 상태, 전환 진행도, 재생 시간 등 실행 데이터를 관리합니다.
 *
 * 언리얼의 FAnimNode_StateMachine과 유사한 구조.
 */
class FAnimNode_StateMachine
{
public:
	FAnimNode_StateMachine();
	~FAnimNode_StateMachine() = default;

	// ===== 초기화 및 업데이트 =====

	/**
	 * @brief 노드 초기화
	 *
	 * @param InPawn 소유 Pawn
	 */
	void Initialize(APawn* InPawn);

	/**
	 * @brief 매 프레임 업데이트
	 *
	 * Character의 Movement 상태를 체크하여 상태 전환.
	 *
	 * @param DeltaSeconds 델타 타임
	 */
	void Update(float DeltaSeconds);

	/**
	 * @brief 현재 포즈 평가 (블렌딩 적용)
	 *
	 * @param OutPose 출력 포즈
	 */
	void Evaluate(FPoseContext& OutPose);

	// ===== 애셋 설정 =====

	/**
	 * @brief State Machine 애셋 설정
	 *
	 * @param InStateMachine 사용할 State Machine 애셋
	 */
	void SetStateMachine(UAnimStateMachine* InStateMachine);

	/**
	 * @brief State Machine 애셋 가져오기
	 */
	UAnimStateMachine* GetStateMachine() const { return StateMachine; }

	// ===== 실행 상태 조회 =====

	/**
	 * @brief 현재 상태 가져오기
	 */
	EAnimState GetCurrentState() const { return CurrentState; }

	/**
	 * @brief 전환 중인지 확인
	 */
	bool IsTransitioning() const { return bIsTransitioning; }

	/**
	 * @brief 전환 진행도 가져오기 (0~1)
	 */
	float GetTransitionAlpha() const;

	/**
	 * @brief 현재 재생 중인 애니메이션 가져오기
	 */
	UAnimSequence* GetCurrentAnimation() const;

protected:
	// ===== 애셋 참조 =====

	/** State Machine 애셋 */
	UAnimStateMachine* StateMachine;

	// ===== 실행 상태 (인스턴스 데이터) =====

	/** 현재 상태 */
	EAnimState CurrentState;

	/** 이전 상태 (전환용) */
	EAnimState PreviousState;

	/** 전환 중인지 여부 */
	bool bIsTransitioning;

	/** 전환 경과 시간 */
	float TransitionTime;

	/** 전환 총 시간 */
	float TransitionDuration;

	/** 현재 상태 애니메이션 재생 시간 */
	float CurrentAnimTime;

	/** 이전 상태 애니메이션 재생 시간 (블렌딩용) */
	float PreviousAnimTime;

	// ===== 소유자 정보 =====

	/** 소유 Pawn */
	APawn* OwnerPawn;

	/** 소유 Character (캐싱) */
	ACharacter* OwnerCharacter;

	/** Character Movement Component (캐싱) */
	UCharacterMovementComponent* MovementComponent;

	// ===== 상태 판별 임계값 =====

	/** 걷기 최대 속도 */
	float WalkSpeed;

	/** 달리기 최소 속도 */
	float RunSpeed;

	// ===== 내부 메서드 =====

	/**
	 * @brief 전환 가능한 상태 체크
	 */
	void CheckTransitions();

	/**
	 * @brief 특정 상태로 전환
	 *
	 * @param NewState 새로운 상태
	 */
	void TransitionToState(EAnimState NewState);

	/**
	 * @brief 현재 Movement 상태 기반으로 AnimState 결정
	 */
	EAnimState DetermineStateFromMovement();
};
