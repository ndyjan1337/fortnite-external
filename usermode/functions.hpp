#include <string.h>
#include <list>
#include <string>
#include "ue.hpp"

namespace camera
{
	fvector location, rotation;
	float fov;
}

namespace uee
{
	class ue 
	{
	public:

		auto get_player_name(uintptr_t player_state) -> std::string
		{
			auto fstring = read<__int64>(player_state + 0xAB0);
			auto length = read<int>(fstring + 16);

			auto v6 = (__int64)length;

			if (!v6) return std::string(ndyjancryptt("In Lobby"));

			auto IsABot = read<char>(player_state + 0x29A & 0x8);

			if (IsABot) return std::string(ndyjancryptt("AI"));


			auto ftext = (PVOID)read<__int64>(fstring + 8);

			wchar_t* buffer = new wchar_t[length];
			mem::read_physical(static_cast<PVOID>(ftext), buffer, length * sizeof(wchar_t));

			char v21;
			int v22;
			int i;

			int v25;
			UINT16* v23;

			v21 = v6 - 1;
			if (!(UINT32)v6)
				v21 = 0;
			v22 = 0;
			v23 = (UINT16*)buffer;
			for (i = (v21) & 3; ; *v23++ += i & 7)
			{
				v25 = v6 - 1;
				if (!(UINT32)v6)
					v25 = 0;
				if (v22 >= v25)
					break;
				i += 3;
				++v22;
			}

			std::wstring username{ buffer };
			delete[] buffer;
			return std::string(username.begin(), username.end());
		};

		auto in_screen(fvector2d screen_location) -> bool {

			if (screen_location.x > 0 && screen_location.x < screen_width && screen_location.y > 0 && screen_location.y < screen_height) return true;
			else return false;
		}


		camera_position_s get_camera()
		{
			camera_position_s camera;

			auto location_pointer = read<uintptr_t>(arrays->uworld + 0x110);
			auto rotation_pointer = read<uintptr_t>(arrays->uworld + 0x120);

			struct FNRot
			{
				double a;
				char pad_0008[24];
				double b;
				char pad_0028[424];
				double c;
			} fnRot;

			fnRot.a = read<double>(rotation_pointer);
			fnRot.b = read<double>(rotation_pointer + 0x20);
			fnRot.c = read<double>(rotation_pointer + 0x1d0);

			camera.location = read<fvector>(location_pointer);
			camera.rotation.x = asin(fnRot.c) * (180.0 / M_PI);
			camera.rotation.y = ((atan2(fnRot.a * -1, fnRot.b) * (180.0 / M_PI)) * -1) * -1;
			camera.fov = read<float>((uintptr_t)arrays->player_controller + 0x394) * 90.f;

			return camera;
		}

		void RadarRange(float* x, float* y, float range)
		{
			if (fabs((*x)) > range || fabs((*y)) > range)
			{
				if ((*y) > (*x))
				{
					if ((*y) > -(*x))
					{
						(*x) = range * (*x) / (*y);
						(*y) = range;
					}
					else
					{
						(*y) = -range * (*y) / (*x);
						(*x) = -range;
					}
				}
				else
				{
					if ((*y) > -(*x))
					{
						(*y) = range * (*y) / (*x);
						(*x) = range;
					}
					else
					{
						(*x) = -range * (*x) / (*y);
						(*y) = -range;
					}
				}
			}
		}


