

// based on: https://github.com/ocornut/imgui/issues/786#issuecomment-479539045

#include <sstream>
#include <string>
#include <iomanip>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <time.h>

#include "ImGuiFileDialog.h"

#include "Engine/Curve.h"
#include "Engine/MathCore.h"

/*
ImguiCurveWidget:
Provides 2 functions: 
- ImGui::CurveEditor
- ImGui::CurveAssetEditor

ImGui::CurveEditor is an editor for Curve structs
controls:
	pan:			drag background 
	zoom:			mouse wheel
	move key:		drag keys
	add key:		double click on curve 
	remove key:		right click key

usage:
	Curve curve;
	while (true)
	{
		ImGui::Begin("test window2");
	
		ImGui::Curve("test curve editor", curve);

		ImGui::End();
		co_await Suspend();
	}



ImGui::CurveAssetEditor wraps ImGui::Curve, adding controls for saving/loading to/from a file:

usage:
	Curve curve;
	std::string curveFilePath;
	while (true)
	{
		ImGui::Begin("test window2");

		ImGui::CurveAssetEditor("test curve editor", curve, curveFilePath);

		ImGui::End();
		co_await Suspend();
	}

*/

/*
TODO:
editing: 
- select keys 
	- type in values 
	- switch section mode

bonus:
- tangent controls:
	- merged tangents
	- auto tangents
- disallow dragging keys out of order
- zoom around cursor pos
- multiselect+drag keys
*/

namespace ImGui
{
	int CurveEditor(const char* in_label, Curve& in_curve) 
	{
		enum { SMOOTHNESS = 200 }; // curve smoothness: the higher number of segments, the smoother curve
		enum { CURVE_WIDTH = 3 }; // main curved line width
		enum { LINE_WIDTH = 2 }; // handlers: small lines width
		enum { KEY_RADIUS = 8 }; // handlers: circle visual radius
		enum { GRAB_RADIUS = 16 }; // handlers: circle click radius
		enum { CURVE_GRAB_RADIUS = 32 }; // handlers: curve radius
		enum { GRAB_BORDER = 2 }; // handlers: circle border width
		enum { AREA_CONSTRAINED = true }; // should grabbers be constrained to grid area?

		const ImGuiStyle& Style = GetStyle();
		const ImGuiIO& IO = GetIO();
		ImDrawList* DrawList = GetWindowDrawList();
		ImGuiWindow* Window = GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		const ImGuiID id = Window->GetID(in_label);

        // data
		ImGuiStorage* storage = ImGui::GetStateStorage();

		const ImGuiID zoomId = id + 1;
		Vec2f viewDims = {
			storage->GetFloat(zoomId + 0, 1.0f),
			storage->GetFloat(zoomId + 1, 1.0f)
		};

		const ImGuiID selectionIdxId = id + 3;
		int& selectedIdx = *storage->GetIntRef(selectionIdxId, -1);

		const ImGuiID selectionTypeId = id + 4;
		int& selectedType = *storage->GetIntRef(selectionTypeId, 0);

		const ImGuiID viewPosId = id + 5;
		Vec2f viewPos = {
			storage->GetFloat(viewPosId + 0, 0.5f),
			storage->GetFloat(viewPosId + 1, 0.5f)
		};
		
		const ImGuiID initedId = id + 7;
		bool& bInited = *storage->GetBoolRef(initedId, false);

		// zoom fit lambda
		const auto ZoomFit = [&](int in_axisIdx) {
			if (in_curve.IsValid())
			{
				const MinMaxf range = in_axisIdx == 0 ? in_curve.GetTimeRange() : in_curve.GetValueRange();

				const float extraScale = 1.2f;
				viewDims[in_axisIdx] = range.Size() * extraScale;

				viewPos[in_axisIdx] = range.Mid();
			}
		};

		// init
		if (!bInited)
		{
			bInited = true;

			ZoomFit(0);
			ZoomFit(1);
		}
		if (!in_curve.IsValid())
		{
			in_curve.Init();
		}

		// handle colors
		const ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);
		const ImVec4 gray(0.5f, 0.5f, 0.5f, 1.0f);
		const ImVec4 pink(1.00f, 0.00f, 0.75f, 1.0f);
		const ImVec4 cyan(0.00f, 0.75f, 1.00f, 1.0f);


		int changed = 0;

		// prepare canvas
		const ImVec2 canvasDims = GetContentRegionAvail() - ImVec2(0, 60);

