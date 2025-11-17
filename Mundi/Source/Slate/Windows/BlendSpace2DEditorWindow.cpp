#include "pch.h"
#include "BlendSpace2DEditorWindow.h"
#include "Source/Runtime/Engine/Animation/BlendSpace2D.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Animation/AnimDataModel.h"
#include "Source/Runtime/InputCore/InputManager.h"
#include "Source/Editor/FBXLoader.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/SkeletalViewer/SkeletalViewerBootstrap.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"
#include "Source/Runtime/Renderer/FViewport.h"
#include"FViewportClient.h"
#include "Math.h"

SBlendSpace2DEditorWindow::SBlendSpace2DEditorWindow()
	: CanvasPos(ImVec2(0, 0))
	, CanvasSize(ImVec2(600, 600))
{
}

SBlendSpace2DEditorWindow::~SBlendSpace2DEditorWindow()
{
	// ViewerState 정리 (SkeletalViewerBootstrap 사용)
	if (PreviewState)
	{
		SkeletalViewerBootstrap::DestroyViewerState(PreviewState);
	}
}

bool SBlendSpace2DEditorWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice)
{
	Device = InDevice;
	SetRect(StartX, StartY, StartX + Width, StartY + Height);
	bIsOpen = true;

	// ViewerState 생성 (SkeletalViewerBootstrap 재활용)
	PreviewState = SkeletalViewerBootstrap::CreateViewerState("BlendSpace2D Preview", InWorld, InDevice);
	if (!PreviewState)
	{
		return false;
	}

	return true;
}

void SBlendSpace2DEditorWindow::SetBlendSpace(UBlendSpace2D* InBlendSpace)
{
	EditingBlendSpace = InBlendSpace;
	SelectedSampleIndex = -1;
	bDraggingSample = false;

	// 기본 프리뷰 파라미터 설정 (중앙)
	if (EditingBlendSpace)
	{
		PreviewParameter.X = (EditingBlendSpace->XAxisMin + EditingBlendSpace->XAxisMax) * 0.5f;
		PreviewParameter.Y = (EditingBlendSpace->YAxisMin + EditingBlendSpace->YAxisMax) * 0.5f;
	}
}

