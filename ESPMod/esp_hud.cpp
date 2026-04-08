
#include "SDK/Basic.hpp"
#include "SDK/Engine_classes.hpp"

#include "SDK/LearnUE1rd_classes.hpp"

#include "esp_hud.h"

#include <imgui.h>
using namespace SDK;
#include <windows.h>
#include <cmath>

constexpr double PI = 3.14159265358979323846;
constexpr double DEG2RAD = PI / 180.0;
constexpr double HALF_CHARACTER_HEIGHT = 90.0;

namespace CamOff
{
    constexpr uintptr_t PlayerCameraManager = 0x0378;
    constexpr uintptr_t ViewTarget  = 0x0340;
    constexpr uintptr_t POV         = 0x0010;
    constexpr uintptr_t Location    = 0x0000;
    constexpr uintptr_t Rotation    = 0x0018;
    constexpr uintptr_t FOV         = 0x0030;
}

struct CamPOV
{
    double LocX, LocY, LocZ;
    double RotPitch, RotYaw, RotRoll;
    float  FOV;
};

static CamPOV ReadCamPOV(APlayerController* PC)
{
    CamPOV pov = {};
    uintptr_t camMgr = *(uintptr_t*)((uintptr_t)PC + CamOff::PlayerCameraManager);
    if (!camMgr) return pov;
    uintptr_t base = camMgr + CamOff::ViewTarget + CamOff::POV;
    pov.LocX     = *(double*)(base + CamOff::Location);
    pov.LocY     = *(double*)(base + CamOff::Location + 8);
    pov.LocZ     = *(double*)(base + CamOff::Location + 16);
    pov.RotPitch = *(double*)(base + CamOff::Rotation);
    pov.RotYaw   = *(double*)(base + CamOff::Rotation + 8);
    pov.RotRoll  = *(double*)(base + CamOff::Rotation + 16);
    pov.FOV      = *(float*)(base + CamOff::FOV);
    return pov;
}

static bool WorldToScreen(const CamPOV& cam, const FVector& WorldLocation, FVector2D& ScreenLocation)
{
    double CamFOV = static_cast<double>(cam.FOV);
    if (CamFOV <= 0.0) return false;

    double Pitch = cam.RotPitch * DEG2RAD;
    double Yaw   = cam.RotYaw   * DEG2RAD;

    double CP = cos(Pitch), SP = sin(Pitch);
    double CY = cos(Yaw),   SY = sin(Yaw);

    double Dx = WorldLocation.X - cam.LocX;
    double Dy = WorldLocation.Y - cam.LocY;
    double Dz = WorldLocation.Z - cam.LocZ;

    double Z = Dx * (CP * CY) + Dy * (CP * SY) + Dz * SP;
    if (Z < 0.1) return false;

    double X = Dx * (-SY)        + Dy * CY;
    double Y = Dx * (-(SP * CY)) + Dy * (-(SP * SY)) + Dz * CP;

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    double CenterX = displaySize.x / 2.0;
    double CenterY = displaySize.y / 2.0;

    double TanHalfFOV = tan(CamFOV * DEG2RAD * 0.5);
    double AspectRatio = displaySize.x / static_cast<double>(displaySize.y);

    ScreenLocation.X = CenterX + (X / Z) * (CenterX / (TanHalfFOV * AspectRatio));
    ScreenLocation.Y = CenterY - (Y / Z) * (CenterY / TanHalfFOV);

    return true;
}

static FVector GetActorWorldPosition(AActor* Actor)
{
    FVector Pos = { 0.0, 0.0, 0.0 };
    if (!Actor) return Pos;

    USceneComponent* Root = Actor->RootComponent;
    if (!Root) return Pos;

    return Root->RelativeLocation;
}

void RenderLoop()
{
    MessageBoxA(nullptr, "", "", MB_OK);
}

static void DbgText(float x, float& y, ImU32 color, const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), color, buf);
    y += 16.0f;
    printf("%s\n", buf);
}

