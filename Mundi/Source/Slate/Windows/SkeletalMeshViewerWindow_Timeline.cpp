#include "pch.h"

#include "BoneAnchorComponent.h"
#include "SkeletalMeshActor.h"
#include "SkeletalMeshViewerWindow.h"
#include "ImGui/imgui.h"
#include "Source/Runtime/Engine/Animation/AnimDataModel.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

// Timeline 컨트롤 UI 렌더링
void SSkeletalMeshViewerWindow::RenderTimelineControls(ViewerState* State)
{
    if (!State || !State->CurrentAnimation)
        return;

    UAnimDataModel* DataModel = State->CurrentAnimation->GetDataModel();
    if (!DataModel)
        return;

    float MaxTime = DataModel->GetPlayLength();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));

    // === 타임라인 컨트롤 버튼들 ===
    float buttonSize = 24.0f;

    // ToFront |<<
    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u23EE"), ImVec2(buttonSize, buttonSize)))
    {
        TimelineToFront(State);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("To Front");

    ImGui::SameLine();

    // ToPrevious |<
    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u23F4"), ImVec2(buttonSize, buttonSize)))
    {
        TimelineToPrevious(State);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Previous Frame");

    ImGui::SameLine();

    // Reverse <<
    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u23EA"), ImVec2(buttonSize, buttonSize)))
    {
        TimelineReverse(State);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reverse");

    ImGui::SameLine();

    // Record (빨간 동그라미) - TODO: 녹화 기능 구현 시 스타일 추가
    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u25CF"), ImVec2(buttonSize, buttonSize)))
    {
        TimelineRecord(State);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Record");

    ImGui::SameLine();

    // Play/Pause (AnimInstance 컨트롤)
    if (State->bIsPlaying)
    {
        if (ImGui::Button(reinterpret_cast<const char*>(u8"\u23F8"), ImVec2(buttonSize, buttonSize))) // Pause
        {
            if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
            {
                UAnimInstance* AnimInst = State->PreviewActor->GetSkeletalMeshComponent()->GetAnimInstance();
                if (AnimInst)
                {
                    AnimInst->StopAnimation();
                    State->bIsPlaying = false;
                }
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause");
    }
    else
    {
        if (ImGui::Button(reinterpret_cast<const char*>(u8"\u25B6"), ImVec2(buttonSize, buttonSize))) // Play
        {
            TimelinePlay(State);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play");
    }

    ImGui::SameLine();

    // ToNext >|
    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u23F5"), ImVec2(buttonSize, buttonSize)))
    {
        TimelineToNext(State);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Next Frame");

    ImGui::SameLine();

    // ToEnd >>|
    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u23ED"), ImVec2(buttonSize, buttonSize)))
    {
        TimelineToEnd(State);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("To End");

    ImGui::SameLine();

    // Loop 토글
    bool bWasLooping = State->bLoopAnimation;
    if (bWasLooping)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
    }

    if (ImGui::Button(reinterpret_cast<const char*>(u8"\u27F3"), ImVec2(buttonSize, buttonSize)))
    {
        State->bLoopAnimation = !State->bLoopAnimation;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Loop");

    if (bWasLooping)
    {
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    // Timeline 슬라이더
    ImGui::SetNextItemWidth(200.0f);
    float PrevTime = State->CurrentAnimationTime;
    if (ImGui::SliderFloat("##Timeline", &State->CurrentAnimationTime, 0.0f, MaxTime, "%.2fs"))
    {
        // 슬라이더가 변경되면 AnimInstance에 시간 설정
        if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
        {
            UAnimInstance* AnimInst = State->PreviewActor->GetSkeletalMeshComponent()->GetAnimInstance();
            if (AnimInst)
            {
                AnimInst->SetPosition(State->CurrentAnimationTime);
            }
        }
    }

    ImGui::SameLine();

    // 시간 표시
    ImGui::Text("%.2f / %.2f s", State->CurrentAnimationTime, MaxTime);

    ImGui::SameLine();

    // 재생 속도
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::SliderFloat("##Speed", &State->PlaybackSpeed, 0.1f, 3.0f, "%.1fx");

    ImGui::PopStyleVar(2);
}

// Timeline 헬퍼: 프레임 변경 시 공통 갱신 로직
void SSkeletalMeshViewerWindow::RefreshAnimationFrame(ViewerState* State)
{
    if (!State) return;

    // 편집된 bone transform 캐시 초기화 (새 프레임의 애니메이션 포즈 적용)
    State->EditedBoneTransforms.clear();

    // AnimInstance를 통해 포즈 업데이트 (EvaluateAnimation이 자동 호출됨)
    if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
    {
        UAnimInstance* AnimInst = State->PreviewActor->GetSkeletalMeshComponent()->GetAnimInstance();
        if (AnimInst)
        {
            AnimInst->SetPosition(State->CurrentAnimationTime);
        }
    }

    // Bone Line 강제 갱신 (모든 본 업데이트)
    State->bBoneLinesDirty = true;
    if (State->PreviewActor)
    {
        State->PreviewActor->RebuildBoneLines(State->SelectedBoneIndex, true);

        // BoneAnchor 위치 동기화 (본이 선택되어 있으면)
        if (State->SelectedBoneIndex >= 0)
        {
            State->PreviewActor->RepositionAnchorToBone(State->SelectedBoneIndex);
        }
    }
}

// Timeline 기능 구현
void SSkeletalMeshViewerWindow::TimelineToFront(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;
    State->CurrentAnimationTime = 0.0f;
    RefreshAnimationFrame(State);
}

void SSkeletalMeshViewerWindow::TimelineToPrevious(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;

    UAnimDataModel* DataModel = State->CurrentAnimation->GetDataModel();
    if (!DataModel) return;

    // 프레임당 시간 (30fps 기준)
    float FrameTime = 1.0f / 30.0f;
    State->CurrentAnimationTime = FMath::Max(0.0f, State->CurrentAnimationTime - FrameTime);

    RefreshAnimationFrame(State);
}

void SSkeletalMeshViewerWindow::TimelineReverse(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;

    // 역재생 (음수 속도) - AnimInstance를 통해 설정
    State->PlaybackSpeed = -std::abs(State->PlaybackSpeed);

    if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
    {
        UAnimInstance* AnimInst = State->PreviewActor->GetSkeletalMeshComponent()->GetAnimInstance();
        if (AnimInst)
        {
            // 일시정지 위치에서 역재생 시작
            AnimInst->SetPlayRate(State->PlaybackSpeed);

            // 현재 애니메이션이 설정되지 않았을 때만 PlayAnimation 호출
            if (AnimInst->GetCurrentAnimation() != State->CurrentAnimation)
            {
                AnimInst->PlayAnimation(State->CurrentAnimation, State->PlaybackSpeed);
            }
            else
            {
                // 같은 애니메이션이면 현재 위치에서 재생 재개
                AnimInst->ResumeAnimation();
            }

            State->bIsPlaying = true;
        }
    }
}

void SSkeletalMeshViewerWindow::TimelineRecord(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;

    // TODO: 녹화 기능 구현 (나중에 추가)
    UE_LOG("Timeline: Record feature not implemented yet");
}

void SSkeletalMeshViewerWindow::TimelinePlay(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;

    // 정방향 재생 - AnimInstance를 통해 설정
    State->PlaybackSpeed = std::abs(State->PlaybackSpeed);

    if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
    {
        UAnimInstance* AnimInst = State->PreviewActor->GetSkeletalMeshComponent()->GetAnimInstance();
        if (AnimInst)
        {
            // 일시정지 위치에서 재생 시작
            AnimInst->SetPlayRate(State->PlaybackSpeed);

            // 현재 애니메이션이 설정되지 않았을 때만 PlayAnimation 호출
            if (AnimInst->GetCurrentAnimation() != State->CurrentAnimation)
            {
                AnimInst->PlayAnimation(State->CurrentAnimation, State->PlaybackSpeed);
            }
            else
            {
                // 같은 애니메이션이면 현재 위치에서 재생 재개
                AnimInst->ResumeAnimation();
            }

            State->bIsPlaying = true;
        }
    }
}

void SSkeletalMeshViewerWindow::TimelineToNext(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;

    UAnimDataModel* DataModel = State->CurrentAnimation->GetDataModel();
    if (!DataModel) return;

    // 프레임당 시간 (30fps 기준)
    float FrameTime = 1.0f / 30.0f;
    float MaxTime = DataModel->GetPlayLength();
    State->CurrentAnimationTime = FMath::Min(MaxTime, State->CurrentAnimationTime + FrameTime);

    RefreshAnimationFrame(State);
}

void SSkeletalMeshViewerWindow::TimelineToEnd(ViewerState* State)
{
    if (!State || !State->CurrentAnimation) return;

    UAnimDataModel* DataModel = State->CurrentAnimation->GetDataModel();
    if (!DataModel) return;

    State->CurrentAnimationTime = DataModel->GetPlayLength();

    RefreshAnimationFrame(State);
}