void SBlendSpace2DEditorWindow::OnRender()
{
	if (!bIsOpen)
	{
		return;
	}

	if (!EditingBlendSpace)
	{
		// BlendSpace가 없으면 간단한 메시지만 표시
		if (ImGui::Begin("Blend Space 2D Editor", &bIsOpen))
		{
			ImGui::Text("No Blend Space loaded.");
			ImGui::Text("Please create or select a Blend Space 2D asset.");
		}
		ImGui::End();
		return;
	}

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("Blend Space 2D Editor", &bIsOpen, flags))
	{
		// 툴바
		RenderToolbar();
		ImGui::Separator();

		// 전체 레이아웃 크기 계산
		ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
		float TopHeight = ContentRegion.y * 0.6f;  // 상단 60%
		float BottomHeight = ContentRegion.y * 0.4f; // 하단 40%

		// ===== 상단: 애니메이션 프리뷰 뷰포트 (60%) =====
		ImGui::BeginChild("BlendSpace2DViewport", ImVec2(0, TopHeight), true, ImGuiWindowFlags_NoScrollbar);
		{
			RenderPreviewViewport();
		}
		ImGui::EndChild();

		// ===== 하단: 그리드 에디터 + 애니메이션 리스트 (40%) =====
		ImGui::BeginChild("BottomArea", ImVec2(0, BottomHeight), false, ImGuiWindowFlags_NoScrollbar);
		{
			float BottomLeftWidth = ContentRegion.x * 0.65f;  // 왼쪽 65%
			float BottomRightWidth = ContentRegion.x * 0.35f; // 오른쪽 35%

			// 왼쪽: 2D 블렌드 그리드
			ImGui::BeginChild("GridEditor", ImVec2(BottomLeftWidth, 0), true);
			{
				RenderGridEditor();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// 오른쪽: 애니메이션 시퀀스 목록
			ImGui::BeginChild("AnimationList", ImVec2(BottomRightWidth, 0), true);
			{
				RenderAnimationList();
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
	}
	ImGui::End();

	HandleKeyboardInput();
}

void SBlendSpace2DEditorWindow::RenderGrid()
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// 그리드 라인 그리기
	for (float x = 0; x <= CanvasSize.x; x += GridCellSize)
	{
		ImVec2 p1(CanvasPos.x + x, CanvasPos.y);
		ImVec2 p2(CanvasPos.x + x, CanvasPos.y + CanvasSize.y);
		DrawList->AddLine(p1, p2, GridColor, 1.0f);
	}

	for (float y = 0; y <= CanvasSize.y; y += GridCellSize)
	{
		ImVec2 p1(CanvasPos.x, CanvasPos.y + y);
		ImVec2 p2(CanvasPos.x + CanvasSize.x, CanvasPos.y + y);
		DrawList->AddLine(p1, p2, GridColor, 1.0f);
	}

	// 중앙 축 (더 굵게)
	ImVec2 centerX(CanvasPos.x + CanvasSize.x * 0.5f, CanvasPos.y);
	ImVec2 centerXEnd(CanvasPos.x + CanvasSize.x * 0.5f, CanvasPos.y + CanvasSize.y);
	DrawList->AddLine(centerX, centerXEnd, AxisColor, 2.0f);

	ImVec2 centerY(CanvasPos.x, CanvasPos.y + CanvasSize.y * 0.5f);
	ImVec2 centerYEnd(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y * 0.5f);
	DrawList->AddLine(centerY, centerYEnd, AxisColor, 2.0f);
}

void SBlendSpace2DEditorWindow::RenderSamplePoints()
{
	if (!EditingBlendSpace)
		return;

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	for (int32 i = 0; i < EditingBlendSpace->GetNumSamples(); ++i)
	{
		const FBlendSample& Sample = EditingBlendSpace->Samples[i];
		ImVec2 ScreenPos = ParamToScreen(Sample.Position);

		ImU32 Color = (i == SelectedSampleIndex) ? SelectedSampleColor : SampleColor;

		// 샘플 포인트 (원)
		DrawList->AddCircleFilled(ScreenPos, SamplePointRadius, Color);
		DrawList->AddCircle(ScreenPos, SamplePointRadius, IM_COL32(255, 255, 255, 255), 0, 2.0f);

		// 샘플 이름 (애니메이션 이름)
		if (Sample.Animation)
		{
			FString AnimName = Sample.Animation->GetName();
			ImVec2 TextPos(ScreenPos.x + SamplePointRadius + 5, ScreenPos.y - 8);
			DrawList->AddText(TextPos, IM_COL32(255, 255, 255, 255), AnimName.c_str());
		}
	}
}

void SBlendSpace2DEditorWindow::RenderPreviewMarker()
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 PreviewPos = ParamToScreen(PreviewParameter);

	// 프리뷰 마커 (속이 빈 원)
	DrawList->AddCircle(PreviewPos, PreviewMarkerRadius, PreviewColor, 0, 3.0f);

	// 십자가 마커
	DrawList->AddLine(
		ImVec2(PreviewPos.x - PreviewMarkerRadius, PreviewPos.y),
		ImVec2(PreviewPos.x + PreviewMarkerRadius, PreviewPos.y),
		PreviewColor, 2.0f);

	DrawList->AddLine(
		ImVec2(PreviewPos.x, PreviewPos.y - PreviewMarkerRadius),
		ImVec2(PreviewPos.x, PreviewPos.y + PreviewMarkerRadius),
		PreviewColor, 2.0f);
}