		ImRect canvasBBox(Window->DC.CursorPos, Window->DC.CursorPos + canvasDims);
		ItemSize(canvasBBox);
		if (!ItemAdd(canvasBBox, NULL))
			return changed;

		const bool hovered = !!ItemHoverable(ImRect(canvasBBox.Min, canvasBBox.Min + canvasDims), id);
		

		// mouse wheel zoom
		ImGui::SetItemUsingMouseWheel();
		if (hovered)
		{
			float wheel = ImGui::GetIO().MouseWheel;
			if (wheel)
			{
				if (ImGui::IsItemActive())
				{
					ImGui::ClearActiveID();
				}
				else
				{
					float mult = std::pow(1.1f, Math::Abs(wheel));
					if (wheel > 0.0f) 
					{
						viewDims /= mult;
					}
					else
					{
						viewDims *= mult;
					}
					viewDims = Math::Max(viewDims, Vec2f::One * 0.01f);
				}
			}
		}

		// zoom fit buttons
		InputFloat2("view dims", &viewDims.x);
		if (ImGui::Button("zoom fit X"))
		{
			ZoomFit(0);
		}
		ImGui::SameLine();
		if (ImGui::Button("zoom fit Y"))
		{
			ZoomFit(1);
		}

		// view tm
		const Transform viewToLocalTM = {
			viewPos,
			0.0f,
			Math::Max(viewDims, Vec2f{SMALL_NUMBER, SMALL_NUMBER})
		};

		const Transform viewToCanvasTM = {
			Vec2f{ 
				canvasBBox.Min.x + Math::Abs(canvasBBox.Max.x - canvasBBox.Min.x) * 0.5f, 
				canvasBBox.Min.y + Math::Abs(canvasBBox.Max.y - canvasBBox.Min.y) * 0.5f
			},
			0.0f,
			Vec2f{ canvasBBox.Max.x - canvasBBox.Min.x, -(canvasBBox.Max.y - canvasBBox.Min.y) }
		};

		const Transform localToViewTM = viewToLocalTM.Inverse() * viewToCanvasTM;

