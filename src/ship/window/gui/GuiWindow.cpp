#include "ship/window/gui/GuiWindow.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"
#include <algorithm>
#include <imgui_internal.h> // ImGuiWindow / FindWindowByName, for the on-screen position clamp

namespace Ship {
GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize,
                     uint32_t windowFlags)
    : GuiElement(isVisible), mName(name), mVisibilityConsoleVariable(consoleVariable), mOriginalSize(originalSize),
      mWindowFlags(windowFlags) {
    if (!mVisibilityConsoleVariable.empty()) {
        mIsVisible = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(mVisibilityConsoleVariable.c_str(),
                                                                                     mIsVisible);
        SyncVisibilityConsoleVariable();
    }
}

GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize)
    : GuiWindow(consoleVariable, isVisible, name, originalSize, ImGuiWindowFlags_None) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name)
    : GuiWindow(consoleVariable, isVisible, name, ImVec2{ -1, -1 }, ImGuiWindowFlags_None) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize,
                     uint32_t windowFlags)
    : GuiWindow(consoleVariable, false, name, originalSize, windowFlags) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize)
    : GuiWindow(consoleVariable, false, name, originalSize, ImGuiWindowFlags_None) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name)
    : GuiWindow(consoleVariable, false, name, ImVec2{ -1, -1 }, ImGuiWindowFlags_None) {
}

void GuiWindow::SetVisibility(bool visible) {
    mIsVisible = visible;
    SyncVisibilityConsoleVariable();
}

void GuiWindow::SyncVisibilityConsoleVariable() {
    if (mVisibilityConsoleVariable.empty()) {
        return;
    }

    bool shouldSave = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
                          mVisibilityConsoleVariable.c_str(), 0) != IsVisible();

    if (IsVisible()) {
        Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(mVisibilityConsoleVariable.c_str(),
                                                                        IsVisible());
    } else {
        Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(mVisibilityConsoleVariable.c_str());
    }

    if (shouldSave) {
        Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
}

void GuiWindow::Draw() { // NOLINT(readability-function-cognitive-complexity)
    if (!IsVisible()) {
        return;
    }
    if (mOriginalSize != ImVec2{ -1, -1 }) {
        // Desktop-authored defaults exceed a phone screen. Clamp to the safe work area (the iOS
        // insets are published there in Gui::StartFrame) and constrain resize so an imgui.ini
        // restored from a desktop install, or a user drag, cannot push the window off-screen.
        const ImVec2 work = ImGui::GetMainViewport()->WorkSize;
        const ImVec2 workPos = ImGui::GetMainViewport()->WorkPos;
        ImGui::SetNextWindowSize(
            ImVec2(std::min(mOriginalSize.x, work.x * 0.95f), std::min(mOriginalSize.y, work.y * 0.95f)),
            ImGuiCond_FirstUseEver);
        // Floor the minimum as well as the maximum: a zero minimum lets a stray drag collapse
        // the window down to WindowMinSize, and on a touch screen there is no handle left big
        // enough to grab it back.
        ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(220.0f, work.x), std::min(140.0f, work.y)), work);
#if defined(__IOS__) || defined(__ANDROID__)
        // Nothing constrains window POSITION, so a window dragged past the edge is gone for
        // good -- there is no keyboard and no window list to recover it with.
        if (ImGuiWindow* w = ImGui::FindWindowByName(mName.c_str())) {
            const ImVec2 clamped(
                std::clamp(w->Pos.x, workPos.x, std::max(workPos.x, workPos.x + work.x - w->Size.x)),
                std::clamp(w->Pos.y, workPos.y, std::max(workPos.y, workPos.y + work.y - w->Size.y)));
            if (clamped.x != w->Pos.x || clamped.y != w->Pos.y) {
                ImGui::SetNextWindowPos(clamped);
            }
        }
#endif
    }
    if (!ImGui::Begin(mName.c_str(), &mIsVisible, mWindowFlags)) {
        ImGui::End();
    } else {
        DrawElement();
        ImGui::End();
    }
    // Sync up the IsVisible flag if it was changed by ImGui
    SyncVisibilityConsoleVariable();
}

std::string GuiWindow::GetName() {
    return mName;
}

void GuiWindow::BeginGroupPanel(const char* name, const ImVec2& size) {
    ImGui::BeginGroup();

    // auto cursorPos = ImGui::GetCursorScreenPos();
    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f) {
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    } else {
        effectiveSize.x = size.x;
    }
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    // ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));
    mGroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void GuiWindow::EndGroupPanel(float minHeight) {
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, std::max(frameHeight - frameHeight * 0.5f - itemSpacing.y, minHeight)));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    // ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

    auto labelRect = mGroupPanelLabelStack.back();
    mGroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;
    for (int i = 0; i < 4; ++i) {
        switch (i) {
            // left half-plane
            case 0:
                ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true);
                break;
                // right half-plane
            case 1:
                ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true);
                break;
                // top
            case 2:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true);
                break;
                // bottom
            case 3:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true);
                break;
        }

        ImGui::GetWindowDrawList()->AddRect(frameRect.Min, frameRect.Max,
                                            ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), halfFrame.x);

        ImGui::PopClipRect();
    }

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
}
} // namespace Ship