void SBlendSpace2DEditorWindow::RenderAxisLabels()
{
	if (!EditingBlendSpace)
		return;

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// X축 레이블
	char LabelMin[64], LabelMax[64];
	sprintf_s(LabelMin, "%.0f", EditingBlendSpace->XAxisMin);
	sprintf_s(LabelMax, "%.0f", EditingBlendSpace->XAxisMax);

	ImVec2 XMinPos(CanvasPos.x - 20, CanvasPos.y + CanvasSize.y + 10);
	ImVec2 XMaxPos(CanvasPos.x + CanvasSize.x - 20, CanvasPos.y + CanvasSize.y + 10);

	DrawList->AddText(XMinPos, IM_COL32(200, 200, 200, 255), LabelMin);
	DrawList->AddText(XMaxPos, IM_COL32(200, 200, 200, 255), LabelMax);

	// Y축 레이블
	sprintf_s(LabelMin, "%.0f", EditingBlendSpace->YAxisMin);
	sprintf_s(LabelMax, "%.0f", EditingBlendSpace->YAxisMax);

	ImVec2 YMinPos(CanvasPos.x - 40, CanvasPos.y + CanvasSize.y - 10);
	ImVec2 YMaxPos(CanvasPos.x - 40, CanvasPos.y);

	DrawList->AddText(YMinPos, IM_COL32(200, 200, 200, 255), LabelMin);
	DrawList->AddText(YMaxPos, IM_COL32(200, 200, 200, 255), LabelMax);
}