		// draw frame
		RenderFrame(canvasBBox.Min, canvasBBox.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

		// bounds of the view box, in curve space
		const MinMaxf tRange = {
			localToViewTM.InvTransformPoint(Vec2f{canvasBBox.Min.x, 0.0f}).x,
			localToViewTM.InvTransformPoint(Vec2f{canvasBBox.Max.x, 0.0f}).x
		};

		const MinMaxf valRange = {
			localToViewTM.InvTransformPoint(Vec2f{0.0f, canvasBBox.Min.y}).y,
			localToViewTM.InvTransformPoint(Vec2f{0.0f, canvasBBox.Max.y}).y
		};

		PushClipRect(canvasBBox.Min, canvasBBox.Max, false);

		// draw background grid
		{
			const auto DrawGridLinesForAxis = [=](int compIdx, float viewSize){
				const int numGridLines = 8;
				const float gridLinesDist = std::pow(2.0f, Math::Ceil(std::log2(Math::Abs(viewSize)))) / numGridLines;

				for (int i = -numGridLines/2; i <= numGridLines/2; i++) {
					const float t = i * gridLinesDist + Math::RoundToMultiple(viewPos[compIdx], gridLinesDist);
				
					Vec2f worldPos = Vec2f::Zero;
					worldPos[compIdx] = t;
					const float pos = localToViewTM.TransformPoint(worldPos)[compIdx];
			
					std::stringstream labelTextStream;
					labelTextStream << std::fixed << std::setprecision(2) << t;

					const int otherCompIdx = 1 - compIdx;
					Vec2f P0 = Vec2f::One * canvasBBox.Min[otherCompIdx];
					Vec2f P1 = Vec2f::One * canvasBBox.Max[otherCompIdx];
					P0[compIdx] = pos;
					P1[compIdx] = pos;

					Vec2f labelPos = P0;
					labelPos[compIdx] += 2;
					DrawList->AddText(labelPos, GetColorU32(ImGuiCol_TextDisabled), labelTextStream.str().c_str());

					const float thickness = t == 0.0f ? 2.0f : 1.0f;
					//const ImU32 col = t == 0.0f ? GetColorU32(ImGuiCol_Text) : GetColorU32(ImGuiCol_TextDisabled);
					const ImU32 col = GetColorU32(ImGuiCol_TextDisabled);
					DrawList->AddLine(P0, P1, col, thickness);
				}
			};
			DrawGridLinesForAxis(0, tRange.Size());
			DrawGridLinesForAxis(1, valRange.Size());
		}

		// draw curve
		{
			const ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
			for (int i = 0; i < SMOOTHNESS; i++)
			{
				const float t0 = tRange.Lerp( (i + 0) / (float)(SMOOTHNESS - 1) );
				const float t1 = tRange.Lerp( (i + 1) / (float)(SMOOTHNESS - 1) );

				const Vec2f p0 = { t0, in_curve.Eval(t0) };
				const Vec2f p1 = { t1, in_curve.Eval(t1) };

				DrawList->AddLine(localToViewTM.TransformPoint(p0), localToViewTM.TransformPoint(p1), color, CURVE_WIDTH);
			}
		}

		std::vector<CurveKey>& keys = in_curve.GetKeys();

		if (!IsMouseDown(0))
		{
			selectedIdx = -1;
			selectedType = 0;
		}

		bool bHoveringKey = false;

		// draw+update handles 
		{
			const Vec2f mousePos = GetIO().MousePos;

			const auto EditPoint = [&](Vec2f& pos, int ptIdx, int ptType, ImVec4 color)
			{
				const Vec2f keyViewPos = localToViewTM.TransformPoint(pos);
				const bool bAlreadySelected = selectedIdx == ptIdx && selectedType == ptType;
				const bool bThisKeyHovered = keyViewPos.Dist(mousePos) < GRAB_RADIUS;

				color.w = 
					bAlreadySelected ? 0.25f
					: bThisKeyHovered ? 0.5f
					: 1.0f;

				DrawList->AddCircleFilled(keyViewPos, KEY_RADIUS, ImColor(white));
				DrawList->AddCircleFilled(keyViewPos, KEY_RADIUS - GRAB_BORDER, ImColor(color));


				//std::stringstream labelTextStream;
				//labelTextStream << std::fixed << std::setprecision(2) << ptIdx;
				//DrawList->AddText(keyViewPos + Vec2f{8, 8}, GetColorU32(ImGuiCol_TextDisabled), labelTextStream.str().c_str());

				if (bAlreadySelected || (bThisKeyHovered && selectedIdx < 0))
				{
					SetTooltip("(%4.3f, %4.3f)", pos.x, pos.y);
					bHoveringKey = true;

					// delete key on right click
					if (ptType == 1 && IsMouseClicked(1) && keys.size() > 1)
					{
						keys.erase(keys.begin() + ptIdx);
					}

					if (bAlreadySelected ? IsMouseDown(0) : IsMouseClicked(0))
					{
						// set up selection
						selectedIdx = ptIdx;
						selectedType = ptType;

						// drag the key
						const Vec2f keyDelta = localToViewTM.InvTransformVector(Vec2f{ GetIO().MouseDelta.x, GetIO().MouseDelta.y });
						pos += keyDelta;

						changed = true;

						return true;
					}
				}

				return false;
			};

			for (int i = 0; i < keys.size(); i++) 
			{
				Vec2f pos = keys[i].GetPos();
				if (EditPoint(pos, i, 1, gray))
				{
					keys[i].m_time = pos.x;
					keys[i].m_val = pos.y;
				}
			}

			for (int i = 0; i < in_curve.GetNumSections(); i++)
			{
				if (in_curve.GetSectionMode(i) == eCurveSectionMode::Bezier)
				{
					const auto EditTangent = [&](Vec2f& tangent, CurveKey& key, int ptIdx, int ptType, ImVec4 color) {
						Vec2f pos = tangent + key.GetPos();

						DrawList->AddLine(
							localToViewTM.TransformPoint(pos),
							localToViewTM.TransformPoint(key.GetPos()),
							ImColor(1.0f, 1.0f, 1.0f, 0.5f), LINE_WIDTH);

						if (EditPoint(pos, ptIdx, ptType, color))
						{
							tangent = pos - key.GetPos();
						}
					};

					EditTangent(keys[i].m_rightTangent, keys[i], i, 2, cyan);
					EditTangent(keys[i+1].m_leftTangent, keys[i+1], i, 3, pink);
				}
			}
		}

		if (selectedIdx < 0)
		{
			// drag view
			if ((IsMouseDragging(0) || IsMouseDragging(1) || IsMouseDragging(2)) && hovered)
			{
				const Vec2f dragDelta = localToViewTM.InvTransformVector(Vec2f{ GetIO().MouseDelta.x, GetIO().MouseDelta.y });
				viewPos -= dragDelta;
			}

			// double click on curve to add key
			if (!bHoveringKey)
			{
				const Vec2f mousePosCurveSpace = localToViewTM.InvTransformPoint(Vec2f{ GetIO().MousePos.x, GetIO().MousePos.y });
				const Vec2f curvePos = Vec2f{ mousePosCurveSpace.x, in_curve.Eval(mousePosCurveSpace.x) };
				const Vec2f curvePosCanvasSpace = localToViewTM.TransformPoint(curvePos);

				if (curvePosCanvasSpace.Dist(GetIO().MousePos) <= CURVE_GRAB_RADIUS)
				{
					// tooltip and dot on curve
					SetTooltip("(%4.3f, %4.3f)", curvePos.x, curvePos.y);
					DrawList->AddCircleFilled(curvePosCanvasSpace, 4, ImColor(white));

					// add key
					if (IsMouseDoubleClicked(0))
					{
						const int sectionIdx = in_curve.GetSectionIdxForT(curvePos.x);

						CurveKey newKey{};
						newKey.m_time = curvePos.x;
						newKey.m_val = curvePos.y;

						const int clampedIdx0 = Math::Clamp(sectionIdx, 0, (int)keys.size() - 1);
						const int clampedIdx1 = Math::Clamp(sectionIdx+1, 0, (int)keys.size() - 1);

						const Vec2f tangent = in_curve.GetTangent(curvePos.x);
						newKey.m_leftTangent = -tangent * curvePos.Dist(keys[clampedIdx0].GetPos()) / 3.0f;
						newKey.m_rightTangent = tangent * curvePos.Dist(keys[clampedIdx1].GetPos()) / 3.0f;

						newKey.m_rightSectionMode = in_curve.GetSectionMode(clampedIdx0);

						keys.insert(keys.begin() + Math::Clamp(sectionIdx+1, 0, (int)keys.size()), newKey);
					}
				}
			}
		}

		PopClipRect();


		// re-store vector state
		storage->SetFloat(zoomId + 0, viewDims.x);
		storage->SetFloat(zoomId + 1, viewDims.y);

		storage->SetFloat(viewPosId + 0, viewPos.x);
		storage->SetFloat(viewPosId + 1, viewPos.y);

		// return
		return changed;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int CurveAssetEditor(const char* in_label, ::Curve& in_curve, std::string& in_curvePath)
	{
		// data
		ImGuiWindow* Window = GetCurrentWindow();
		const ImGuiID id = Window->GetID(in_label);
		ImGuiStorage* storage = ImGui::GetStateStorage();

		const ImGuiID editedId = id + 1;
		bool& bCurveDirty = *storage->GetBoolRef(editedId, false);


		bool bEditedThisFrame = false;

		// filename display
		ImGui::Text("filename: %s %s", in_curvePath.empty() ? "[none]" : in_curvePath.c_str(), bCurveDirty ? "(edited)" : "");

		// save button
		if (!in_curvePath.empty() && ImGui::Button("Save"))
		{
			in_curve.Serialize(in_curvePath);

			bCurveDirty = false;
		}

		// save as button
		ImGui::SameLine();
		if (ImGui::Button("Save As"))
		{
			ImGuiFileDialog::Instance()->OpenDialog("saveCurveKey", "Save Curve File", ".rtcurve", "data/curves/", "");
		}

		if (ImGuiFileDialog::Instance()->Display("saveCurveKey"))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				in_curvePath = ImGuiFileDialog::Instance()->GetFilePathName();

				in_curve.Serialize(in_curvePath);

				bCurveDirty = false;
			}

			ImGuiFileDialog::Instance()->Close();
		}

		// load button
		ImGui::SameLine();
		if (ImGui::Button("Load"))
		{
			ImGuiFileDialog::Instance()->OpenDialog("loadCurveKey", "Load Curve File", ".rtcurve", "data/curves/", "");
		}

		if (ImGuiFileDialog::Instance()->Display("loadCurveKey"))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				in_curvePath = ImGuiFileDialog::Instance()->GetFilePathName();

				in_curve = Curve(in_curvePath);
				bEditedThisFrame = true;
			}

			ImGuiFileDialog::Instance()->Close();
		}

		// curve editor
		if (ImGui::CurveEditor("test curve editor", in_curve))
		{
			bCurveDirty = true;
			bEditedThisFrame = true;
		}

		return bEditedThisFrame;
	}
}