static void DrawESP_HUD_Internal()
{
    float dbgY = 50.0f;
    const ImU32 WHITE = IM_COL32(255, 255, 255, 255);
    const ImU32 GREEN = IM_COL32(0, 255, 0, 255);
    const ImU32 RED   = IM_COL32(255, 0, 0, 255);

    DbgText(10, dbgY, WHITE, "[ESP] DrawESP_HUD_Internal entered");

    if (!SDK::Offsets::GWorld) { DbgText(10, dbgY, RED, "[ESP] GWorld offset is 0"); return; }

    SDK::UWorld* World = UWorld::GetWorld();
    if (!World) { DbgText(10, dbgY, RED, "[ESP] World is null"); return; }
    if (!World->PersistentLevel) { DbgText(10, dbgY, RED, "[ESP] PersistentLevel is null"); return; }

    SDK::UGameInstance* GameInstance = World->OwningGameInstance;
    if (!GameInstance) { DbgText(10, dbgY, RED, "[ESP] GameInstance is null"); return; }
    if (GameInstance->LocalPlayers.Num() == 0) { DbgText(10, dbgY, RED, "[ESP] No LocalPlayers"); return; }

    ULocalPlayer* LocalPlayer = GameInstance->LocalPlayers[0];
    if (!LocalPlayer) { DbgText(10, dbgY, RED, "[ESP] LocalPlayer is null"); return; }
    if (!LocalPlayer->PlayerController) { DbgText(10, dbgY, RED, "[ESP] PlayerController is null"); return; }

    APlayerController* PC = (APlayerController*)LocalPlayer->PlayerController;

    uintptr_t rawCamMgr = *(uintptr_t*)((uintptr_t)PC + CamOff::PlayerCameraManager);
    if (!rawCamMgr) { DbgText(10, dbgY, RED, "[ESP] PlayerCameraManager is null (raw 0x378)"); return; }

    CamPOV cam = ReadCamPOV(PC);

    // #region agent log — post-fix verification
    {
        FILE* lf = fopen("debug-98387e.log", "a");
        if (lf) {
            fprintf(lf, "{\"sessionId\":\"98387e\",\"hypothesisId\":\"A-fix\",\"runId\":\"post-fix\",\"location\":\"esp_hud.cpp:140\",\"message\":\"cam_data\",\"data\":{\"rawCamMgr\":\"0x%llX\",\"fov\":%.2f,\"locX\":%.2f,\"locY\":%.2f,\"locZ\":%.2f,\"rotP\":%.2f,\"rotY\":%.2f},\"timestamp\":%llu}\n",
                (unsigned long long)rawCamMgr, cam.FOV, cam.LocX, cam.LocY, cam.LocZ, cam.RotPitch, cam.RotYaw,
                (unsigned long long)GetTickCount64());
            fclose(lf);
        }
    }
    // #endregion

    DbgText(10, dbgY, GREEN, "[ESP] POV FOV=%.1f Pos=(%.1f,%.1f,%.1f) Rot=(%.1f,%.1f,%.1f)",
        cam.FOV, cam.LocX, cam.LocY, cam.LocZ, cam.RotPitch, cam.RotYaw, cam.RotRoll);

    ImVec2 ds = ImGui::GetIO().DisplaySize;
    DbgText(10, dbgY, WHITE, "[ESP] DisplaySize: %.0f x %.0f", ds.x, ds.y);

    int totalActors = World->PersistentLevel->Actors.Num();
    DbgText(10, dbgY, WHITE, "[ESP] Total actors in level: %d", totalActors);

    UClass* EnemyClass = AEnemyCharacter::StaticClass();
    DbgText(10, dbgY, WHITE, "[ESP] EnemyCharacter StaticClass: %p", EnemyClass);

    int enemyCount = 0;
    int drawnCount = 0;

    for (int i = 0; i < totalActors; ++i)
    {
        AActor* Actor = World->PersistentLevel->Actors[i];
        if (!Actor || Actor == PC->AcknowledgedPawn) continue;

        if (!EnemyClass) continue;
        if (!Actor->IsA(EnemyClass)) continue;

        enemyCount++;

        FVector Origin = GetActorWorldPosition(Actor);
        DbgText(10, dbgY, GREEN, "[ESP] Enemy %d Pos: %.1f, %.1f, %.1f", enemyCount, Origin.X, Origin.Y, Origin.Z);

        FVector Top    = { Origin.X, Origin.Y, Origin.Z + HALF_CHARACTER_HEIGHT };
        FVector Bottom = { Origin.X, Origin.Y, Origin.Z - HALF_CHARACTER_HEIGHT };

        FVector2D ScreenTop, ScreenBottom;
        bool bTopVisible = WorldToScreen(cam, Top, ScreenTop);
        bool bBottomVisible = WorldToScreen(cam, Bottom, ScreenBottom);

        DbgText(10, dbgY, WHITE, "  W2S Top: %s (%.0f,%.0f) Bot: %s (%.0f,%.0f)",
            bTopVisible ? "OK" : "FAIL", ScreenTop.X, ScreenTop.Y,
            bBottomVisible ? "OK" : "FAIL", ScreenBottom.X, ScreenBottom.Y);

        if (bTopVisible && bBottomVisible)
        {
            float Height = (float)fabs(ScreenBottom.Y - ScreenTop.Y);
            float Width = Height * 0.5f;

            float CX = (float)(ScreenTop.X + ScreenBottom.X) * 0.5f;
            float CYs = (float)(ScreenTop.Y + ScreenBottom.Y) * 0.5f;

            ImVec2 upper_left  = ImVec2(CX - Width / 2, CYs - Height / 2);
            ImVec2 lower_right = ImVec2(CX + Width / 2, CYs + Height / 2);

            ImGui::GetBackgroundDrawList()->AddRect(upper_left, lower_right, ImColor(255, 255, 0), 0, 0, 5);
            drawnCount++;
        }
    }

    DbgText(10, dbgY, WHITE, "[ESP] Enemies found: %d, Drawn: %d", enemyCount, drawnCount);
}

void DrawESP_HUD()
{
    static bool g_once = true;
    if (g_once)
    {
        MessageBoxA(nullptr, "ESP", "ESP", MB_OK);
        g_once = false;
    }

    __try
    {
        DrawESP_HUD_Internal();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}