void SBlendSpace2DEditorWindow::RenderToolbar()
{
	static char SavePath[256] = "Data/Animation/MyBlendSpace.blend2d";
	static char LoadPath[256] = "Data/Animation/MyBlendSpace.blend2d";

	// 저장 버튼
	if (ImGui::Button("Save"))
	{
		if (EditingBlendSpace)
		{
			ImGui::OpenPopup("Save BlendSpace");
		}
	}

	// 저장 팝업
	if (ImGui::BeginPopupModal("Save BlendSpace", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Save BlendSpace2D to file:");
		ImGui::InputText("Path", SavePath, sizeof(SavePath));

		if (ImGui::Button("Save", ImVec2(120, 0)))
		{
			if (EditingBlendSpace)
			{
				EditingBlendSpace->SaveToFile(SavePath);
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	// 로드 버튼
	if (ImGui::Button("Load"))
	{
		ImGui::OpenPopup("Load BlendSpace");
	}

	// 로드 팝업
	if (ImGui::BeginPopupModal("Load BlendSpace", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Load BlendSpace2D from file:");
		ImGui::InputText("Path", LoadPath, sizeof(LoadPath));

		if (ImGui::Button("Load", ImVec2(120, 0)))
		{
			UBlendSpace2D* LoadedBS = UBlendSpace2D::LoadFromFile(LoadPath);
			if (LoadedBS)
			{
				SetBlendSpace(LoadedBS);
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	if (ImGui::Button("Add Sample"))
	{
		// 중앙에 샘플 추가
		if (EditingBlendSpace)
		{
			FVector2D CenterPos(
				(EditingBlendSpace->XAxisMin + EditingBlendSpace->XAxisMax) * 0.5f,
				(EditingBlendSpace->YAxisMin + EditingBlendSpace->YAxisMax) * 0.5f
			);
			AddSampleAtPosition(CenterPos);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Remove Selected"))
	{
		RemoveSelectedSample();
	}

	ImGui::SameLine();

	if (ImGui::Button("Clear All"))
	{
		if (EditingBlendSpace)
		{
			EditingBlendSpace->ClearSamples();
			SelectedSampleIndex = -1;
		}
	}
}

void SBlendSpace2DEditorWindow::RenderSampleList()
{
	if (!EditingBlendSpace)
		return;

	ImGui::Text("Sample Points (%d)", EditingBlendSpace->GetNumSamples());
	ImGui::Separator();

	for (int32 i = 0; i < EditingBlendSpace->GetNumSamples(); ++i)
	{
		const FBlendSample& Sample = EditingBlendSpace->Samples[i];

		char Label[256];
		FString AnimName = Sample.Animation ? Sample.Animation->GetName() : "None";
		sprintf_s(Label, "[%d] %.1f, %.1f - %s",
			i,
			Sample.Position.X,
			Sample.Position.Y,
			AnimName.c_str());

		bool bSelected = (i == SelectedSampleIndex);
		if (ImGui::Selectable(Label, bSelected))
		{
			SelectSample(i);
		}
	}
}

void SBlendSpace2DEditorWindow::RenderProperties()
{
	if (!EditingBlendSpace)
		return;

	ImGui::Text("Blend Space Properties");
	ImGui::Separator();

	// 축 범위 편집
	ImGui::InputFloat("X Min", &EditingBlendSpace->XAxisMin);
	ImGui::InputFloat("X Max", &EditingBlendSpace->XAxisMax);
	ImGui::InputFloat("Y Min", &EditingBlendSpace->YAxisMin);
	ImGui::InputFloat("Y Max", &EditingBlendSpace->YAxisMax);

	ImGui::Separator();

	// 선택된 샘플 속성
	if (SelectedSampleIndex >= 0 && SelectedSampleIndex < EditingBlendSpace->GetNumSamples())
	{
		ImGui::Text("Selected Sample Properties");
		FBlendSample& Sample = EditingBlendSpace->Samples[SelectedSampleIndex];

		ImGui::InputFloat("Position X", &Sample.Position.X);
		ImGui::InputFloat("Position Y", &Sample.Position.Y);

		if (Sample.Animation)
		{
			ImGui::Text("Animation: %s", Sample.Animation->GetName().c_str());
			ImGui::Text("Duration: %.2fs", Sample.Animation->GetPlayLength());
		}
		else
		{
			ImGui::Text("Animation: None");
		}
	}
}

ImVec2 SBlendSpace2DEditorWindow::ParamToScreen(FVector2D Param) const
{
	if (!EditingBlendSpace)
		return CanvasPos;

	// 파라미터를 0~1 범위로 정규화
	float NormX = (Param.X - EditingBlendSpace->XAxisMin) /
		(EditingBlendSpace->XAxisMax - EditingBlendSpace->XAxisMin);
	float NormY = (Param.Y - EditingBlendSpace->YAxisMin) /
		(EditingBlendSpace->YAxisMax - EditingBlendSpace->YAxisMin);

	// 0~1을 스크린 좌표로 변환 (Y축은 반전)
	float ScreenX = CanvasPos.x + NormX * CanvasSize.x;
	float ScreenY = CanvasPos.y + (1.0f - NormY) * CanvasSize.y;

	return ImVec2(ScreenX, ScreenY);
}

FVector2D SBlendSpace2DEditorWindow::ScreenToParam(ImVec2 ScreenPos) const
{
	if (!EditingBlendSpace)
		return FVector2D::Zero();

	// 스크린 좌표를 0~1 범위로 정규화
	float NormX = (ScreenPos.x - CanvasPos.x) / CanvasSize.x;
	float NormY = (CanvasPos.y + CanvasSize.y - ScreenPos.y) / CanvasSize.y;

	// 범위 클램핑
	NormX = FMath::Clamp(NormX, 0.0f, 1.0f);
	NormY = FMath::Clamp(NormY, 0.0f, 1.0f);

	// 0~1을 파라미터 범위로 변환
	float ParamX = EditingBlendSpace->XAxisMin +
		NormX * (EditingBlendSpace->XAxisMax - EditingBlendSpace->XAxisMin);
	float ParamY = EditingBlendSpace->YAxisMin +
		NormY * (EditingBlendSpace->YAxisMax - EditingBlendSpace->YAxisMin);

	return FVector2D(ParamX, ParamY);
}

void SBlendSpace2DEditorWindow::HandleMouseInput()
{
	if (!EditingBlendSpace)
		return;

	ImVec2 MousePos = ImGui::GetMousePos();
	bool bMouseInCanvas = ImGui::IsMouseHoveringRect(
		CanvasPos,
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y)
	);

	if (!bMouseInCanvas)
		return;

	// 왼쪽 클릭: 샘플 선택 또는 드래그 시작
	if (ImGui::IsMouseClicked(0))
	{
		// 클릭한 위치에 샘플이 있는지 확인
		bool bFoundSample = false;
		for (int32 i = 0; i < EditingBlendSpace->GetNumSamples(); ++i)
		{
			ImVec2 SampleScreenPos = ParamToScreen(EditingBlendSpace->Samples[i].Position);
			float Dist = sqrtf(
				(MousePos.x - SampleScreenPos.x) * (MousePos.x - SampleScreenPos.x) +
				(MousePos.y - SampleScreenPos.y) * (MousePos.y - SampleScreenPos.y)
			);

			if (Dist <= SamplePointRadius + 5.0f)
			{
				SelectSample(i);
				bDraggingSample = true;
				bFoundSample = true;
				break;
			}
		}

		if (!bFoundSample)
		{
			SelectedSampleIndex = -1;
		}
	}

	// 드래그 중
	if (bDraggingSample && ImGui::IsMouseDragging(0))
	{
		if (SelectedSampleIndex >= 0 && SelectedSampleIndex < EditingBlendSpace->GetNumSamples())
		{
			FVector2D NewPos = ScreenToParam(MousePos);
			EditingBlendSpace->Samples[SelectedSampleIndex].Position = NewPos;
		}
	}

	// 드래그 종료
	if (ImGui::IsMouseReleased(0))
	{
		bDraggingSample = false;
	}

	// 더블 클릭: 새 샘플 추가
	if (ImGui::IsMouseDoubleClicked(0))
	{
		FVector2D ClickPos = ScreenToParam(MousePos);
		AddSampleAtPosition(ClickPos);
	}
}

void SBlendSpace2DEditorWindow::HandleKeyboardInput()
{
	// Delete 키: 선택된 샘플 삭제
	if (INPUT.IsKeyPressed(VK_DELETE))
	{
		RemoveSelectedSample();
	}
}

void SBlendSpace2DEditorWindow::AddSampleAtPosition(FVector2D Position)
{
	if (!EditingBlendSpace)
		return;

	// TODO: 애니메이션 선택 다이얼로그 열기
	// 지금은 nullptr로 추가
	EditingBlendSpace->AddSample(Position, nullptr);

	// 새로 추가된 샘플 선택
	SelectedSampleIndex = EditingBlendSpace->GetNumSamples() - 1;
}

void SBlendSpace2DEditorWindow::RemoveSelectedSample()
{
	if (!EditingBlendSpace)
		return;

	if (SelectedSampleIndex >= 0 && SelectedSampleIndex < EditingBlendSpace->GetNumSamples())
	{
		EditingBlendSpace->RemoveSample(SelectedSampleIndex);
		SelectedSampleIndex = -1;
	}
}

void SBlendSpace2DEditorWindow::SelectSample(int32 Index)
{
	if (Index >= 0 && Index < EditingBlendSpace->GetNumSamples())
	{
		SelectedSampleIndex = Index;
	}
}

// ===== 새로운 레이아웃 렌더 함수들 =====

/**
 * @brief 상단: 애니메이션 프리뷰 뷰포트
 */
void SBlendSpace2DEditorWindow::RenderPreviewViewport()
{
	ImGui::Text("Animation Preview");
	ImGui::Separator();

	// 뷰포트 렌더링 영역 (전체 영역 사용)
	ImGui::BeginChild("ViewportRenderArea", ImVec2(0, -80), false, ImGuiWindowFlags_NoScrollbar);
	ImVec2 childPos = ImGui::GetWindowPos();
	ImVec2 childSize = ImGui::GetWindowSize();
	ImVec2 rectMin = childPos;
	ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);

	// 뷰포트 영역 저장 (SkeletalMeshViewer의 CenterRect와 동일)
	PreviewViewportRect.Left = rectMin.x;
	PreviewViewportRect.Top = rectMin.y;
	PreviewViewportRect.Right = rectMax.x;
	PreviewViewportRect.Bottom = rectMax.y;
	PreviewViewportRect.UpdateMinMax();

	ImGui::EndChild();

	// 프리뷰 컨트롤
	ImGui::Separator();
	ImGui::Text("Preview Controls:");
	ImGui::SliderFloat("X Axis", &PreviewParameter.X,
		EditingBlendSpace->XAxisMin,
		EditingBlendSpace->XAxisMax);

	// Y축 슬라이더를 별도로 (범위가 다를 수 있음)
	ImGui::SliderFloat("Y Axis", &PreviewParameter.Y,
		EditingBlendSpace->YAxisMin,
		EditingBlendSpace->YAxisMax);
}

/**
 * @brief 하단 왼쪽: 2D 블렌드 그리드 에디터
 */
void SBlendSpace2DEditorWindow::RenderGridEditor()
{
	ImGui::Text("Blend Space 2D Grid");
	ImGui::Separator();

	// 축 정보 표시
	ImGui::Text("X: %s (%.1f ~ %.1f)  |  Y: %s (%.1f ~ %.1f)",
		EditingBlendSpace->XAxisName.c_str(),
		EditingBlendSpace->XAxisMin,
		EditingBlendSpace->XAxisMax,
		EditingBlendSpace->YAxisName.c_str(),
		EditingBlendSpace->YAxisMin,
		EditingBlendSpace->YAxisMax);

	ImGui::Separator();

	// 그리드 캔버스
	CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
	CanvasSize = ImVec2(FMath::Min(AvailableSize.x - 20, 500.0f), FMath::Min(AvailableSize.y - 20, 500.0f));

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// 캔버스 배경
	DrawList->AddRectFilled(CanvasPos,
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
		IM_COL32(30, 30, 30, 255));

	// 그리드 렌더링
	RenderGrid();

	// 축 라벨 렌더링
	RenderAxisLabels();

	// 샘플 포인트 렌더링
	RenderSamplePoints();

	// 프리뷰 마커 렌더링
	RenderPreviewMarker();

	// 입력 처리
	HandleMouseInput();

	// 캔버스 영역 확보 (클릭 감지를 위해)
	ImGui::Dummy(CanvasSize);
}

/**
 * @brief 하단 오른쪽: 애니메이션 시퀀스 목록
 */
void SBlendSpace2DEditorWindow::RenderAnimationList()
{
	ImGui::Text("Animation Sequences");
	ImGui::Separator();

	// 애니메이션 로드 버튼
	if (ImGui::Button("Load Animations", ImVec2(-1, 30)))
	{
		// TODO: FBX 파일에서 애니메이션 로드
		ImGui::OpenPopup("LoadAnimationsPopup");
	}

	// 로드 팝업
	if (ImGui::BeginPopupModal("LoadAnimationsPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Select Skeletal Mesh:");
		ImGui::Separator();

		// ResourceManager에서 로드된 SkeletalMesh들 가져오기
		TArray<USkeletalMesh*> LoadedMeshes = UResourceManager::GetInstance().GetAll<USkeletalMesh>();

		static int SelectedMeshIndex = -1;
		USkeletalMesh* SelectedMesh = nullptr;

		ImGui::Text("Loaded Skeletal Meshes: %d", LoadedMeshes.Num());
		ImGui::Spacing();

		// SkeletalMesh 리스트 표시 (스크롤 가능한 영역)
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
		ImGui::BeginChild("MeshList", ImVec2(500, 350), true, ImGuiWindowFlags_None);
		{
			if (LoadedMeshes.Num() == 0)
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Skeletal Meshes loaded.");
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Please load a Skeletal Mesh first.");
			}
			else
			{
				for (int32 i = 0; i < LoadedMeshes.Num(); ++i)
				{
					USkeletalMesh* Mesh = LoadedMeshes[i];
					if (!Mesh) continue;

					const FSkeleton* Skeleton = Mesh->GetSkeleton();
					if (!Skeleton) continue;

					bool bIsSelected = (i == SelectedMeshIndex);

					// 선택 가능한 항목 - Skeleton 이름 또는 파일 경로 표시
					FString DisplayName;
					if (!Skeleton->Name.empty())
					{
						DisplayName = Skeleton->Name;
					}
					else
					{
						DisplayName = Mesh->GetFilePath();
					}

					// 선택 하이라이트 색상
					if (bIsSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
					}

					if (ImGui::Selectable(DisplayName.c_str(), bIsSelected, ImGuiSelectableFlags_None, ImVec2(0, 25)))
					{
						SelectedMeshIndex = i;
						SelectedMesh = Mesh;
					}

					if (bIsSelected)
					{
						ImGui::PopStyleColor();
					}

					// 더블클릭으로 바로 로드
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
					{
						SelectedMesh = Mesh;
						// 아래 Load 버튼 로직 실행
						goto LoadSelectedMesh;
					}

					// 툴팁: 자세한 정보 표시
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Skeleton: %s", Skeleton->Name.c_str());
						ImGui::Text("File: %s", Mesh->GetFilePath().c_str());
						ImGui::Text("Bones: %d", Skeleton->Bones.Num());
						ImGui::Text("Animations: %d", Mesh->GetAnimations().Num());
						ImGui::Separator();
						ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Double-click to load animations");
						ImGui::EndTooltip();
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		ImGui::Separator();

		if (ImGui::Button("Load", ImVec2(120, 0)))
		{
			LoadSelectedMesh:
			if (SelectedMeshIndex >= 0 && SelectedMeshIndex < LoadedMeshes.Num())
			{
				USkeletalMesh* Mesh = LoadedMeshes[SelectedMeshIndex];
				if (Mesh)
				{
					// 기존 리스트 초기화 (선택된 스켈레톤의 애니메이션만 표시)
					AvailableAnimations.clear();

					// SkeletalMesh에 저장된 애니메이션들을 가져옴
					const TArray<UAnimSequence*>& Animations = Mesh->GetAnimations();

					// 모든 애니메이션 추가
					for (UAnimSequence* Anim : Animations)
					{
						if (Anim)
						{
							AvailableAnimations.Add(Anim);
						}
					}

					// 프리뷰 메시 설정
					if (PreviewState && PreviewState->PreviewActor)
					{
						PreviewState->PreviewActor->SetSkeletalMesh(Mesh->GetFilePath());
						PreviewState->CurrentMesh = Mesh;
						if (auto* SkelComp = PreviewState->PreviewActor->GetSkeletalMeshComponent())
						{
							SkelComp->SetVisibility(true);
						}
					}

					UE_LOG("Loaded %d animations from SkeletalMesh: %s", Animations.Num(), Mesh->GetName().c_str());
				}
			}

			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::Separator();

	// SkeletalMeshViewer 스타일로 헤더 표시
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
	ImGui::Text("Animation List (%d)", AvailableAnimations.Num());
	ImGui::PopStyleColor();
	ImGui::Separator();
	ImGui::Spacing();

	// 애니메이션 리스트
	if (AvailableAnimations.Num() == 0)
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No animations loaded.");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select a Skeletal Mesh to load animations.");
	}
	else
	{
		ImGui::BeginChild("AnimListScroll", ImVec2(0, 0), false);
		for (int32 i = 0; i < AvailableAnimations.Num(); ++i)
		{
			UAnimSequence* Anim = AvailableAnimations[i];
			if (!Anim) continue;

			UAnimDataModel* DataModel = Anim->GetDataModel();
			if (!DataModel) continue;

			bool bIsSelected = (i == SelectedAnimationIndex);

			// FilePath에서 "파일명#애니메이션이름" 형식으로 표시
			FString FilePath = Anim->GetFilePath();
			FString DisplayName;

			size_t HashPos = FilePath.find('#');
			if (HashPos != FString::npos)
			{
				FString FullPath = FilePath.substr(0, HashPos); // "path/to/file.fbx"
				FString AnimStackName = FilePath.substr(HashPos + 1); // "AnimStackName"

				// 파일명만 추출 (경로 제거)
				size_t LastSlash = FullPath.find_last_of("/\\");
				FString FileName;
				if (LastSlash != FString::npos)
				{
					FileName = FullPath.substr(LastSlash + 1);
				}
				else
				{
					FileName = FullPath;
				}

				// 확장자 제거
				size_t DotPos = FileName.find_last_of('.');
				if (DotPos != FString::npos)
				{
					FileName = FileName.substr(0, DotPos);
				}

				DisplayName = FileName + "#" + AnimStackName;
			}
			else
			{
				// FilePath에 #이 없으면 GetName() 사용
				DisplayName = Anim->GetName();
				if (DisplayName.empty())
				{
					DisplayName = "Anim " + std::to_string(i);
				}
			}

			// "애니메이션이름 (재생시간)" 형식으로 표시
			char LabelBuffer[256];
			snprintf(LabelBuffer, sizeof(LabelBuffer), "%s (%.1fs)", DisplayName.c_str(), DataModel->GetPlayLength());
			FString Label = LabelBuffer;

			// 선택 가능한 항목
			if (ImGui::Selectable(Label.c_str(), bIsSelected))
			{
				SelectedAnimationIndex = i;
			}

			// 더블 클릭으로 샘플에 추가
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				// 선택된 그리드 샘플에 애니메이션 할당
				if (SelectedSampleIndex >= 0 && SelectedSampleIndex < EditingBlendSpace->GetNumSamples())
				{
					EditingBlendSpace->Samples[SelectedSampleIndex].Animation = Anim;
				}
			}

			// 우클릭 메뉴
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Add to Grid"))
				{
					// 중앙에 새 샘플 추가
					FVector2D CenterPos(
						(EditingBlendSpace->XAxisMin + EditingBlendSpace->XAxisMax) * 0.5f,
						(EditingBlendSpace->YAxisMin + EditingBlendSpace->YAxisMax) * 0.5f
					);

					EditingBlendSpace->AddSample(CenterPos, Anim);
				}

				if (ImGui::MenuItem("Preview"))
				{
					// TODO: 이 애니메이션을 프리뷰 뷰포트에서 재생
				}

				ImGui::EndPopup();
			}

			// 애니메이션 정보 툴팁
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Name: %s", Anim->GetName().c_str());
				if (Anim->GetDataModel())
				{
					ImGui::Text("Duration: %.2f seconds", Anim->GetDataModel()->GetPlayLength());
					ImGui::Text("Frames: %d", Anim->GetDataModel()->GetNumberOfFrames());
				}
				ImGui::Text("Double-click to assign to selected sample");
				ImGui::Text("Right-click for more options");
				ImGui::EndTooltip();
			}
		}
		ImGui::EndChild();
	}
}

void SBlendSpace2DEditorWindow::OnUpdate(float DeltaSeconds)
{
	if (!PreviewState || !PreviewState->World)
	{
		return;
	}

	// ViewportClient Tick (카메라 입력 처리)
	if (PreviewState->Client)
	{
		PreviewState->Client->Tick(DeltaSeconds);
	}

	// PreviewWorld 업데이트 (SkeletalMeshViewer와 동일)
	PreviewState->World->Tick(DeltaSeconds);
}

void SBlendSpace2DEditorWindow::OnMouseMove(FVector2D MousePos)
{
	if (!PreviewState || !PreviewState->Viewport) return;

	if (PreviewViewportRect.Contains(MousePos))
	{
		FVector2D LocalPos = MousePos - FVector2D(PreviewViewportRect.Left, PreviewViewportRect.Top);
		PreviewState->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
	}
}

void SBlendSpace2DEditorWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
	if (!PreviewState || !PreviewState->Viewport) return;

	if (PreviewViewportRect.Contains(MousePos))
	{
		FVector2D LocalPos = MousePos - FVector2D(PreviewViewportRect.Left, PreviewViewportRect.Top);
		PreviewState->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
	}
}

void SBlendSpace2DEditorWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
	if (!PreviewState || !PreviewState->Viewport) return;

	if (PreviewViewportRect.Contains(MousePos))
	{
		FVector2D LocalPos = MousePos - FVector2D(PreviewViewportRect.Left, PreviewViewportRect.Top);
		PreviewState->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
	}
}

void SBlendSpace2DEditorWindow::OnRenderViewport()
{
	// 윈도우가 닫혀있으면 렌더링하지 않음
	if (!bIsOpen)
	{
		return;
	}

	// SkeletalMeshViewer와 동일한 방식으로 뷰포트 렌더링
	if (PreviewState && PreviewState->Viewport && PreviewViewportRect.GetWidth() > 0 && PreviewViewportRect.GetHeight() > 0)
	{
		const uint32 NewStartX = static_cast<uint32>(PreviewViewportRect.Left);
		const uint32 NewStartY = static_cast<uint32>(PreviewViewportRect.Top);
		const uint32 NewWidth = static_cast<uint32>(PreviewViewportRect.Right - PreviewViewportRect.Left);
		const uint32 NewHeight = static_cast<uint32>(PreviewViewportRect.Bottom - PreviewViewportRect.Top);

		PreviewState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

		// 뷰포트 렌더링
		PreviewState->Viewport->Render();
	}
	else
	{
		// 뷰포트 영역이 없으면 리셋
		PreviewViewportRect = FRect(0, 0, 0, 0);
		PreviewViewportRect.UpdateMinMax();
	}
}
