#pragma once
#include "SWindow.h"
#include "Source/Runtime/Core/Math/Vector.h"

class UBlendSpace2D;
class UAnimSequence;
class UWorld;
class FViewport;
class ASkeletalMeshActor;
class USkeletalMeshComponent;
class ViewerState;

/**
 * @brief Blend Space 2D 에디터 윈도우
 *
 * 2D 그리드 상에서 애니메이션 샘플 포인트를 시각적으로 배치하고 편집할 수 있는 에디터입니다.
 */
class SBlendSpace2DEditorWindow : public SWindow
{
public:
	SBlendSpace2DEditorWindow();
	virtual ~SBlendSpace2DEditorWindow();

	/**
	 * @brief 윈도우 초기화
	 */
	bool Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice);

	/**
	 * @brief 편집할 BlendSpace 설정
	 */
	void SetBlendSpace(UBlendSpace2D* InBlendSpace);

	/**
	 * @brief 현재 편집 중인 BlendSpace 반환
	 */
	UBlendSpace2D* GetBlendSpace() const { return EditingBlendSpace; }

	/**
	 * @brief 렌더링
	 */
	virtual void OnRender() override;

	/**
	 * @brief 뷰포트 렌더링
	 */
	void OnRenderViewport();

	/**
	 * @brief 업데이트
	 */
	void OnUpdate(float DeltaSeconds);

	/**
	 * @brief 마우스 입력 처리
	 */
	virtual void OnMouseMove(FVector2D MousePos) override;
	virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
	virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;

	/**
	 * @brief 윈도우가 열려있는지 확인
	 */
	bool IsOpen() const { return bIsOpen; }

	/**
	 * @brief 윈도우 닫기
	 */
	void Close() { bIsOpen = false; }

private:
	// ===== UI 렌더링 =====
	void RenderPreviewViewport();      // 상단: 애니메이션 프리뷰
	void RenderGridEditor();           // 하단 왼쪽: 2D 그리드
	void RenderAnimationList();        // 하단 오른쪽: 애니메이션 시퀀스 목록

	void RenderGrid();
	void RenderSamplePoints();
	void RenderPreviewMarker();
	void RenderAxisLabels();
	void RenderToolbar();
	void RenderSampleList();
	void RenderProperties();

	// ===== 좌표 변환 =====
	ImVec2 ParamToScreen(FVector2D Param) const;
	FVector2D ScreenToParam(ImVec2 ScreenPos) const;

	// ===== 입력 처리 =====
	void HandleMouseInput();
	void HandleKeyboardInput();

	// ===== 샘플 관리 =====
	void AddSampleAtPosition(FVector2D Position);
	void RemoveSelectedSample();
	void SelectSample(int32 Index);

	// ===== 데이터 =====
	UBlendSpace2D* EditingBlendSpace = nullptr;
	ID3D11Device* Device = nullptr;

	// ===== 프리뷰 뷰포트 (ViewerState 재활용) =====
	ViewerState* PreviewState = nullptr;

	// ===== 애니메이션 시퀀스 목록 =====
	TArray<UAnimSequence*> AvailableAnimations;  // 로드된 모든 애니메이션
	int32 SelectedAnimationIndex = -1;           // 선택된 애니메이션 인덱스

	// ===== UI 상태 =====
	bool bIsOpen = true;
	ImVec2 CanvasPos;
	ImVec2 CanvasSize;
	int32 SelectedSampleIndex = -1;
	bool bDraggingSample = false;
	FVector2D PreviewParameter = FVector2D(0.0f, 0.0f);
	FRect PreviewViewportRect = FRect(0, 0, 0, 0);  // 프리뷰 뷰포트 영역

	// ===== UI 설정 =====
	static constexpr float GridCellSize = 40.0f;
	static constexpr float SamplePointRadius = 8.0f;
	static constexpr float PreviewMarkerRadius = 12.0f;

	// ===== 색상 =====
	ImU32 GridColor = IM_COL32(80, 80, 80, 255);
	ImU32 AxisColor = IM_COL32(150, 150, 150, 255);
	ImU32 SampleColor = IM_COL32(255, 200, 0, 255);
	ImU32 SelectedSampleColor = IM_COL32(255, 100, 0, 255);
	ImU32 PreviewColor = IM_COL32(0, 255, 0, 255);
};