		inline fvector2d w2s(fvector WorldLocation)
		{
			camera_postion = get_camera();

			if (WorldLocation.x == 0)
				return fvector2d(0, 0);

			_MATRIX tempMatrix = Matrix(camera_postion.rotation);

			fvector vAxisX = fvector(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
			fvector vAxisY = fvector(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
			fvector vAxisZ = fvector(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

			fvector vDelta = WorldLocation - camera_postion.location;
			fvector vTransformed = fvector(vDelta.dot(vAxisY), vDelta.dot(vAxisZ), vDelta.dot(vAxisX));

			if (vTransformed.z < 1.f)
				vTransformed.z = 1.f;

			return fvector2d((screen_width / 2.0f) + vTransformed.x * (((screen_width / 2.0f) / tanf(camera_postion.fov * (float)M_PI / 360.f))) / vTransformed.z, (screen_height / 2.0f) - vTransformed.y * (((screen_width / 2.0f) / tanf(camera_postion.fov * (float)M_PI / 360.f))) / vTransformed.z);
		}


		static auto Bone(uintptr_t skeletal_mesh, int bone_index) -> fvector {

			uintptr_t bone_array = read<uintptr_t>(skeletal_mesh + 0x5B8); //BoneArray 
			if (bone_array == NULL) bone_array = read<uintptr_t>(skeletal_mesh + 0x5B0 + 0x10);
			FTransform bone = read<FTransform>(bone_array + (bone_index * 0x60));
			FTransform component_to_world = read<FTransform>(skeletal_mesh + 0x1c0);
			D3DMATRIX matrix = MatrixMultiplication(bone.ToMatrixWithScale(), component_to_world.ToMatrixWithScale());
			return fvector(matrix._41, matrix._42, matrix._43);
		}

		auto IsShootable(fvector lur, fvector wl) -> bool {

			if (lur.x >= wl.x - 20 && lur.x <= wl.x + 20 && lur.y >= wl.y - 20 && lur.y <= wl.y + 20 && lur.z >= wl.z - 30 && lur.z <= wl.z + 30)
				return true;
			else
				return false;

		}

		static auto is_visible(uintptr_t skeletal_mesh) -> bool {

			auto last_submit = read<float>(skeletal_mesh + 0x2E8);
			auto last_render = read<float>(skeletal_mesh + 0x2F0);
			return (bool)(last_render + 0.06f >= last_submit);
		}

		fvector Prediction(fvector TargetPosition, fvector ComponentVelocity, float player_distance, float ProjectileSpeed = 239)
		{
			float gravity = 3.5;
			float TimeToTarget = player_distance / ProjectileSpeed;
			float bulletDrop = abs(gravity) * (TimeToTarget * TimeToTarget) * 0.5;
			return fvector
			{
			TargetPosition.x + TimeToTarget * ComponentVelocity.x,
			TargetPosition.y + TimeToTarget * ComponentVelocity.y,
			TargetPosition.z + TimeToTarget * ComponentVelocity.z + bulletDrop
			};
		}


		//Pasted from Nebula src
		std::string get_rank(int32_t Tier)
		{
			if (Tier == 0)
				return std::string("Bronze 1");
			else if (Tier == 1)
				return std::string("Bronze 2");
			else if (Tier == 2)
				return std::string("Bronze 3");
			else if (Tier == 3)
				return std::string("Silver 1");
			else if (Tier == 4)
				return std::string("Silver 2");
			else if (Tier == 5)
				return std::string("Silver 3");
			else if (Tier == 6)
				return std::string("Gold 1");
			else if (Tier == 7)
				return std::string("Gold 2");
			else if (Tier == 8)
				return std::string("Gold 3");
			else if (Tier == 9)
				return std::string("Platinum 1");
			else if (Tier == 10)
				return std::string("Platinum 2");
			else if (Tier == 11)
				return std::string("Platinum 3");
			else if (Tier == 12)
				return std::string("Diamond 1");
			else if (Tier == 13)
				return std::string("Diamond 2");
			else if (Tier == 14)
				return std::string("Diamond 3");
			else if (Tier == 15)
				return std::string("Elite");
			else if (Tier == 16)
				return std::string("Champion");
			else if (Tier == 17)
				return std::string("Unreal");
			else
				return std::string("Unranked");
		}
		ImVec4 get_rank_color(int32_t Tier)
		{
			if (Tier == 0 || Tier == 1 || Tier == 2)
				return ImVec4(0.902f, 0.580f, 0.227f, 1.0f); // Bronze
			else if (Tier == 3 || Tier == 4 || Tier == 5)
				return ImVec4(0.843f, 0.843f, 0.843f, 1.0f); // Silver
			else if (Tier == 6 || Tier == 7 || Tier == 8)
				return ImVec4(1.0f, 0.871f, 0.0f, 1.0f); // Gold
			else if (Tier == 9 || Tier == 10 || Tier == 11)
				return ImVec4(0.0f, 0.7f, 0.7f, 1.0f); // Platinum
			else if (Tier == 12 || Tier == 13 || Tier == 14)
				return ImVec4(0.1686f, 0.3294f, 0.8235f, 1.0f); // Diamond
			else if (Tier == 15)
				return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Elite
			else if (Tier == 16)
				return ImVec4(1.0f, 0.6f, 0.0f, 1.0f); // Champion
			else if (Tier == 17)
				return ImVec4(0.6f, 0.0f, 0.6f, 1.0f); // Unreal
			else
				return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Unranked
		}

		auto getranked(uintptr_t PlayerState) -> std::string
		{
			if (is_valid(player_state))
			{
				uintptr_t HabaneroComponent = read<uintptr_t>(player_state + 0xab0);
				if (is_valid(HabaneroComponent))

				{
					uintptr_t rankedprogress = read<uintptr_t>(HabaneroComponent + 0xb8);
				}
				wchar_t ranked[64] = { 0 }; // Initialize the array to avoid potential issues
				std::wstring ranked_wstr(ranked);
				std::string ranked_str(ranked_wstr.begin(), ranked_wstr.end());
				return ranked_str;
			}
		}




		__forceinline auto skeleton(uintptr_t mesh) -> void
		{
			const int color = ImColor(globals::g_color[0], globals::g_color[1], globals::g_color[2]);

			// O L D  S K E L E T O N  C O D E
			// 
			// 
			//fvector bonePositions[] = {
			//	Bone(mesh, 109),  // HeadBone
			//	Bone(mesh, 2),   // Hip
			//	Bone(mesh, 66),  // Neck
			//	Bone(mesh, 9),   // UpperArmLeft
			//	Bone(mesh, 38),  // UpperArmRight
			//	Bone(mesh, 10),  // LeftHand
			//	Bone(mesh, 39),  // RightHand
			//	Bone(mesh, 11),  // LeftHand1
			//	Bone(mesh, 40),  // RightHand1
			//	Bone(mesh, 78),  // RightThigh
			//	Bone(mesh, 71),  // LeftThigh
			//	Bone(mesh, 79),  // RightCalf
			//	Bone(mesh, 72),  // LeftCalf
			//	Bone(mesh, 75),  // LeftFoot 
			//	Bone(mesh, 82)   // RightFoot
			//};

			//fvector2d bonePositionsOut[16];
			//for (int i = 0; i < 16; ++i) {
			//	bonePositionsOut[i] = w2s(bonePositions[i]);
			//}

			/*ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[7].x, bonePositionsOut[7].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[8].x, bonePositionsOut[8].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[13].x, bonePositionsOut[13].y), ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(bonePositionsOut[14].x, bonePositionsOut[14].y), ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), color, globals::g_skeletonthickness);*/

			fvector vHeadBone = Bone(mesh, 67);
			fvector vHip = Bone(mesh, 3);
			fvector vNeck = Bone(mesh, 66);
			fvector vUpperArmLeft = Bone(mesh, 9);
			fvector vUpperArmRight = Bone(mesh, 38);
			fvector vElbowLeft = Bone(mesh, 10);
			fvector vElbowRight = Bone(mesh, 39);
			fvector vLeftHand = Bone(mesh, 33);
			fvector vRightHand = Bone(mesh, 62);
			fvector vLeftHand1 = Bone(mesh, 33);
			fvector vRightHand1 = Bone(mesh, 62);
			fvector vRightThigh = Bone(mesh, 78);
			fvector vLeftThigh = Bone(mesh, 71);
			fvector vRightCalf = Bone(mesh, 79);
			fvector vLeftCalf = Bone(mesh, 72);
			fvector vLeftFoot = Bone(mesh, 73);
			fvector vRightFoot = Bone(mesh, 80);
			fvector vLeftHeel = Bone(mesh, 75);
			fvector vRightHeel = Bone(mesh, 82);
			fvector vLeftToe = Bone(mesh, 76);
			fvector vRightToe = Bone(mesh, 83);


			fvector2d vHeadBoneOut = w2s(fvector(vHeadBone.x, vHeadBone.y, vHeadBone.z));
			fvector2d vHipOut = w2s(vHip);
			fvector2d vNeckOut = w2s(vNeck);
			fvector2d vUpperArmLeftOut = w2s(vUpperArmLeft);
			fvector2d vUpperArmRightOut = w2s(vUpperArmRight);
			fvector2d vElbowLeftOut = w2s(vElbowLeft);
			fvector2d vElbowRightOut = w2s(vElbowRight);
			fvector2d vLeftHandOut = w2s(vLeftHand);
			fvector2d vRightHandOut = w2s(vRightHand);
			fvector2d vLeftHandOut1 = w2s(vLeftHand1);
			fvector2d vRightHandOut1 = w2s(vRightHand1);
			fvector2d vLeftHeelOut = w2s(vLeftHeel);
			fvector2d vRightHeelOut = w2s(vRightHeel);
			fvector2d vRightThighOut = w2s(vRightThigh);
			fvector2d vLeftThighOut = w2s(vLeftThigh);
			fvector2d vRightCalfOut = w2s(vRightCalf);
			fvector2d vLeftCalfOut = w2s(vLeftCalf);
			fvector2d vLeftFootOut = w2s(vLeftFoot);
			fvector2d vRightFootOut = w2s(vRightFoot);
			fvector2d vLeftToeOut = w2s(vLeftToe);
			fvector2d vRightToeOut = w2s(vRightToe);


			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vHipOut.x, vHipOut.y), ImVec2(vNeckOut.x, vNeckOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vUpperArmLeftOut.x, vUpperArmLeftOut.y), ImVec2(vNeckOut.x, vNeckOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vUpperArmRightOut.x, vUpperArmRightOut.y), ImVec2(vNeckOut.x, vNeckOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vElbowLeftOut.x, vElbowLeftOut.y), ImVec2(vUpperArmLeftOut.x, vUpperArmLeftOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vElbowRightOut.x, vElbowRightOut.y), ImVec2(vUpperArmRightOut.x, vUpperArmRightOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftHandOut.x, vLeftHandOut.y), ImVec2(vElbowLeftOut.x, vElbowLeftOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightHandOut.x, vRightHandOut.y), ImVec2(vElbowRightOut.x, vElbowRightOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftHandOut.x, vLeftHandOut.y), ImVec2(vLeftHandOut1.x, vLeftHandOut1.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightHandOut.x, vRightHandOut.y), ImVec2(vRightHandOut1.x, vRightHandOut1.y), color, globals::g_skeletonthickness);

			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftThighOut.x, vLeftThighOut.y), ImVec2(vHipOut.x, vHipOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightThighOut.x, vRightThighOut.y), ImVec2(vHipOut.x, vHipOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftCalfOut.x, vLeftCalfOut.y), ImVec2(vLeftThighOut.x, vLeftThighOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightCalfOut.x, vRightCalfOut.y), ImVec2(vRightThighOut.x, vRightThighOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftFootOut.x, vLeftFootOut.y), ImVec2(vLeftCalfOut.x, vLeftCalfOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightFootOut.x, vRightFootOut.y), ImVec2(vRightCalfOut.x, vRightCalfOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftFootOut.x, vLeftFootOut.y), ImVec2(vLeftHeelOut.x, vLeftHeelOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightFootOut.x, vRightFootOut.y), ImVec2(vRightHeelOut.x, vRightHeelOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vLeftHeelOut.x, vLeftHeelOut.y), ImVec2(vLeftToeOut.x, vLeftToeOut.y), color, globals::g_skeletonthickness);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vRightHeelOut.x, vRightHeelOut.y), ImVec2(vRightToeOut.x, vRightToeOut.y), color, globals::g_skeletonthickness);
		}


	};  static uee::ue* ue5 = new uee::ue();
}


