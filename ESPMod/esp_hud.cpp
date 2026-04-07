
#include "SDK/Basic.hpp"
#include "SDK/Engine_classes.hpp"

#include "SDK/LearnUE1rd_classes.hpp"

#include "esp_hud.h"

#include <imgui.h>
using namespace SDK;
#include <windows.h>



void RenderLoop()
{
    MessageBoxA(nullptr, "", "", MB_OK);

   /*
    while (true)
    {
        DrawESP_HUD();
        Sleep(16); // 每帧调用
    }
    */
}


void DrawESP_HUD()
{
    static bool g_once = true;
	if (g_once)
	{
        MessageBoxA(nullptr, "ESP", "ESP", MB_OK);
        //StartMainThread();
        g_once = false;
	}


    if (!SDK::Offsets::GWorld) return;

    SDK::UWorld* World = UWorld::GetWorld();
    if (!World || !World->PersistentLevel) return;

    SDK::UGameInstance* GameInstance = World->OwningGameInstance;
    if (!GameInstance || GameInstance->LocalPlayers.Num() == 0) return;

    ULocalPlayer* LocalPlayer = GameInstance->LocalPlayers[0];
    if (!LocalPlayer || !LocalPlayer->PlayerController) return;

    APlayerController* PC = (APlayerController*)LocalPlayer->PlayerController;
    AHUD* HUD = PC->GetHUD();

    if (!HUD ) return;


    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(&CameraLocation, &CameraRotation);

    for (int i = 0; i < World->PersistentLevel->Actors.Num(); ++i)
    {
        AActor* Actor = World->PersistentLevel->Actors[i];
        if (!Actor || Actor == PC->AcknowledgedPawn) continue;

        if (!Actor->IsA(AEnemyCharacter::StaticClass())) continue;

        {
            AEnemyCharacter* Enemy = (AEnemyCharacter*)Actor;

            FVector Origin;
            FVector BoxExtent;
            Enemy->GetActorBounds(true, &Origin, &BoxExtent,false);

            // 计算包围盒顶点（上、下）
            FVector Top = Origin + FVector(0, 0, BoxExtent.Z);
            FVector Bottom = Origin - FVector(0, 0, BoxExtent.Z);

            FVector2D ScreenTop, ScreenBottom;
            bool bTopVisible = PC->ProjectWorldLocationToScreen(Top, &ScreenTop,false);
            bool bBottomVisible = PC->ProjectWorldLocationToScreen(Bottom, &ScreenBottom,false);

            if (bTopVisible && bBottomVisible)
            {
                float Height = ::abs(ScreenBottom.Y - ScreenTop.Y);
                float Width = Height * 0.5f;

                FVector2D BoxCenter = (ScreenTop + ScreenBottom) * 0.5f;
                FVector2D BoxTopLeft = FVector2D(BoxCenter.X - Width / 2, BoxCenter.Y - Height / 2);

                // 绘制矩形框

                /*
            	FLinearColor BoxColor(0.f, 1.f, 0.f, 1.f); // Green

            	float Thickness = 5.0f;

                // 四条边
                HUD->DrawLine(BoxTopLeft.X, BoxTopLeft.Y, BoxTopLeft.X + Width, BoxTopLeft.Y, BoxColor, Thickness); // Top
                HUD->DrawLine(BoxTopLeft.X, BoxTopLeft.Y, BoxTopLeft.X, BoxTopLeft.Y + Height, BoxColor, Thickness); // Left
                HUD->DrawLine(BoxTopLeft.X + Width, BoxTopLeft.Y, BoxTopLeft.X + Width, BoxTopLeft.Y + Height, BoxColor, Thickness); // Right
                HUD->DrawLine(BoxTopLeft.X, BoxTopLeft.Y + Height, BoxTopLeft.X + Width, BoxTopLeft.Y + Height, BoxColor, Thickness); // Bottom

                */
                ImVec2 upper_left = ImVec2(BoxTopLeft.X, BoxTopLeft.Y);
                ImVec2 ower_right = ImVec2(BoxTopLeft.X + Width, BoxTopLeft.Y + Height);

                ImColor m_cBox2D = ImColor(255, 255, 0);
                ImGui::GetBackgroundDrawList()->AddRect(upper_left, ower_right, m_cBox2D, 0, 0,5);
                break;
            }
	        
        }


        // 显示名字、血量、距离
        std::string Name = Actor->GetName();
        //printf("Actor Name %s \n", Name.c_str());
        
    }
}