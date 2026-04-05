#pragma once

#ifdef _DEBUG

#include "imgui.h"
#include <string>
#include <vector>
#include <functional>

namespace PAIN {
    namespace Editor {

        // ============================================================
        //  EditorTheme
        //  All colour/style tokens used across every editor panel.
        //  Add new tokens here, never hardcode ImVec4 in panel files.
        // ============================================================
        struct EditorTheme {

            // ----------------------------------------------------------
            // General text
            // ----------------------------------------------------------
            ImVec4 textDefault         = { 1.00f, 1.00f, 1.00f, 1.00f };
            ImVec4 textMuted           = { 0.60f, 0.60f, 0.60f, 1.00f };
            ImVec4 textDisabled        = { 0.45f, 0.45f, 0.45f, 1.00f };
            ImVec4 textWarning         = { 1.00f, 0.80f, 0.20f, 1.00f };
            ImVec4 textError           = { 1.00f, 0.30f, 0.30f, 1.00f };
            ImVec4 textSuccess         = { 0.20f, 1.00f, 0.20f, 1.00f };
            ImVec4 textInfo            = { 0.80f, 0.90f, 1.00f, 1.00f };
            ImVec4 textHighlight       = { 0.40f, 0.70f, 1.00f, 1.00f };
            ImVec4 textAccent          = { 0.30f, 0.80f, 1.00f, 1.00f };
            ImVec4 textSceneName       = { 0.70f, 0.70f, 0.70f, 1.00f };

            // ----------------------------------------------------------
            // Log console colours
            // ----------------------------------------------------------
            ImVec4 logTrace            = { 0.60f, 0.60f, 0.60f, 1.00f };
            ImVec4 logInfo             = { 0.40f, 0.90f, 0.40f, 1.00f };
            ImVec4 logWarn             = { 1.00f, 0.80f, 0.00f, 1.00f };
            ImVec4 logError            = { 1.00f, 0.30f, 0.30f, 1.00f };
            ImVec4 logCritical         = { 1.00f, 0.00f, 0.50f, 1.00f };

            // ----------------------------------------------------------
            // Undo / Redo history
            // ----------------------------------------------------------
            ImVec4 historyUndo         = { 0.40f, 1.00f, 0.40f, 1.00f };
            ImVec4 historyRedo         = { 0.40f, 0.70f, 1.00f, 1.00f };
            ImVec4 historyNext         = { 1.00f, 1.00f, 0.30f, 1.00f };

            // ----------------------------------------------------------
            // Buttons — danger (delete / discard)
            // ----------------------------------------------------------
            ImVec4 btnDanger           = { 0.75f, 0.20f, 0.20f, 1.00f };
            ImVec4 btnDangerHovered    = { 0.95f, 0.30f, 0.30f, 1.00f };
            ImVec4 btnDangerActive     = { 0.60f, 0.10f, 0.10f, 1.00f };

            // Buttons — success (save / apply)
            ImVec4 btnSuccess          = { 0.20f, 0.70f, 0.20f, 1.00f };
            ImVec4 btnSuccessHovered   = { 0.30f, 0.80f, 0.30f, 1.00f };
            ImVec4 btnSuccessActive    = { 0.10f, 0.60f, 0.10f, 1.00f };

            // Buttons — primary (add / accept)
            ImVec4 btnPrimary          = { 0.20f, 0.50f, 0.80f, 0.80f };
            ImVec4 btnPrimaryHovered   = { 0.30f, 0.60f, 0.90f, 1.00f };
            ImVec4 btnPrimaryActive    = { 0.15f, 0.40f, 0.70f, 1.00f };

            // Buttons — secondary (neutral actions)
            ImVec4 btnSecondary        = { 0.20f, 0.20f, 0.20f, 0.60f };
            ImVec4 btnSecondaryHovered = { 0.30f, 0.30f, 0.30f, 0.80f };
            ImVec4 btnSecondaryActive  = { 0.40f, 0.40f, 0.40f, 0.50f };

            // Buttons — accent (highlight active gizmo / mode)
            ImVec4 btnAccent           = { 0.20f, 0.55f, 1.00f, 0.80f };
            ImVec4 btnAccentHovered    = { 0.30f, 0.70f, 1.00f, 1.00f };

            // Buttons — brown / tertiary
            ImVec4 btnTertiary         = { 0.60f, 0.35f, 0.20f, 0.80f };
            ImVec4 btnTertiaryHovered  = { 0.80f, 0.45f, 0.25f, 1.00f };
            ImVec4 btnTertiaryActive   = { 0.50f, 0.25f, 0.15f, 1.00f };

            // Invisible / ghost button (EntityPanel tree arrows)
            ImVec4 btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            ImVec4 btnGhostHovered     = { 0.30f, 0.30f, 0.30f, 0.50f };
            ImVec4 btnGhostActive      = { 0.40f, 0.40f, 0.40f, 0.50f };

            // ----------------------------------------------------------
            // Component-header pill colours  (IM_COL32 stored as ImU32)
            // ----------------------------------------------------------
            ImU32 compTransform        = IM_COL32(255, 220,  80, 255); // yellow
            ImU32 compMetadata         = IM_COL32(180, 180, 180, 255); // grey
            ImU32 compRendering        = IM_COL32( 80, 160, 255, 255); // blue
            ImU32 compLighting         = IM_COL32(255, 200,  80, 255); // warm yellow
            ImU32 compPhysics          = IM_COL32(255, 140,  40, 255); // orange
            ImU32 compBounding         = IM_COL32(120, 200, 120, 255); // green
            ImU32 compAnimation        = IM_COL32(180,  80, 255, 255); // purple
            ImU32 compAudio            = IM_COL32( 80, 220, 200, 255); // teal
            ImU32 compAI               = IM_COL32(255,  80,  80, 255); // red
            ImU32 compParticle         = IM_COL32(255, 160, 200, 255); // pink
            ImU32 compUI               = IM_COL32( 80, 200, 255, 255); // light blue
            ImU32 compScript           = IM_COL32(120, 220, 120, 255); // green
            ImU32 compPrefab           = IM_COL32(100, 180, 255, 255); // periwinkle
            ImU32 compDefault          = IM_COL32(160, 160, 160, 255); // fallback grey

            // Component header button (collapse / expand)
            ImVec4 compHeaderBtn       = { 0.20f, 0.40f, 0.80f, 1.00f };
            ImVec4 compHeaderBtnHov    = { 0.30f, 0.50f, 0.90f, 1.00f };
            ImVec4 compHeaderBtnAct    = { 0.15f, 0.35f, 0.70f, 1.00f };

            // ----------------------------------------------------------
            // Scene-panel card / layer colours
            // ----------------------------------------------------------
            ImU32 sceneCardActive      = IM_COL32( 40,  80,  55, 255);
            ImU32 sceneCardSelected    = IM_COL32( 50,  70, 100, 255);
            ImU32 sceneCardDefault     = IM_COL32( 38,  42,  48, 255);
            ImU32 sceneBorderActive    = IM_COL32( 80, 200, 120, 255);
            ImU32 sceneBorderSelected  = IM_COL32(100, 150, 230, 255);
            ImU32 sceneBorderDefault   = IM_COL32( 60,  65,  70, 255);
            ImU32 sceneThumb           = IM_COL32( 30,  30,  35, 255);
            ImU32 sceneThumbBorder     = IM_COL32( 70,  70,  80, 255);
            ImU32 sceneThumbText       = IM_COL32(150, 150, 170, 255);

            ImVec4 sceneTextActive     = { 0.40f, 1.00f, 0.60f, 1.00f };
            ImVec4 sceneTextActiveHov  = { 0.30f, 0.90f, 0.50f, 1.00f };
            ImVec4 sceneTextInactive   = { 0.45f, 0.45f, 0.50f, 1.00f };
            ImVec4 sceneActiveLabel    = { 0.70f, 0.90f, 1.00f, 1.00f };
            ImVec4 sceneHeading        = { 0.90f, 0.90f, 1.00f, 1.00f };
            ImVec4 sceneSubheading     = { 0.90f, 0.90f, 0.50f, 1.00f };
            ImVec4 scenePreviewLabel   = { 0.80f, 1.00f, 0.80f, 1.00f };
            ImVec4 sceneWarning        = { 1.00f, 0.50f, 0.00f, 1.00f };

            ImU32 layerCardSelected    = IM_COL32( 60,  80, 100, 255);
            ImU32 layerCardDefault     = IM_COL32( 40,  45,  50, 255);
            ImVec4 layerFrameBg        = { 0.20f, 0.25f, 0.30f, 0.80f };
            ImVec4 layerCountActive    = { 0.70f, 0.90f, 0.70f, 1.00f };
            ImVec4 layerCountEmpty     = { 0.60f, 0.60f, 0.60f, 1.00f };

            // ----------------------------------------------------------
            // Prefab panel
            // ----------------------------------------------------------
            ImVec4 prefabWarning       = { 1.00f, 0.80f, 0.20f, 1.00f };
            ImVec4 prefabNoChanges     = { 0.50f, 0.50f, 0.50f, 1.00f };
            ImVec4 prefabInstances     = { 1.00f, 1.00f, 1.00f, 1.00f };

            // ----------------------------------------------------------
            // Viewport panel
            // ----------------------------------------------------------
            ImVec4 vpTextMuted         = { 0.70f, 0.70f, 0.70f, 1.00f };
            ImU32  vpSelectFill        = IM_COL32(100, 180, 255,  30);
            ImU32  vpSelectBorder      = IM_COL32(100, 180, 255, 255);
            ImU32  vpSelectHandle      = IM_COL32(100, 180, 255, 255);

            // ----------------------------------------------------------
            // Animation panel
            // ----------------------------------------------------------
            ImVec4 animEntityName      = { 0.80f, 0.90f, 1.00f, 1.00f };
            ImVec4 animError           = { 1.00f, 0.30f, 0.30f, 1.00f };
            ImVec4 animControls        = { 0.30f, 0.80f, 1.00f, 1.00f };
            ImVec4 animMuted           = { 0.70f, 0.70f, 0.70f, 1.00f };
            ImVec4 animPlaying         = { 0.00f, 1.00f, 0.00f, 1.00f };
            ImVec4 animPaused          = { 0.70f, 0.70f, 0.70f, 1.00f };
            ImU32  animTimelineBg      = IM_COL32( 40,  40,  40, 255);
            ImU32  animTimelineGrid    = IM_COL32(100, 100, 100, 255);
            ImU32  animTimelineTick    = IM_COL32(150, 150, 150, 255);
            ImU32  animTimelineLabel   = IM_COL32(200, 200, 200, 255);
            ImU32  animKeyframe        = IM_COL32(255, 200,   0, 255);

            // ----------------------------------------------------------
            // Entity panel
            // ----------------------------------------------------------
            ImVec4 entityWarning       = { 1.00f, 0.60f, 0.00f, 1.00f };
            ImVec4 entityDragHint      = { 0.60f, 0.60f, 0.60f, 1.00f };
            ImVec4 entityPrefabTint    = { 1.00f, 0.90f, 0.30f, 1.00f };
            ImVec4 entityTemplateTint  = { 0.40f, 0.70f, 1.00f, 1.00f };
            ImVec4 entityNormalTint    = { 0.70f, 0.90f, 1.00f, 1.00f };

            // ----------------------------------------------------------
            // Theme meta
            // ----------------------------------------------------------
            std::string name           = "Dark";
        };

        // ============================================================
        //  Built-in presets
        // ============================================================
        inline EditorTheme makeThemeDark() {
            EditorTheme t;
            t.name = "Dark";
            return t; // defaults already match Dark
        }

        inline EditorTheme makeThemeLight() {
            EditorTheme t;
            t.name = "Light";

            t.textDefault         = { 0.10f, 0.10f, 0.10f, 1.00f };
            t.textMuted           = { 0.40f, 0.40f, 0.40f, 1.00f };
            t.textDisabled        = { 0.60f, 0.60f, 0.60f, 1.00f };
            t.textWarning         = { 0.80f, 0.50f, 0.00f, 1.00f };
            t.textError           = { 0.85f, 0.10f, 0.10f, 1.00f };
            t.textSuccess         = { 0.10f, 0.65f, 0.10f, 1.00f };
            t.textInfo            = { 0.10f, 0.40f, 0.80f, 1.00f };
            t.textHighlight       = { 0.10f, 0.45f, 0.85f, 1.00f };
            t.textAccent          = { 0.00f, 0.50f, 0.80f, 1.00f };
            t.textSceneName       = { 0.30f, 0.30f, 0.30f, 1.00f };

            t.logTrace            = { 0.50f, 0.50f, 0.50f, 1.00f };
            t.logInfo             = { 0.10f, 0.60f, 0.10f, 1.00f };
            t.logWarn             = { 0.75f, 0.55f, 0.00f, 1.00f };
            t.logError            = { 0.85f, 0.10f, 0.10f, 1.00f };
            t.logCritical         = { 0.70f, 0.00f, 0.30f, 1.00f };

            t.historyUndo         = { 0.10f, 0.65f, 0.10f, 1.00f };
            t.historyRedo         = { 0.10f, 0.40f, 0.80f, 1.00f };
            t.historyNext         = { 0.70f, 0.60f, 0.00f, 1.00f };

            t.btnDanger           = { 0.85f, 0.20f, 0.20f, 1.00f };
            t.btnDangerHovered    = { 1.00f, 0.30f, 0.30f, 1.00f };
            t.btnDangerActive     = { 0.65f, 0.10f, 0.10f, 1.00f };

            t.btnSuccess          = { 0.15f, 0.65f, 0.15f, 1.00f };
            t.btnSuccessHovered   = { 0.20f, 0.80f, 0.20f, 1.00f };
            t.btnSuccessActive    = { 0.10f, 0.50f, 0.10f, 1.00f };

            t.btnPrimary          = { 0.20f, 0.50f, 0.85f, 0.90f };
            t.btnPrimaryHovered   = { 0.30f, 0.60f, 0.95f, 1.00f };
            t.btnPrimaryActive    = { 0.15f, 0.40f, 0.75f, 1.00f };

            t.btnSecondary        = { 0.75f, 0.75f, 0.75f, 1.00f };
            t.btnSecondaryHovered = { 0.85f, 0.85f, 0.85f, 1.00f };
            t.btnSecondaryActive  = { 0.65f, 0.65f, 0.65f, 1.00f };

            t.btnAccent           = { 0.20f, 0.55f, 1.00f, 0.90f };
            t.btnAccentHovered    = { 0.30f, 0.70f, 1.00f, 1.00f };

            t.sceneCardActive     = IM_COL32(180, 230, 200, 255);
            t.sceneCardSelected   = IM_COL32(200, 215, 240, 255);
            t.sceneCardDefault    = IM_COL32(230, 232, 235, 255);
            t.sceneBorderActive   = IM_COL32( 40, 160,  80, 255);
            t.sceneBorderSelected = IM_COL32( 60, 110, 200, 255);
            t.sceneBorderDefault  = IM_COL32(180, 185, 190, 255);
            t.sceneThumb          = IM_COL32(240, 240, 245, 255);
            t.sceneThumbBorder    = IM_COL32(160, 160, 170, 255);
            t.sceneThumbText      = IM_COL32( 80,  80,  90, 255);

            t.sceneTextActive     = { 0.10f, 0.65f, 0.25f, 1.00f };
            t.sceneTextActiveHov  = { 0.10f, 0.55f, 0.20f, 1.00f };
            t.sceneTextInactive   = { 0.40f, 0.40f, 0.45f, 1.00f };
            t.sceneActiveLabel    = { 0.10f, 0.50f, 0.75f, 1.00f };
            t.sceneHeading        = { 0.15f, 0.15f, 0.30f, 1.00f };
            t.sceneSubheading     = { 0.35f, 0.35f, 0.05f, 1.00f };
            t.scenePreviewLabel   = { 0.10f, 0.55f, 0.10f, 1.00f };
            t.sceneWarning        = { 0.75f, 0.35f, 0.00f, 1.00f };

            t.layerCardSelected   = IM_COL32(190, 210, 235, 255);
            t.layerCardDefault    = IM_COL32(215, 220, 225, 255);
            t.layerFrameBg        = { 0.85f, 0.88f, 0.92f, 0.90f };
            t.layerCountActive    = { 0.15f, 0.60f, 0.15f, 1.00f };
            t.layerCountEmpty     = { 0.50f, 0.50f, 0.50f, 1.00f };

            t.prefabWarning       = { 0.80f, 0.50f, 0.00f, 1.00f };
            t.prefabNoChanges     = { 0.45f, 0.45f, 0.45f, 1.00f };

            t.entityWarning       = { 0.75f, 0.40f, 0.00f, 1.00f };
            t.entityDragHint      = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.entityPrefabTint    = { 0.75f, 0.65f, 0.10f, 1.00f };
            t.entityTemplateTint  = { 0.20f, 0.50f, 0.85f, 1.00f };
            t.entityNormalTint    = { 0.30f, 0.55f, 0.80f, 1.00f };

            t.animEntityName      = { 0.10f, 0.40f, 0.75f, 1.00f };
            t.animError           = { 0.85f, 0.10f, 0.10f, 1.00f };
            t.animControls        = { 0.10f, 0.55f, 0.80f, 1.00f };
            t.animMuted           = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.animPlaying         = { 0.10f, 0.70f, 0.10f, 1.00f };
            t.animPaused          = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.animTimelineBg      = IM_COL32(220, 220, 220, 255);
            t.animTimelineGrid    = IM_COL32(160, 160, 160, 255);
            t.animTimelineTick    = IM_COL32(100, 100, 100, 255);
            t.animTimelineLabel   = IM_COL32( 60,  60,  60, 255);
            t.animKeyframe        = IM_COL32(200, 140,   0, 255);

            t.vpTextMuted         = { 0.35f, 0.35f, 0.35f, 1.00f };
            t.vpSelectFill        = IM_COL32( 30,  90, 200,  40);
            t.vpSelectBorder      = IM_COL32( 30,  90, 200, 255);
            t.vpSelectHandle      = IM_COL32( 30,  90, 200, 255);

            return t;
        }

        inline EditorTheme makeThemeDracula() {
            EditorTheme t;
            t.name = "Dracula";

            t.textDefault         = { 0.97f, 0.97f, 0.95f, 1.00f };
            t.textMuted           = { 0.63f, 0.64f, 0.82f, 1.00f };
            t.textDisabled        = { 0.44f, 0.46f, 0.62f, 1.00f };
            t.textWarning         = { 1.00f, 0.72f, 0.42f, 1.00f }; // orange
            t.textError           = { 1.00f, 0.33f, 0.33f, 1.00f }; // red
            t.textSuccess         = { 0.31f, 0.98f, 0.48f, 1.00f }; // green
            t.textInfo            = { 0.54f, 0.91f, 0.99f, 1.00f }; // cyan
            t.textHighlight       = { 0.74f, 0.58f, 0.98f, 1.00f }; // purple
            t.textAccent          = { 1.00f, 0.47f, 0.77f, 1.00f }; // pink
            t.textSceneName       = { 0.63f, 0.64f, 0.82f, 1.00f };

            t.logTrace            = { 0.63f, 0.64f, 0.82f, 1.00f };
            t.logInfo             = { 0.31f, 0.98f, 0.48f, 1.00f };
            t.logWarn             = { 1.00f, 0.72f, 0.42f, 1.00f };
            t.logError            = { 1.00f, 0.33f, 0.33f, 1.00f };
            t.logCritical         = { 1.00f, 0.20f, 0.60f, 1.00f };

            t.historyUndo         = { 0.31f, 0.98f, 0.48f, 1.00f };
            t.historyRedo         = { 0.54f, 0.91f, 0.99f, 1.00f };
            t.historyNext         = { 1.00f, 0.72f, 0.42f, 1.00f };

            t.btnDanger           = { 0.70f, 0.15f, 0.22f, 1.00f };
            t.btnDangerHovered    = { 1.00f, 0.33f, 0.33f, 1.00f };
            t.btnDangerActive     = { 0.55f, 0.10f, 0.15f, 1.00f };

            t.btnSuccess          = { 0.16f, 0.60f, 0.30f, 1.00f };
            t.btnSuccessHovered   = { 0.22f, 0.78f, 0.40f, 1.00f };
            t.btnSuccessActive    = { 0.10f, 0.45f, 0.22f, 1.00f };

            t.btnPrimary          = { 0.40f, 0.30f, 0.65f, 0.90f }; // purple
            t.btnPrimaryHovered   = { 0.55f, 0.42f, 0.80f, 1.00f };
            t.btnPrimaryActive    = { 0.30f, 0.22f, 0.55f, 1.00f };

            t.btnAccent           = { 0.80f, 0.25f, 0.55f, 0.85f }; // pink
            t.btnAccentHovered    = { 1.00f, 0.47f, 0.77f, 1.00f };

            t.sceneCardActive     = IM_COL32( 39, 67, 50, 255);
            t.sceneCardSelected   = IM_COL32( 55, 50, 80, 255);
            t.sceneCardDefault    = IM_COL32( 40, 42, 54, 255);
            t.sceneBorderActive   = IM_COL32( 80, 250, 123, 255);
            t.sceneBorderSelected = IM_COL32(189, 147, 249, 255);
            t.sceneBorderDefault  = IM_COL32( 68, 71,  90, 255);

            t.animTimelineBg      = IM_COL32( 30, 31, 40, 255);
            t.animTimelineGrid    = IM_COL32( 68, 71, 90, 255);
            t.animTimelineTick    = IM_COL32(139, 142, 168, 255);
            t.animTimelineLabel   = IM_COL32(200, 202, 215, 255);
            t.animKeyframe        = IM_COL32(255, 184,  108, 255); // orange

            t.animPlaying         = { 0.31f, 0.98f, 0.48f, 1.00f };
            t.animPaused          = { 0.63f, 0.64f, 0.82f, 1.00f };
            t.animControls        = { 0.54f, 0.91f, 0.99f, 1.00f };
            t.animEntityName      = { 0.54f, 0.91f, 0.99f, 1.00f };
            t.animError           = { 1.00f, 0.33f, 0.33f, 1.00f };

            t.entityPrefabTint    = { 1.00f, 0.72f, 0.42f, 1.00f };
            t.entityTemplateTint  = { 0.54f, 0.91f, 0.99f, 1.00f };
            t.entityNormalTint    = { 0.74f, 0.58f, 0.98f, 1.00f };

            return t;
        }

        inline EditorTheme makeThemeMidnight() {
            EditorTheme t;
            t.name = "Midnight";

            t.textDefault         = { 0.85f, 0.90f, 1.00f, 1.00f };
            t.textMuted           = { 0.50f, 0.55f, 0.70f, 1.00f };
            t.textDisabled        = { 0.35f, 0.38f, 0.50f, 1.00f };
            t.textWarning         = { 1.00f, 0.75f, 0.20f, 1.00f };
            t.textError           = { 1.00f, 0.35f, 0.35f, 1.00f };
            t.textSuccess         = { 0.25f, 0.90f, 0.55f, 1.00f };
            t.textInfo            = { 0.45f, 0.80f, 1.00f, 1.00f };
            t.textHighlight       = { 0.60f, 0.80f, 1.00f, 1.00f };
            t.textAccent          = { 0.35f, 0.70f, 1.00f, 1.00f };
            t.textSceneName       = { 0.50f, 0.55f, 0.70f, 1.00f };

            t.logTrace            = { 0.50f, 0.55f, 0.70f, 1.00f };
            t.logInfo             = { 0.25f, 0.90f, 0.55f, 1.00f };
            t.logWarn             = { 1.00f, 0.75f, 0.20f, 1.00f };
            t.logError            = { 1.00f, 0.35f, 0.35f, 1.00f };
            t.logCritical         = { 1.00f, 0.10f, 0.45f, 1.00f };

            t.historyUndo         = { 0.25f, 0.90f, 0.55f, 1.00f };
            t.historyRedo         = { 0.45f, 0.80f, 1.00f, 1.00f };
            t.historyNext         = { 1.00f, 0.90f, 0.30f, 1.00f };

            t.btnDanger           = { 0.65f, 0.12f, 0.18f, 1.00f };
            t.btnDangerHovered    = { 0.90f, 0.22f, 0.28f, 1.00f };
            t.btnDangerActive     = { 0.50f, 0.08f, 0.12f, 1.00f };

            t.btnSuccess          = { 0.12f, 0.55f, 0.30f, 1.00f };
            t.btnSuccessHovered   = { 0.18f, 0.75f, 0.42f, 1.00f };
            t.btnSuccessActive    = { 0.08f, 0.42f, 0.22f, 1.00f };

            t.btnPrimary          = { 0.15f, 0.35f, 0.65f, 0.90f };
            t.btnPrimaryHovered   = { 0.22f, 0.50f, 0.85f, 1.00f };
            t.btnPrimaryActive    = { 0.10f, 0.25f, 0.55f, 1.00f };

            t.animTimelineBg      = IM_COL32( 18, 20, 30, 255);
            t.animTimelineGrid    = IM_COL32( 45, 50, 70, 255);
            t.animTimelineTick    = IM_COL32( 90, 100, 130, 255);
            t.animTimelineLabel   = IM_COL32(155, 165, 195, 255);
            t.animKeyframe        = IM_COL32(255, 200,   0, 255);

            t.sceneCardActive     = IM_COL32( 20, 50, 40, 255);
            t.sceneCardSelected   = IM_COL32( 25, 35, 60, 255);
            t.sceneCardDefault    = IM_COL32( 18, 20, 30, 255);
            t.sceneBorderActive   = IM_COL32( 50, 180, 100, 255);
            t.sceneBorderSelected = IM_COL32( 70, 120, 210, 255);
            t.sceneBorderDefault  = IM_COL32( 40,  45,  60, 255);

            return t;
        }

        // ============================================================
        //  Pink — candy pink / rose dark theme
        // ============================================================
        inline EditorTheme makeThemePink() {
            EditorTheme t;
            t.name = "Pink";

            t.textDefault         = { 1.00f, 0.90f, 0.95f, 1.00f };
            t.textMuted           = { 0.80f, 0.60f, 0.70f, 1.00f };
            t.textDisabled        = { 0.55f, 0.40f, 0.48f, 1.00f };
            t.textWarning         = { 1.00f, 0.80f, 0.20f, 1.00f };
            t.textError           = { 1.00f, 0.30f, 0.40f, 1.00f };
            t.textSuccess         = { 0.40f, 1.00f, 0.60f, 1.00f };
            t.textInfo            = { 1.00f, 0.70f, 0.85f, 1.00f };
            t.textHighlight       = { 1.00f, 0.50f, 0.75f, 1.00f };
            t.textAccent          = { 1.00f, 0.40f, 0.70f, 1.00f };
            t.textSceneName       = { 0.90f, 0.65f, 0.75f, 1.00f };

            t.logTrace            = { 0.80f, 0.60f, 0.70f, 1.00f };
            t.logInfo             = { 0.40f, 1.00f, 0.60f, 1.00f };
            t.logWarn             = { 1.00f, 0.80f, 0.20f, 1.00f };
            t.logError            = { 1.00f, 0.30f, 0.40f, 1.00f };
            t.logCritical         = { 1.00f, 0.10f, 0.50f, 1.00f };

            t.historyUndo         = { 0.40f, 1.00f, 0.60f, 1.00f };
            t.historyRedo         = { 1.00f, 0.50f, 0.75f, 1.00f };
            t.historyNext         = { 1.00f, 0.90f, 0.30f, 1.00f };

            t.btnDanger           = { 0.75f, 0.15f, 0.25f, 1.00f };
            t.btnDangerHovered    = { 0.95f, 0.25f, 0.40f, 1.00f };
            t.btnDangerActive     = { 0.55f, 0.08f, 0.15f, 1.00f };

            t.btnSuccess          = { 0.20f, 0.65f, 0.35f, 1.00f };
            t.btnSuccessHovered   = { 0.30f, 0.80f, 0.45f, 1.00f };
            t.btnSuccessActive    = { 0.12f, 0.50f, 0.25f, 1.00f };

            t.btnPrimary          = { 0.75f, 0.20f, 0.50f, 0.85f };
            t.btnPrimaryHovered   = { 0.90f, 0.35f, 0.65f, 1.00f };
            t.btnPrimaryActive    = { 0.60f, 0.12f, 0.38f, 1.00f };

            t.btnSecondary        = { 0.35f, 0.20f, 0.28f, 0.70f };
            t.btnSecondaryHovered = { 0.50f, 0.30f, 0.40f, 0.90f };
            t.btnSecondaryActive  = { 0.60f, 0.35f, 0.48f, 0.60f };

            t.btnAccent           = { 0.85f, 0.30f, 0.60f, 0.85f };
            t.btnAccentHovered    = { 1.00f, 0.47f, 0.77f, 1.00f };

            t.btnTertiary         = { 0.55f, 0.25f, 0.38f, 0.80f };
            t.btnTertiaryHovered  = { 0.70f, 0.38f, 0.52f, 1.00f };
            t.btnTertiaryActive   = { 0.42f, 0.15f, 0.28f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.60f, 0.30f, 0.45f, 0.50f };
            t.btnGhostActive      = { 0.70f, 0.38f, 0.52f, 0.50f };

            t.compHeaderBtn       = { 0.75f, 0.20f, 0.50f, 1.00f };
            t.compHeaderBtnHov    = { 0.90f, 0.35f, 0.65f, 1.00f };
            t.compHeaderBtnAct    = { 0.60f, 0.12f, 0.38f, 1.00f };

            t.sceneCardActive     = IM_COL32( 80, 35, 55, 255);
            t.sceneCardSelected   = IM_COL32( 70, 30, 50, 255);
            t.sceneCardDefault    = IM_COL32( 45, 22, 33, 255);
            t.sceneBorderActive   = IM_COL32(255,  80, 150, 255);
            t.sceneBorderSelected = IM_COL32(220,  60, 120, 255);
            t.sceneBorderDefault  = IM_COL32( 90,  45,  65, 255);
            t.sceneThumb          = IM_COL32( 35, 18, 28, 255);
            t.sceneThumbBorder    = IM_COL32(100,  50,  75, 255);
            t.sceneThumbText      = IM_COL32(200, 140, 170, 255);

            t.sceneTextActive     = { 1.00f, 0.50f, 0.75f, 1.00f };
            t.sceneTextActiveHov  = { 0.90f, 0.40f, 0.65f, 1.00f };
            t.sceneTextInactive   = { 0.55f, 0.35f, 0.45f, 1.00f };
            t.sceneActiveLabel    = { 1.00f, 0.70f, 0.85f, 1.00f };
            t.sceneHeading        = { 1.00f, 0.85f, 0.90f, 1.00f };
            t.sceneSubheading     = { 0.90f, 0.65f, 0.75f, 1.00f };
            t.scenePreviewLabel   = { 0.80f, 1.00f, 0.85f, 1.00f };
            t.sceneWarning        = { 1.00f, 0.55f, 0.10f, 1.00f };

            t.layerCardSelected   = IM_COL32( 90, 40, 62, 255);
            t.layerCardDefault    = IM_COL32( 55, 26, 40, 255);
            t.layerFrameBg        = { 0.28f, 0.12f, 0.20f, 0.85f };
            t.layerCountActive    = { 1.00f, 0.60f, 0.80f, 1.00f };
            t.layerCountEmpty     = { 0.60f, 0.40f, 0.50f, 1.00f };

            t.prefabWarning       = { 1.00f, 0.80f, 0.20f, 1.00f };
            t.prefabNoChanges     = { 0.60f, 0.40f, 0.50f, 1.00f };
            t.prefabInstances     = { 1.00f, 0.90f, 0.95f, 1.00f };

            t.vpTextMuted         = { 0.80f, 0.60f, 0.70f, 1.00f };
            t.vpSelectFill        = IM_COL32(255,  80, 160,  30);
            t.vpSelectBorder      = IM_COL32(255,  80, 160, 255);
            t.vpSelectHandle      = IM_COL32(255, 120, 180, 255);

            t.animEntityName      = { 1.00f, 0.70f, 0.85f, 1.00f };
            t.animError           = { 1.00f, 0.30f, 0.40f, 1.00f };
            t.animControls        = { 1.00f, 0.50f, 0.75f, 1.00f };
            t.animMuted           = { 0.80f, 0.60f, 0.70f, 1.00f };
            t.animPlaying         = { 0.40f, 1.00f, 0.60f, 1.00f };
            t.animPaused          = { 0.80f, 0.60f, 0.70f, 1.00f };
            t.animTimelineBg      = IM_COL32( 35, 18, 28, 255);
            t.animTimelineGrid    = IM_COL32( 90, 45, 65, 255);
            t.animTimelineTick    = IM_COL32(160,  90, 120, 255);
            t.animTimelineLabel   = IM_COL32(220, 160, 190, 255);
            t.animKeyframe        = IM_COL32(255, 100, 180, 255);

            t.entityWarning       = { 1.00f, 0.65f, 0.10f, 1.00f };
            t.entityDragHint      = { 0.75f, 0.50f, 0.62f, 1.00f };
            t.entityPrefabTint    = { 1.00f, 0.80f, 0.40f, 1.00f };
            t.entityTemplateTint  = { 1.00f, 0.55f, 0.78f, 1.00f };
            t.entityNormalTint    = { 1.00f, 0.75f, 0.88f, 1.00f };

            return t;
        }

        // ============================================================
        //  Blue — ocean / cobalt dark theme
        // ============================================================
        inline EditorTheme makeThemeBlue() {
            EditorTheme t;
            t.name = "Blue";

            t.textDefault         = { 0.88f, 0.94f, 1.00f, 1.00f };
            t.textMuted           = { 0.55f, 0.70f, 0.88f, 1.00f };
            t.textDisabled        = { 0.38f, 0.50f, 0.65f, 1.00f };
            t.textWarning         = { 1.00f, 0.82f, 0.28f, 1.00f };
            t.textError           = { 1.00f, 0.38f, 0.38f, 1.00f };
            t.textSuccess         = { 0.30f, 1.00f, 0.65f, 1.00f };
            t.textInfo            = { 0.50f, 0.85f, 1.00f, 1.00f };
            t.textHighlight       = { 0.35f, 0.75f, 1.00f, 1.00f };
            t.textAccent          = { 0.20f, 0.65f, 1.00f, 1.00f };
            t.textSceneName       = { 0.55f, 0.70f, 0.88f, 1.00f };

            t.logTrace            = { 0.55f, 0.70f, 0.88f, 1.00f };
            t.logInfo             = { 0.30f, 1.00f, 0.65f, 1.00f };
            t.logWarn             = { 1.00f, 0.82f, 0.28f, 1.00f };
            t.logError            = { 1.00f, 0.38f, 0.38f, 1.00f };
            t.logCritical         = { 1.00f, 0.15f, 0.50f, 1.00f };

            t.historyUndo         = { 0.30f, 1.00f, 0.65f, 1.00f };
            t.historyRedo         = { 0.35f, 0.75f, 1.00f, 1.00f };
            t.historyNext         = { 1.00f, 0.92f, 0.30f, 1.00f };

            t.btnDanger           = { 0.65f, 0.12f, 0.18f, 1.00f };
            t.btnDangerHovered    = { 0.88f, 0.22f, 0.28f, 1.00f };
            t.btnDangerActive     = { 0.48f, 0.08f, 0.12f, 1.00f };

            t.btnSuccess          = { 0.10f, 0.55f, 0.32f, 1.00f };
            t.btnSuccessHovered   = { 0.18f, 0.72f, 0.44f, 1.00f };
            t.btnSuccessActive    = { 0.06f, 0.40f, 0.22f, 1.00f };

            t.btnPrimary          = { 0.10f, 0.40f, 0.80f, 0.90f };
            t.btnPrimaryHovered   = { 0.18f, 0.55f, 0.95f, 1.00f };
            t.btnPrimaryActive    = { 0.06f, 0.30f, 0.65f, 1.00f };

            t.btnSecondary        = { 0.12f, 0.22f, 0.38f, 0.70f };
            t.btnSecondaryHovered = { 0.20f, 0.32f, 0.52f, 0.90f };
            t.btnSecondaryActive  = { 0.28f, 0.42f, 0.65f, 0.60f };

            t.btnAccent           = { 0.15f, 0.55f, 1.00f, 0.85f };
            t.btnAccentHovered    = { 0.25f, 0.70f, 1.00f, 1.00f };

            t.btnTertiary         = { 0.15f, 0.35f, 0.60f, 0.80f };
            t.btnTertiaryHovered  = { 0.22f, 0.48f, 0.75f, 1.00f };
            t.btnTertiaryActive   = { 0.10f, 0.25f, 0.48f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.20f, 0.38f, 0.60f, 0.50f };
            t.btnGhostActive      = { 0.28f, 0.48f, 0.72f, 0.50f };

            t.compHeaderBtn       = { 0.10f, 0.42f, 0.82f, 1.00f };
            t.compHeaderBtnHov    = { 0.18f, 0.56f, 0.95f, 1.00f };
            t.compHeaderBtnAct    = { 0.06f, 0.30f, 0.65f, 1.00f };

            t.sceneCardActive     = IM_COL32( 18, 50, 80, 255);
            t.sceneCardSelected   = IM_COL32( 20, 45, 75, 255);
            t.sceneCardDefault    = IM_COL32( 14, 24, 40, 255);
            t.sceneBorderActive   = IM_COL32( 50, 180, 255, 255);
            t.sceneBorderSelected = IM_COL32( 80, 140, 230, 255);
            t.sceneBorderDefault  = IM_COL32( 35,  60,  90, 255);
            t.sceneThumb          = IM_COL32( 10, 18, 32, 255);
            t.sceneThumbBorder    = IM_COL32( 40,  75, 115, 255);
            t.sceneThumbText      = IM_COL32(120, 165, 210, 255);

            t.sceneTextActive     = { 0.35f, 0.75f, 1.00f, 1.00f };
            t.sceneTextActiveHov  = { 0.28f, 0.65f, 0.90f, 1.00f };
            t.sceneTextInactive   = { 0.38f, 0.52f, 0.68f, 1.00f };
            t.sceneActiveLabel    = { 0.55f, 0.85f, 1.00f, 1.00f };
            t.sceneHeading        = { 0.80f, 0.92f, 1.00f, 1.00f };
            t.sceneSubheading     = { 0.65f, 0.80f, 0.60f, 1.00f };
            t.scenePreviewLabel   = { 0.65f, 1.00f, 0.80f, 1.00f };
            t.sceneWarning        = { 1.00f, 0.55f, 0.10f, 1.00f };

            t.layerCardSelected   = IM_COL32( 28, 58, 95, 255);
            t.layerCardDefault    = IM_COL32( 16, 30, 52, 255);
            t.layerFrameBg        = { 0.08f, 0.18f, 0.32f, 0.85f };
            t.layerCountActive    = { 0.40f, 0.85f, 1.00f, 1.00f };
            t.layerCountEmpty     = { 0.38f, 0.52f, 0.68f, 1.00f };

            t.prefabWarning       = { 1.00f, 0.82f, 0.28f, 1.00f };
            t.prefabNoChanges     = { 0.40f, 0.55f, 0.72f, 1.00f };
            t.prefabInstances     = { 0.88f, 0.94f, 1.00f, 1.00f };

            t.vpTextMuted         = { 0.55f, 0.70f, 0.88f, 1.00f };
            t.vpSelectFill        = IM_COL32( 30, 120, 255,  35);
            t.vpSelectBorder      = IM_COL32( 30, 140, 255, 255);
            t.vpSelectHandle      = IM_COL32( 80, 170, 255, 255);

            t.animEntityName      = { 0.55f, 0.85f, 1.00f, 1.00f };
            t.animError           = { 1.00f, 0.38f, 0.38f, 1.00f };
            t.animControls        = { 0.30f, 0.72f, 1.00f, 1.00f };
            t.animMuted           = { 0.55f, 0.70f, 0.88f, 1.00f };
            t.animPlaying         = { 0.30f, 1.00f, 0.65f, 1.00f };
            t.animPaused          = { 0.55f, 0.70f, 0.88f, 1.00f };
            t.animTimelineBg      = IM_COL32( 10, 18, 32, 255);
            t.animTimelineGrid    = IM_COL32( 35, 60, 90, 255);
            t.animTimelineTick    = IM_COL32( 70, 115, 160, 255);
            t.animTimelineLabel   = IM_COL32(140, 185, 220, 255);
            t.animKeyframe        = IM_COL32( 80, 190, 255, 255);

            t.entityWarning       = { 1.00f, 0.65f, 0.10f, 1.00f };
            t.entityDragHint      = { 0.50f, 0.65f, 0.82f, 1.00f };
            t.entityPrefabTint    = { 1.00f, 0.88f, 0.35f, 1.00f };
            t.entityTemplateTint  = { 0.40f, 0.78f, 1.00f, 1.00f };
            t.entityNormalTint    = { 0.65f, 0.88f, 1.00f, 1.00f };

            return t;
        }

        // ============================================================
        //  Black — OLED / near-black minimal theme
        // ============================================================
        inline EditorTheme makeThemeBlack() {
            EditorTheme t;
            t.name = "Black";

            t.textDefault         = { 0.92f, 0.92f, 0.92f, 1.00f };
            t.textMuted           = { 0.55f, 0.55f, 0.55f, 1.00f };
            t.textDisabled        = { 0.35f, 0.35f, 0.35f, 1.00f };
            t.textWarning         = { 0.95f, 0.78f, 0.20f, 1.00f };
            t.textError           = { 0.95f, 0.30f, 0.30f, 1.00f };
            t.textSuccess         = { 0.30f, 0.90f, 0.45f, 1.00f };
            t.textInfo            = { 0.60f, 0.80f, 1.00f, 1.00f };
            t.textHighlight       = { 0.80f, 0.80f, 0.80f, 1.00f };
            t.textAccent          = { 0.70f, 0.70f, 0.70f, 1.00f };
            t.textSceneName       = { 0.55f, 0.55f, 0.55f, 1.00f };

            t.logTrace            = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.logInfo             = { 0.30f, 0.90f, 0.45f, 1.00f };
            t.logWarn             = { 0.95f, 0.78f, 0.20f, 1.00f };
            t.logError            = { 0.95f, 0.30f, 0.30f, 1.00f };
            t.logCritical         = { 1.00f, 0.08f, 0.40f, 1.00f };

            t.historyUndo         = { 0.30f, 0.90f, 0.45f, 1.00f };
            t.historyRedo         = { 0.50f, 0.72f, 1.00f, 1.00f };
            t.historyNext         = { 1.00f, 0.90f, 0.25f, 1.00f };

            t.btnDanger           = { 0.55f, 0.08f, 0.08f, 1.00f };
            t.btnDangerHovered    = { 0.78f, 0.15f, 0.15f, 1.00f };
            t.btnDangerActive     = { 0.40f, 0.04f, 0.04f, 1.00f };

            t.btnSuccess          = { 0.08f, 0.45f, 0.20f, 1.00f };
            t.btnSuccessHovered   = { 0.14f, 0.62f, 0.28f, 1.00f };
            t.btnSuccessActive    = { 0.04f, 0.30f, 0.14f, 1.00f };

            t.btnPrimary          = { 0.18f, 0.18f, 0.18f, 0.90f };
            t.btnPrimaryHovered   = { 0.28f, 0.28f, 0.28f, 1.00f };
            t.btnPrimaryActive    = { 0.12f, 0.12f, 0.12f, 1.00f };

            t.btnSecondary        = { 0.10f, 0.10f, 0.10f, 0.80f };
            t.btnSecondaryHovered = { 0.18f, 0.18f, 0.18f, 1.00f };
            t.btnSecondaryActive  = { 0.22f, 0.22f, 0.22f, 0.60f };

            t.btnAccent           = { 0.30f, 0.30f, 0.30f, 0.85f };
            t.btnAccentHovered    = { 0.45f, 0.45f, 0.45f, 1.00f };

            t.btnTertiary         = { 0.25f, 0.18f, 0.10f, 0.80f };
            t.btnTertiaryHovered  = { 0.38f, 0.28f, 0.16f, 1.00f };
            t.btnTertiaryActive   = { 0.18f, 0.12f, 0.06f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.22f, 0.22f, 0.22f, 0.50f };
            t.btnGhostActive      = { 0.30f, 0.30f, 0.30f, 0.50f };

            t.compHeaderBtn       = { 0.20f, 0.20f, 0.20f, 1.00f };
            t.compHeaderBtnHov    = { 0.32f, 0.32f, 0.32f, 1.00f };
            t.compHeaderBtnAct    = { 0.12f, 0.12f, 0.12f, 1.00f };

            t.sceneCardActive     = IM_COL32( 22, 40, 28, 255);
            t.sceneCardSelected   = IM_COL32( 20, 25, 38, 255);
            t.sceneCardDefault    = IM_COL32(  8,  8,  8, 255);
            t.sceneBorderActive   = IM_COL32( 60, 180,  90, 255);
            t.sceneBorderSelected = IM_COL32( 70, 110, 200, 255);
            t.sceneBorderDefault  = IM_COL32( 35,  35,  35, 255);
            t.sceneThumb          = IM_COL32(  4,  4,  4, 255);
            t.sceneThumbBorder    = IM_COL32( 45,  45,  45, 255);
            t.sceneThumbText      = IM_COL32(110, 110, 110, 255);

            t.sceneTextActive     = { 0.35f, 0.90f, 0.50f, 1.00f };
            t.sceneTextActiveHov  = { 0.28f, 0.78f, 0.42f, 1.00f };
            t.sceneTextInactive   = { 0.38f, 0.38f, 0.38f, 1.00f };
            t.sceneActiveLabel    = { 0.60f, 0.80f, 1.00f, 1.00f };
            t.sceneHeading        = { 0.82f, 0.82f, 0.82f, 1.00f };
            t.sceneSubheading     = { 0.68f, 0.68f, 0.30f, 1.00f };
            t.scenePreviewLabel   = { 0.50f, 0.82f, 0.50f, 1.00f };
            t.sceneWarning        = { 0.92f, 0.48f, 0.05f, 1.00f };

            t.layerCardSelected   = IM_COL32( 28, 28, 28, 255);
            t.layerCardDefault    = IM_COL32( 10, 10, 10, 255);
            t.layerFrameBg        = { 0.04f, 0.04f, 0.04f, 0.95f };
            t.layerCountActive    = { 0.45f, 0.85f, 0.55f, 1.00f };
            t.layerCountEmpty     = { 0.38f, 0.38f, 0.38f, 1.00f };

            t.prefabWarning       = { 0.95f, 0.78f, 0.20f, 1.00f };
            t.prefabNoChanges     = { 0.40f, 0.40f, 0.40f, 1.00f };
            t.prefabInstances     = { 0.88f, 0.88f, 0.88f, 1.00f };

            t.vpTextMuted         = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.vpSelectFill        = IM_COL32(200, 200, 200,  20);
            t.vpSelectBorder      = IM_COL32(200, 200, 200, 220);
            t.vpSelectHandle      = IM_COL32(230, 230, 230, 255);

            t.animEntityName      = { 0.75f, 0.75f, 0.75f, 1.00f };
            t.animError           = { 0.95f, 0.30f, 0.30f, 1.00f };
            t.animControls        = { 0.65f, 0.65f, 0.65f, 1.00f };
            t.animMuted           = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.animPlaying         = { 0.30f, 0.90f, 0.45f, 1.00f };
            t.animPaused          = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.animTimelineBg      = IM_COL32(  4,   4,  4, 255);
            t.animTimelineGrid    = IM_COL32( 30,  30, 30, 255);
            t.animTimelineTick    = IM_COL32( 70,  70, 70, 255);
            t.animTimelineLabel   = IM_COL32(130, 130, 130, 255);
            t.animKeyframe        = IM_COL32(220, 180,   0, 255);

            t.entityWarning       = { 0.92f, 0.58f, 0.08f, 1.00f };
            t.entityDragHint      = { 0.45f, 0.45f, 0.45f, 1.00f };
            t.entityPrefabTint    = { 0.95f, 0.85f, 0.28f, 1.00f };
            t.entityTemplateTint  = { 0.50f, 0.72f, 1.00f, 1.00f };
            t.entityNormalTint    = { 0.75f, 0.88f, 1.00f, 1.00f };

            return t;
        }

        // ============================================================
        //  Purple — deep violet / amethyst theme
        // ============================================================
        inline EditorTheme makeThemePurple() {
            EditorTheme t;
            t.name = "Purple";

            t.textDefault         = { 0.95f, 0.90f, 1.00f, 1.00f };
            t.textMuted           = { 0.65f, 0.55f, 0.80f, 1.00f };
            t.textDisabled        = { 0.44f, 0.36f, 0.58f, 1.00f };
            t.textWarning         = { 1.00f, 0.78f, 0.25f, 1.00f };
            t.textError           = { 1.00f, 0.35f, 0.50f, 1.00f };
            t.textSuccess         = { 0.38f, 1.00f, 0.62f, 1.00f };
            t.textInfo            = { 0.75f, 0.62f, 1.00f, 1.00f };
            t.textHighlight       = { 0.85f, 0.72f, 1.00f, 1.00f };
            t.textAccent          = { 0.70f, 0.50f, 1.00f, 1.00f };
            t.textSceneName       = { 0.65f, 0.55f, 0.80f, 1.00f };

            t.logTrace            = { 0.65f, 0.55f, 0.80f, 1.00f };
            t.logInfo             = { 0.38f, 1.00f, 0.62f, 1.00f };
            t.logWarn             = { 1.00f, 0.78f, 0.25f, 1.00f };
            t.logError            = { 1.00f, 0.35f, 0.50f, 1.00f };
            t.logCritical         = { 1.00f, 0.12f, 0.55f, 1.00f };

            t.historyUndo         = { 0.38f, 1.00f, 0.62f, 1.00f };
            t.historyRedo         = { 0.75f, 0.62f, 1.00f, 1.00f };
            t.historyNext         = { 1.00f, 0.90f, 0.28f, 1.00f };

            t.btnDanger           = { 0.60f, 0.10f, 0.22f, 1.00f };
            t.btnDangerHovered    = { 0.85f, 0.18f, 0.35f, 1.00f };
            t.btnDangerActive     = { 0.45f, 0.05f, 0.14f, 1.00f };

            t.btnSuccess          = { 0.14f, 0.52f, 0.28f, 1.00f };
            t.btnSuccessHovered   = { 0.22f, 0.70f, 0.40f, 1.00f };
            t.btnSuccessActive    = { 0.08f, 0.38f, 0.20f, 1.00f };

            t.btnPrimary          = { 0.42f, 0.22f, 0.80f, 0.88f };
            t.btnPrimaryHovered   = { 0.58f, 0.35f, 0.95f, 1.00f };
            t.btnPrimaryActive    = { 0.30f, 0.14f, 0.65f, 1.00f };

            t.btnSecondary        = { 0.22f, 0.14f, 0.35f, 0.70f };
            t.btnSecondaryHovered = { 0.32f, 0.22f, 0.50f, 0.90f };
            t.btnSecondaryActive  = { 0.42f, 0.30f, 0.62f, 0.60f };

            t.btnAccent           = { 0.55f, 0.30f, 0.90f, 0.85f };
            t.btnAccentHovered    = { 0.70f, 0.48f, 1.00f, 1.00f };

            t.btnTertiary         = { 0.38f, 0.20f, 0.60f, 0.80f };
            t.btnTertiaryHovered  = { 0.52f, 0.30f, 0.78f, 1.00f };
            t.btnTertiaryActive   = { 0.28f, 0.12f, 0.48f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.35f, 0.22f, 0.52f, 0.50f };
            t.btnGhostActive      = { 0.45f, 0.30f, 0.65f, 0.50f };

            t.compHeaderBtn       = { 0.45f, 0.22f, 0.82f, 1.00f };
            t.compHeaderBtnHov    = { 0.60f, 0.38f, 0.95f, 1.00f };
            t.compHeaderBtnAct    = { 0.32f, 0.14f, 0.65f, 1.00f };

            t.sceneCardActive     = IM_COL32( 42, 22, 55, 255);
            t.sceneCardSelected   = IM_COL32( 38, 20, 62, 255);
            t.sceneCardDefault    = IM_COL32( 22, 14, 35, 255);
            t.sceneBorderActive   = IM_COL32(130,  70, 230, 255);
            t.sceneBorderSelected = IM_COL32(160, 100, 255, 255);
            t.sceneBorderDefault  = IM_COL32( 58,  38,  80, 255);
            t.sceneThumb          = IM_COL32( 14,  8, 22, 255);
            t.sceneThumbBorder    = IM_COL32( 65,  40, 95, 255);
            t.sceneThumbText      = IM_COL32(155, 120, 195, 255);

            t.sceneTextActive     = { 0.72f, 0.52f, 1.00f, 1.00f };
            t.sceneTextActiveHov  = { 0.62f, 0.42f, 0.90f, 1.00f };
            t.sceneTextInactive   = { 0.42f, 0.32f, 0.58f, 1.00f };
            t.sceneActiveLabel    = { 0.80f, 0.68f, 1.00f, 1.00f };
            t.sceneHeading        = { 0.92f, 0.86f, 1.00f, 1.00f };
            t.sceneSubheading     = { 0.80f, 0.72f, 0.50f, 1.00f };
            t.scenePreviewLabel   = { 0.55f, 1.00f, 0.72f, 1.00f };
            t.sceneWarning        = { 1.00f, 0.52f, 0.10f, 1.00f };

            t.layerCardSelected   = IM_COL32( 55, 30, 80, 255);
            t.layerCardDefault    = IM_COL32( 28, 16, 45, 255);
            t.layerFrameBg        = { 0.14f, 0.08f, 0.22f, 0.88f };
            t.layerCountActive    = { 0.80f, 0.65f, 1.00f, 1.00f };
            t.layerCountEmpty     = { 0.45f, 0.35f, 0.60f, 1.00f };

            t.prefabWarning       = { 1.00f, 0.78f, 0.25f, 1.00f };
            t.prefabNoChanges     = { 0.48f, 0.38f, 0.62f, 1.00f };
            t.prefabInstances     = { 0.95f, 0.90f, 1.00f, 1.00f };

            t.vpTextMuted         = { 0.62f, 0.52f, 0.78f, 1.00f };
            t.vpSelectFill        = IM_COL32(140,  80, 255,  30);
            t.vpSelectBorder      = IM_COL32(160, 100, 255, 255);
            t.vpSelectHandle      = IM_COL32(190, 140, 255, 255);

            t.animEntityName      = { 0.80f, 0.68f, 1.00f, 1.00f };
            t.animError           = { 1.00f, 0.35f, 0.50f, 1.00f };
            t.animControls        = { 0.72f, 0.52f, 1.00f, 1.00f };
            t.animMuted           = { 0.62f, 0.52f, 0.78f, 1.00f };
            t.animPlaying         = { 0.38f, 1.00f, 0.62f, 1.00f };
            t.animPaused          = { 0.62f, 0.52f, 0.78f, 1.00f };
            t.animTimelineBg      = IM_COL32( 14,  8, 22, 255);
            t.animTimelineGrid    = IM_COL32( 48, 30, 70, 255);
            t.animTimelineTick    = IM_COL32( 95, 62, 138, 255);
            t.animTimelineLabel   = IM_COL32(165, 135, 210, 255);
            t.animKeyframe        = IM_COL32(180, 100, 255, 255);

            t.entityWarning       = { 1.00f, 0.62f, 0.12f, 1.00f };
            t.entityDragHint      = { 0.58f, 0.46f, 0.75f, 1.00f };
            t.entityPrefabTint    = { 1.00f, 0.82f, 0.38f, 1.00f };
            t.entityTemplateTint  = { 0.72f, 0.52f, 1.00f, 1.00f };
            t.entityNormalTint    = { 0.85f, 0.75f, 1.00f, 1.00f };

            return t;
        }

        // ============================================================
        //  White — clean, high-contrast light theme
        // ============================================================
        inline EditorTheme makeThemeWhite() {
            EditorTheme t;
            t.name = "White";

            // Text
            t.textDefault         = { 0.05f, 0.05f, 0.05f, 1.00f };
            t.textMuted           = { 0.35f, 0.35f, 0.35f, 1.00f };
            t.textDisabled        = { 0.58f, 0.58f, 0.58f, 1.00f };
            t.textWarning         = { 0.70f, 0.42f, 0.00f, 1.00f };
            t.textError           = { 0.78f, 0.08f, 0.08f, 1.00f };
            t.textSuccess         = { 0.06f, 0.58f, 0.14f, 1.00f };
            t.textInfo            = { 0.08f, 0.38f, 0.75f, 1.00f };
            t.textHighlight       = { 0.08f, 0.38f, 0.75f, 1.00f };
            t.textAccent          = { 0.05f, 0.32f, 0.68f, 1.00f };
            t.textSceneName       = { 0.25f, 0.25f, 0.25f, 1.00f };

            t.logTrace            = { 0.42f, 0.42f, 0.42f, 1.00f };
            t.logInfo             = { 0.06f, 0.58f, 0.14f, 1.00f };
            t.logWarn             = { 0.70f, 0.42f, 0.00f, 1.00f };
            t.logError            = { 0.78f, 0.08f, 0.08f, 1.00f };
            t.logCritical         = { 0.65f, 0.00f, 0.28f, 1.00f };

            t.historyUndo         = { 0.06f, 0.55f, 0.14f, 1.00f };
            t.historyRedo         = { 0.08f, 0.38f, 0.75f, 1.00f };
            t.historyNext         = { 0.62f, 0.52f, 0.00f, 1.00f };

            t.btnDanger           = { 0.80f, 0.12f, 0.12f, 1.00f };
            t.btnDangerHovered    = { 0.95f, 0.22f, 0.22f, 1.00f };
            t.btnDangerActive     = { 0.62f, 0.06f, 0.06f, 1.00f };

            t.btnSuccess          = { 0.10f, 0.62f, 0.18f, 1.00f };
            t.btnSuccessHovered   = { 0.15f, 0.78f, 0.24f, 1.00f };
            t.btnSuccessActive    = { 0.06f, 0.48f, 0.12f, 1.00f };

            t.btnPrimary          = { 0.12f, 0.48f, 0.88f, 0.92f };
            t.btnPrimaryHovered   = { 0.20f, 0.60f, 1.00f, 1.00f };
            t.btnPrimaryActive    = { 0.08f, 0.36f, 0.72f, 1.00f };

            t.btnSecondary        = { 0.82f, 0.82f, 0.82f, 1.00f };
            t.btnSecondaryHovered = { 0.92f, 0.92f, 0.92f, 1.00f };
            t.btnSecondaryActive  = { 0.70f, 0.70f, 0.70f, 1.00f };

            t.btnAccent           = { 0.10f, 0.50f, 0.90f, 0.90f };
            t.btnAccentHovered    = { 0.18f, 0.65f, 1.00f, 1.00f };

            t.btnTertiary         = { 0.55f, 0.32f, 0.14f, 0.85f };
            t.btnTertiaryHovered  = { 0.72f, 0.44f, 0.20f, 1.00f };
            t.btnTertiaryActive   = { 0.42f, 0.22f, 0.08f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.70f, 0.70f, 0.70f, 0.45f };
            t.btnGhostActive      = { 0.60f, 0.60f, 0.60f, 0.45f };

            t.compHeaderBtn       = { 0.12f, 0.48f, 0.88f, 1.00f };
            t.compHeaderBtnHov    = { 0.20f, 0.60f, 1.00f, 1.00f };
            t.compHeaderBtnAct    = { 0.08f, 0.36f, 0.72f, 1.00f };

            t.sceneCardActive     = IM_COL32(195, 235, 210, 255);
            t.sceneCardSelected   = IM_COL32(205, 220, 245, 255);
            t.sceneCardDefault    = IM_COL32(248, 248, 250, 255);
            t.sceneBorderActive   = IM_COL32( 30, 150,  70, 255);
            t.sceneBorderSelected = IM_COL32( 50, 100, 200, 255);
            t.sceneBorderDefault  = IM_COL32(195, 195, 200, 255);
            t.sceneThumb          = IM_COL32(250, 250, 252, 255);
            t.sceneThumbBorder    = IM_COL32(170, 170, 180, 255);
            t.sceneThumbText      = IM_COL32( 70,  70,  80, 255);

            t.sceneTextActive     = { 0.06f, 0.58f, 0.20f, 1.00f };
            t.sceneTextActiveHov  = { 0.05f, 0.48f, 0.16f, 1.00f };
            t.sceneTextInactive   = { 0.38f, 0.38f, 0.42f, 1.00f };
            t.sceneActiveLabel    = { 0.08f, 0.45f, 0.72f, 1.00f };
            t.sceneHeading        = { 0.08f, 0.08f, 0.15f, 1.00f };
            t.sceneSubheading     = { 0.30f, 0.30f, 0.04f, 1.00f };
            t.scenePreviewLabel   = { 0.06f, 0.50f, 0.10f, 1.00f };
            t.sceneWarning        = { 0.72f, 0.36f, 0.00f, 1.00f };

            t.layerCardSelected   = IM_COL32(198, 215, 240, 255);
            t.layerCardDefault    = IM_COL32(225, 228, 232, 255);
            t.layerFrameBg        = { 0.90f, 0.92f, 0.96f, 0.92f };
            t.layerCountActive    = { 0.08f, 0.55f, 0.16f, 1.00f };
            t.layerCountEmpty     = { 0.48f, 0.48f, 0.48f, 1.00f };

            t.prefabWarning       = { 0.72f, 0.44f, 0.00f, 1.00f };
            t.prefabNoChanges     = { 0.42f, 0.42f, 0.42f, 1.00f };
            t.prefabInstances     = { 0.05f, 0.05f, 0.05f, 1.00f };

            t.vpTextMuted         = { 0.32f, 0.32f, 0.32f, 1.00f };
            t.vpSelectFill        = IM_COL32( 20,  80, 200,  35);
            t.vpSelectBorder      = IM_COL32( 20,  80, 200, 255);
            t.vpSelectHandle      = IM_COL32( 10,  60, 175, 255);

            t.animEntityName      = { 0.08f, 0.40f, 0.78f, 1.00f };
            t.animError           = { 0.78f, 0.08f, 0.08f, 1.00f };
            t.animControls        = { 0.08f, 0.52f, 0.82f, 1.00f };
            t.animMuted           = { 0.40f, 0.40f, 0.40f, 1.00f };
            t.animPlaying         = { 0.06f, 0.65f, 0.16f, 1.00f };
            t.animPaused          = { 0.40f, 0.40f, 0.40f, 1.00f };
            t.animTimelineBg      = IM_COL32(245, 245, 248, 255);
            t.animTimelineGrid    = IM_COL32(175, 175, 185, 255);
            t.animTimelineTick    = IM_COL32(110, 110, 120, 255);
            t.animTimelineLabel   = IM_COL32( 55,  55,  65, 255);
            t.animKeyframe        = IM_COL32(180, 120,   0, 255);

            t.entityWarning       = { 0.70f, 0.38f, 0.00f, 1.00f };
            t.entityDragHint      = { 0.40f, 0.40f, 0.40f, 1.00f };
            t.entityPrefabTint    = { 0.68f, 0.55f, 0.04f, 1.00f };
            t.entityTemplateTint  = { 0.14f, 0.46f, 0.82f, 1.00f };
            t.entityNormalTint    = { 0.22f, 0.48f, 0.75f, 1.00f };

            return t;
        }

        // ============================================================
        //  Nord — arctic, north-bluish color palette
        // ============================================================
        inline EditorTheme makeThemeNord() {
            EditorTheme t;
            t.name = "Nord";

            t.textDefault         = { 0.85f, 0.87f, 0.91f, 1.00f }; // nord4
            t.textMuted           = { 0.56f, 0.62f, 0.71f, 1.00f }; // nord3
            t.textDisabled        = { 0.38f, 0.44f, 0.52f, 1.00f };
            t.textWarning         = { 0.92f, 0.80f, 0.55f, 1.00f }; // nord13
            t.textError           = { 0.75f, 0.38f, 0.42f, 1.00f }; // nord11
            t.textSuccess         = { 0.64f, 0.75f, 0.55f, 1.00f }; // nord14
            t.textInfo            = { 0.53f, 0.75f, 0.82f, 1.00f }; // nord8
            t.textHighlight       = { 0.50f, 0.70f, 0.90f, 1.00f }; // nord9
            t.textAccent          = { 0.36f, 0.60f, 0.80f, 1.00f }; // nord10
            t.textSceneName       = { 0.56f, 0.62f, 0.71f, 1.00f };

            t.logTrace            = { 0.56f, 0.62f, 0.71f, 1.00f };
            t.logInfo             = { 0.64f, 0.75f, 0.55f, 1.00f };
            t.logWarn             = { 0.92f, 0.80f, 0.55f, 1.00f };
            t.logError            = { 0.75f, 0.38f, 0.42f, 1.00f };
            t.logCritical         = { 0.82f, 0.52f, 0.60f, 1.00f };

            t.historyUndo         = { 0.64f, 0.75f, 0.55f, 1.00f };
            t.historyRedo         = { 0.53f, 0.75f, 0.82f, 1.00f };
            t.historyNext         = { 0.92f, 0.80f, 0.55f, 1.00f };

            t.btnDanger           = { 0.60f, 0.25f, 0.28f, 1.00f };
            t.btnDangerHovered    = { 0.78f, 0.35f, 0.38f, 1.00f };
            t.btnDangerActive     = { 0.45f, 0.16f, 0.18f, 1.00f };

            t.btnSuccess          = { 0.40f, 0.55f, 0.32f, 1.00f };
            t.btnSuccessHovered   = { 0.52f, 0.70f, 0.42f, 1.00f };
            t.btnSuccessActive    = { 0.30f, 0.42f, 0.22f, 1.00f };

            t.btnPrimary          = { 0.24f, 0.42f, 0.62f, 0.90f };
            t.btnPrimaryHovered   = { 0.34f, 0.54f, 0.75f, 1.00f };
            t.btnPrimaryActive    = { 0.16f, 0.32f, 0.50f, 1.00f };

            t.btnSecondary        = { 0.18f, 0.22f, 0.28f, 0.80f };
            t.btnSecondaryHovered = { 0.26f, 0.30f, 0.38f, 1.00f };
            t.btnSecondaryActive  = { 0.32f, 0.38f, 0.46f, 0.60f };

            t.btnAccent           = { 0.30f, 0.52f, 0.72f, 0.85f };
            t.btnAccentHovered    = { 0.42f, 0.65f, 0.85f, 1.00f };

            t.btnTertiary         = { 0.36f, 0.42f, 0.28f, 0.80f };
            t.btnTertiaryHovered  = { 0.48f, 0.56f, 0.38f, 1.00f };
            t.btnTertiaryActive   = { 0.26f, 0.32f, 0.18f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.26f, 0.32f, 0.40f, 0.50f };
            t.btnGhostActive      = { 0.34f, 0.40f, 0.50f, 0.50f };

            t.compHeaderBtn       = { 0.24f, 0.42f, 0.62f, 1.00f };
            t.compHeaderBtnHov    = { 0.34f, 0.54f, 0.75f, 1.00f };
            t.compHeaderBtnAct    = { 0.16f, 0.32f, 0.50f, 1.00f };

            t.sceneCardActive     = IM_COL32( 38, 58, 48, 255);
            t.sceneCardSelected   = IM_COL32( 38, 50, 68, 255);
            t.sceneCardDefault    = IM_COL32( 36, 42, 52, 255); // nord1
            t.sceneBorderActive   = IM_COL32(163, 190, 140, 255); // nord14
            t.sceneBorderSelected = IM_COL32(136, 192, 208, 255); // nord8
            t.sceneBorderDefault  = IM_COL32( 67,  76,  94, 255); // nord3
            t.sceneThumb          = IM_COL32( 29, 35, 44, 255); // nord0
            t.sceneThumbBorder    = IM_COL32( 59,  66,  82, 255);
            t.sceneThumbText      = IM_COL32(144, 155, 173, 255);

            t.sceneTextActive     = { 0.64f, 0.75f, 0.55f, 1.00f };
            t.sceneTextActiveHov  = { 0.54f, 0.65f, 0.45f, 1.00f };
            t.sceneTextInactive   = { 0.44f, 0.50f, 0.60f, 1.00f };
            t.sceneActiveLabel    = { 0.53f, 0.75f, 0.82f, 1.00f };
            t.sceneHeading        = { 0.82f, 0.84f, 0.88f, 1.00f };
            t.sceneSubheading     = { 0.75f, 0.72f, 0.50f, 1.00f };
            t.scenePreviewLabel   = { 0.60f, 0.80f, 0.65f, 1.00f };
            t.sceneWarning        = { 0.92f, 0.56f, 0.12f, 1.00f };

            t.layerCardSelected   = IM_COL32( 52, 62, 80, 255);
            t.layerCardDefault    = IM_COL32( 36, 42, 52, 255);
            t.layerFrameBg        = { 0.18f, 0.22f, 0.28f, 0.88f };
            t.layerCountActive    = { 0.64f, 0.78f, 0.55f, 1.00f };
            t.layerCountEmpty     = { 0.44f, 0.50f, 0.58f, 1.00f };

            t.prefabWarning       = { 0.92f, 0.80f, 0.55f, 1.00f };
            t.prefabNoChanges     = { 0.44f, 0.50f, 0.58f, 1.00f };
            t.prefabInstances     = { 0.85f, 0.87f, 0.91f, 1.00f };

            t.vpTextMuted         = { 0.56f, 0.62f, 0.71f, 1.00f };
            t.vpSelectFill        = IM_COL32(136, 192, 208,  28);
            t.vpSelectBorder      = IM_COL32(136, 192, 208, 255);
            t.vpSelectHandle      = IM_COL32(129, 161, 193, 255);

            t.animEntityName      = { 0.53f, 0.75f, 0.82f, 1.00f };
            t.animError           = { 0.75f, 0.38f, 0.42f, 1.00f };
            t.animControls        = { 0.50f, 0.70f, 0.90f, 1.00f };
            t.animMuted           = { 0.56f, 0.62f, 0.71f, 1.00f };
            t.animPlaying         = { 0.64f, 0.75f, 0.55f, 1.00f };
            t.animPaused          = { 0.56f, 0.62f, 0.71f, 1.00f };
            t.animTimelineBg      = IM_COL32( 29, 35, 44, 255);
            t.animTimelineGrid    = IM_COL32( 59, 66, 82, 255);
            t.animTimelineTick    = IM_COL32(100, 112, 132, 255);
            t.animTimelineLabel   = IM_COL32(160, 168, 180, 255);
            t.animKeyframe        = IM_COL32(235, 203, 139, 255); // nord13

            t.entityWarning       = { 0.92f, 0.60f, 0.12f, 1.00f };
            t.entityDragHint      = { 0.52f, 0.58f, 0.68f, 1.00f };
            t.entityPrefabTint    = { 0.92f, 0.80f, 0.55f, 1.00f };
            t.entityTemplateTint  = { 0.53f, 0.75f, 0.82f, 1.00f };
            t.entityNormalTint    = { 0.78f, 0.82f, 0.88f, 1.00f };

            return t;
        }

        // ============================================================
        //  Monokai — classic dark editor theme
        // ============================================================
        inline EditorTheme makeThemeMonokai() {
            EditorTheme t;
            t.name = "Monokai";

            t.textDefault         = { 0.97f, 0.97f, 0.95f, 1.00f }; // #f8f8f2
            t.textMuted           = { 0.62f, 0.61f, 0.52f, 1.00f }; // #75715e (comment)
            t.textDisabled        = { 0.45f, 0.44f, 0.38f, 1.00f };
            t.textWarning         = { 0.98f, 0.82f, 0.20f, 1.00f }; // #f9d029
            t.textError           = { 0.98f, 0.15f, 0.45f, 1.00f }; // #f92672
            t.textSuccess         = { 0.65f, 0.89f, 0.18f, 1.00f }; // #a6e22e
            t.textInfo            = { 0.40f, 0.85f, 1.00f, 1.00f }; // #66d9e8
            t.textHighlight       = { 0.98f, 0.60f, 0.22f, 1.00f }; // #fd971f
            t.textAccent          = { 0.67f, 0.53f, 1.00f, 1.00f }; // #ae81ff
            t.textSceneName       = { 0.62f, 0.61f, 0.52f, 1.00f };

            t.logTrace            = { 0.62f, 0.61f, 0.52f, 1.00f };
            t.logInfo             = { 0.65f, 0.89f, 0.18f, 1.00f };
            t.logWarn             = { 0.98f, 0.82f, 0.20f, 1.00f };
            t.logError            = { 0.98f, 0.15f, 0.45f, 1.00f };
            t.logCritical         = { 0.98f, 0.08f, 0.30f, 1.00f };

            t.historyUndo         = { 0.65f, 0.89f, 0.18f, 1.00f };
            t.historyRedo         = { 0.40f, 0.85f, 1.00f, 1.00f };
            t.historyNext         = { 0.98f, 0.82f, 0.20f, 1.00f };

            t.btnDanger           = { 0.65f, 0.06f, 0.22f, 1.00f };
            t.btnDangerHovered    = { 0.88f, 0.10f, 0.35f, 1.00f };
            t.btnDangerActive     = { 0.48f, 0.04f, 0.14f, 1.00f };

            t.btnSuccess          = { 0.38f, 0.58f, 0.08f, 1.00f };
            t.btnSuccessHovered   = { 0.52f, 0.75f, 0.12f, 1.00f };
            t.btnSuccessActive    = { 0.26f, 0.42f, 0.04f, 1.00f };

            t.btnPrimary          = { 0.38f, 0.32f, 0.62f, 0.90f };
            t.btnPrimaryHovered   = { 0.52f, 0.44f, 0.80f, 1.00f };
            t.btnPrimaryActive    = { 0.28f, 0.22f, 0.48f, 1.00f };

            t.btnSecondary        = { 0.20f, 0.18f, 0.15f, 0.80f };
            t.btnSecondaryHovered = { 0.30f, 0.28f, 0.24f, 1.00f };
            t.btnSecondaryActive  = { 0.38f, 0.36f, 0.30f, 0.60f };

            t.btnAccent           = { 0.55f, 0.40f, 0.88f, 0.85f };
            t.btnAccentHovered    = { 0.68f, 0.54f, 1.00f, 1.00f };

            t.btnTertiary         = { 0.52f, 0.38f, 0.10f, 0.80f };
            t.btnTertiaryHovered  = { 0.68f, 0.52f, 0.16f, 1.00f };
            t.btnTertiaryActive   = { 0.38f, 0.26f, 0.06f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.30f, 0.28f, 0.24f, 0.50f };
            t.btnGhostActive      = { 0.40f, 0.38f, 0.32f, 0.50f };

            t.compHeaderBtn       = { 0.40f, 0.55f, 0.10f, 1.00f };
            t.compHeaderBtnHov    = { 0.55f, 0.72f, 0.16f, 1.00f };
            t.compHeaderBtnAct    = { 0.28f, 0.40f, 0.06f, 1.00f };

            t.sceneCardActive     = IM_COL32( 45, 58, 20, 255);
            t.sceneCardSelected   = IM_COL32( 45, 40, 68, 255);
            t.sceneCardDefault    = IM_COL32( 30, 28, 24, 255); // #272822
            t.sceneBorderActive   = IM_COL32(166, 226,  46, 255); // green
            t.sceneBorderSelected = IM_COL32(102, 217, 232, 255); // cyan
            t.sceneBorderDefault  = IM_COL32( 62,  58,  50, 255);
            t.sceneThumb          = IM_COL32( 22, 20, 17, 255);
            t.sceneThumbBorder    = IM_COL32( 60, 56, 48, 255);
            t.sceneThumbText      = IM_COL32(158, 156, 132, 255);

            t.sceneTextActive     = { 0.65f, 0.89f, 0.18f, 1.00f };
            t.sceneTextActiveHov  = { 0.55f, 0.75f, 0.14f, 1.00f };
            t.sceneTextInactive   = { 0.46f, 0.44f, 0.36f, 1.00f };
            t.sceneActiveLabel    = { 0.40f, 0.85f, 1.00f, 1.00f };
            t.sceneHeading        = { 0.95f, 0.95f, 0.90f, 1.00f };
            t.sceneSubheading     = { 0.98f, 0.82f, 0.20f, 1.00f };
            t.scenePreviewLabel   = { 0.65f, 0.90f, 0.25f, 1.00f };
            t.sceneWarning        = { 0.98f, 0.60f, 0.22f, 1.00f };

            t.layerCardSelected   = IM_COL32( 55, 52, 44, 255);
            t.layerCardDefault    = IM_COL32( 32, 30, 26, 255);
            t.layerFrameBg        = { 0.16f, 0.15f, 0.12f, 0.90f };
            t.layerCountActive    = { 0.65f, 0.89f, 0.18f, 1.00f };
            t.layerCountEmpty     = { 0.48f, 0.46f, 0.38f, 1.00f };

            t.prefabWarning       = { 0.98f, 0.82f, 0.20f, 1.00f };
            t.prefabNoChanges     = { 0.50f, 0.48f, 0.40f, 1.00f };
            t.prefabInstances     = { 0.97f, 0.97f, 0.95f, 1.00f };

            t.vpTextMuted         = { 0.62f, 0.61f, 0.52f, 1.00f };
            t.vpSelectFill        = IM_COL32(102, 217, 232,  28);
            t.vpSelectBorder      = IM_COL32(102, 217, 232, 255);
            t.vpSelectHandle      = IM_COL32(166, 226,  46, 255);

            t.animEntityName      = { 0.40f, 0.85f, 1.00f, 1.00f };
            t.animError           = { 0.98f, 0.15f, 0.45f, 1.00f };
            t.animControls        = { 0.67f, 0.53f, 1.00f, 1.00f };
            t.animMuted           = { 0.62f, 0.61f, 0.52f, 1.00f };
            t.animPlaying         = { 0.65f, 0.89f, 0.18f, 1.00f };
            t.animPaused          = { 0.62f, 0.61f, 0.52f, 1.00f };
            t.animTimelineBg      = IM_COL32( 22, 20, 17, 255);
            t.animTimelineGrid    = IM_COL32( 55, 52, 44, 255);
            t.animTimelineTick    = IM_COL32(100,  96, 80, 255);
            t.animTimelineLabel   = IM_COL32(160, 157, 132, 255);
            t.animKeyframe        = IM_COL32(253, 151,  31, 255); // orange

            t.entityWarning       = { 0.98f, 0.60f, 0.22f, 1.00f };
            t.entityDragHint      = { 0.58f, 0.56f, 0.46f, 1.00f };
            t.entityPrefabTint    = { 0.98f, 0.82f, 0.20f, 1.00f };
            t.entityTemplateTint  = { 0.40f, 0.85f, 1.00f, 1.00f };
            t.entityNormalTint    = { 0.65f, 0.89f, 0.18f, 1.00f };

            return t;
        }

        // ============================================================
        //  Solarized — Ethan Schoonover's precision colours
        // ============================================================
        inline EditorTheme makeThemeSolarized() {
            EditorTheme t;
            t.name = "Solarized";

            // Solarized dark base palette
            // base03=#002b36 base02=#073642 base01=#586e75 base00=#657b83
            // base0=#839496  base1=#93a1a1  base2=#eee8d5  base3=#fdf6e3
            // yellow=#b58900 orange=#cb4b16 red=#dc322f magenta=#d33682
            // violet=#6c71c4 blue=#268bd2   cyan=#2aa198  green=#859900

            t.textDefault         = { 0.51f, 0.58f, 0.59f, 1.00f }; // base0
            t.textMuted           = { 0.35f, 0.43f, 0.46f, 1.00f }; // base01
            t.textDisabled        = { 0.26f, 0.32f, 0.35f, 1.00f };
            t.textWarning         = { 0.71f, 0.54f, 0.00f, 1.00f }; // yellow
            t.textError           = { 0.86f, 0.20f, 0.18f, 1.00f }; // red
            t.textSuccess         = { 0.52f, 0.60f, 0.00f, 1.00f }; // green
            t.textInfo            = { 0.16f, 0.55f, 0.82f, 1.00f }; // blue
            t.textHighlight       = { 0.42f, 0.44f, 0.77f, 1.00f }; // violet
            t.textAccent          = { 0.16f, 0.63f, 0.60f, 1.00f }; // cyan
            t.textSceneName       = { 0.35f, 0.43f, 0.46f, 1.00f };

            t.logTrace            = { 0.35f, 0.43f, 0.46f, 1.00f };
            t.logInfo             = { 0.52f, 0.60f, 0.00f, 1.00f };
            t.logWarn             = { 0.71f, 0.54f, 0.00f, 1.00f };
            t.logError            = { 0.86f, 0.20f, 0.18f, 1.00f };
            t.logCritical         = { 0.83f, 0.21f, 0.51f, 1.00f }; // magenta

            t.historyUndo         = { 0.52f, 0.60f, 0.00f, 1.00f };
            t.historyRedo         = { 0.16f, 0.55f, 0.82f, 1.00f };
            t.historyNext         = { 0.71f, 0.54f, 0.00f, 1.00f };

            t.btnDanger           = { 0.60f, 0.12f, 0.10f, 1.00f };
            t.btnDangerHovered    = { 0.82f, 0.18f, 0.16f, 1.00f };
            t.btnDangerActive     = { 0.44f, 0.06f, 0.05f, 1.00f };

            t.btnSuccess          = { 0.28f, 0.36f, 0.00f, 1.00f };
            t.btnSuccessHovered   = { 0.40f, 0.50f, 0.00f, 1.00f };
            t.btnSuccessActive    = { 0.18f, 0.24f, 0.00f, 1.00f };

            t.btnPrimary          = { 0.10f, 0.36f, 0.58f, 0.90f };
            t.btnPrimaryHovered   = { 0.16f, 0.50f, 0.76f, 1.00f };
            t.btnPrimaryActive    = { 0.06f, 0.26f, 0.44f, 1.00f };

            t.btnSecondary        = { 0.02f, 0.22f, 0.28f, 0.80f };
            t.btnSecondaryHovered = { 0.04f, 0.30f, 0.38f, 1.00f };
            t.btnSecondaryActive  = { 0.08f, 0.36f, 0.46f, 0.60f };

            t.btnAccent           = { 0.10f, 0.40f, 0.55f, 0.85f };
            t.btnAccentHovered    = { 0.16f, 0.55f, 0.72f, 1.00f };

            t.btnTertiary         = { 0.44f, 0.28f, 0.00f, 0.80f };
            t.btnTertiaryHovered  = { 0.60f, 0.40f, 0.00f, 1.00f };
            t.btnTertiaryActive   = { 0.30f, 0.18f, 0.00f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.08f, 0.30f, 0.38f, 0.50f };
            t.btnGhostActive      = { 0.12f, 0.38f, 0.48f, 0.50f };

            t.compHeaderBtn       = { 0.10f, 0.38f, 0.60f, 1.00f };
            t.compHeaderBtnHov    = { 0.16f, 0.52f, 0.78f, 1.00f };
            t.compHeaderBtnAct    = { 0.06f, 0.26f, 0.44f, 1.00f };

            t.sceneCardActive     = IM_COL32( 15, 55, 40, 255);
            t.sceneCardSelected   = IM_COL32( 10, 48, 68, 255);
            t.sceneCardDefault    = IM_COL32(  7, 54, 66, 255); // base02
            t.sceneBorderActive   = IM_COL32(133, 153,   0, 255); // green
            t.sceneBorderSelected = IM_COL32( 38, 139, 210, 255); // blue
            t.sceneBorderDefault  = IM_COL32( 42,  65,  75, 255);
            t.sceneThumb          = IM_COL32(  0, 43, 54, 255); // base03
            t.sceneThumbBorder    = IM_COL32( 42,  65,  75, 255);
            t.sceneThumbText      = IM_COL32(101, 123, 131, 255); // base00

            t.sceneTextActive     = { 0.52f, 0.60f, 0.00f, 1.00f };
            t.sceneTextActiveHov  = { 0.42f, 0.50f, 0.00f, 1.00f };
            t.sceneTextInactive   = { 0.26f, 0.34f, 0.38f, 1.00f };
            t.sceneActiveLabel    = { 0.16f, 0.55f, 0.82f, 1.00f };
            t.sceneHeading        = { 0.58f, 0.63f, 0.63f, 1.00f }; // base1
            t.sceneSubheading     = { 0.60f, 0.52f, 0.00f, 1.00f };
            t.scenePreviewLabel   = { 0.52f, 0.62f, 0.10f, 1.00f };
            t.sceneWarning        = { 0.80f, 0.29f, 0.09f, 1.00f }; // orange

            t.layerCardSelected   = IM_COL32( 18, 68, 82, 255);
            t.layerCardDefault    = IM_COL32(  7, 54, 66, 255);
            t.layerFrameBg        = { 0.02f, 0.22f, 0.28f, 0.90f };
            t.layerCountActive    = { 0.52f, 0.60f, 0.00f, 1.00f };
            t.layerCountEmpty     = { 0.30f, 0.38f, 0.42f, 1.00f };

            t.prefabWarning       = { 0.71f, 0.54f, 0.00f, 1.00f };
            t.prefabNoChanges     = { 0.30f, 0.38f, 0.42f, 1.00f };
            t.prefabInstances     = { 0.51f, 0.58f, 0.59f, 1.00f };

            t.vpTextMuted         = { 0.35f, 0.43f, 0.46f, 1.00f };
            t.vpSelectFill        = IM_COL32( 38, 139, 210,  28);
            t.vpSelectBorder      = IM_COL32( 38, 139, 210, 255);
            t.vpSelectHandle      = IM_COL32( 42, 161, 152, 255); // cyan

            t.animEntityName      = { 0.16f, 0.55f, 0.82f, 1.00f };
            t.animError           = { 0.86f, 0.20f, 0.18f, 1.00f };
            t.animControls        = { 0.16f, 0.63f, 0.60f, 1.00f };
            t.animMuted           = { 0.35f, 0.43f, 0.46f, 1.00f };
            t.animPlaying         = { 0.52f, 0.60f, 0.00f, 1.00f };
            t.animPaused          = { 0.35f, 0.43f, 0.46f, 1.00f };
            t.animTimelineBg      = IM_COL32(  0, 43, 54, 255);
            t.animTimelineGrid    = IM_COL32( 42, 65, 75, 255);
            t.animTimelineTick    = IM_COL32( 72, 95, 106, 255);
            t.animTimelineLabel   = IM_COL32(101, 123, 131, 255);
            t.animKeyframe        = IM_COL32(181, 137,   0, 255); // yellow

            t.entityWarning       = { 0.80f, 0.29f, 0.09f, 1.00f };
            t.entityDragHint      = { 0.32f, 0.40f, 0.44f, 1.00f };
            t.entityPrefabTint    = { 0.71f, 0.54f, 0.00f, 1.00f };
            t.entityTemplateTint  = { 0.16f, 0.55f, 0.82f, 1.00f };
            t.entityNormalTint    = { 0.51f, 0.58f, 0.59f, 1.00f };

            return t;
        }

        // ============================================================
        //  Cyberpunk — neon on near-black
        // ============================================================
        inline EditorTheme makeThemeCyberpunk() {
            EditorTheme t;
            t.name = "Cyberpunk";

            t.textDefault         = { 0.95f, 0.95f, 1.00f, 1.00f };
            t.textMuted           = { 0.55f, 0.50f, 0.70f, 1.00f };
            t.textDisabled        = { 0.35f, 0.32f, 0.48f, 1.00f };
            t.textWarning         = { 1.00f, 0.90f, 0.00f, 1.00f }; // neon yellow
            t.textError           = { 1.00f, 0.08f, 0.55f, 1.00f }; // hot pink
            t.textSuccess         = { 0.00f, 1.00f, 0.60f, 1.00f }; // neon green
            t.textInfo            = { 0.00f, 0.90f, 1.00f, 1.00f }; // cyan
            t.textHighlight       = { 1.00f, 0.00f, 0.75f, 1.00f }; // magenta
            t.textAccent          = { 0.80f, 0.00f, 1.00f, 1.00f }; // purple neon
            t.textSceneName       = { 0.55f, 0.50f, 0.70f, 1.00f };

            t.logTrace            = { 0.45f, 0.40f, 0.60f, 1.00f };
            t.logInfo             = { 0.00f, 1.00f, 0.60f, 1.00f };
            t.logWarn             = { 1.00f, 0.90f, 0.00f, 1.00f };
            t.logError            = { 1.00f, 0.08f, 0.55f, 1.00f };
            t.logCritical         = { 1.00f, 0.00f, 0.30f, 1.00f };

            t.historyUndo         = { 0.00f, 1.00f, 0.60f, 1.00f };
            t.historyRedo         = { 0.00f, 0.90f, 1.00f, 1.00f };
            t.historyNext         = { 1.00f, 0.90f, 0.00f, 1.00f };

            t.btnDanger           = { 0.70f, 0.02f, 0.30f, 1.00f };
            t.btnDangerHovered    = { 1.00f, 0.05f, 0.50f, 1.00f };
            t.btnDangerActive     = { 0.50f, 0.01f, 0.20f, 1.00f };

            t.btnSuccess          = { 0.00f, 0.55f, 0.30f, 1.00f };
            t.btnSuccessHovered   = { 0.00f, 0.80f, 0.45f, 1.00f };
            t.btnSuccessActive    = { 0.00f, 0.38f, 0.20f, 1.00f };

            t.btnPrimary          = { 0.45f, 0.00f, 0.65f, 0.90f };
            t.btnPrimaryHovered   = { 0.65f, 0.00f, 0.90f, 1.00f };
            t.btnPrimaryActive    = { 0.30f, 0.00f, 0.48f, 1.00f };

            t.btnSecondary        = { 0.12f, 0.10f, 0.20f, 0.80f };
            t.btnSecondaryHovered = { 0.20f, 0.16f, 0.32f, 1.00f };
            t.btnSecondaryActive  = { 0.28f, 0.22f, 0.42f, 0.60f };

            t.btnAccent           = { 0.00f, 0.55f, 0.80f, 0.85f };
            t.btnAccentHovered    = { 0.00f, 0.80f, 1.00f, 1.00f };

            t.btnTertiary         = { 0.50f, 0.35f, 0.00f, 0.80f };
            t.btnTertiaryHovered  = { 0.75f, 0.55f, 0.00f, 1.00f };
            t.btnTertiaryActive   = { 0.35f, 0.22f, 0.00f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.25f, 0.18f, 0.40f, 0.50f };
            t.btnGhostActive      = { 0.35f, 0.25f, 0.52f, 0.50f };

            t.compHeaderBtn       = { 0.45f, 0.00f, 0.65f, 1.00f };
            t.compHeaderBtnHov    = { 0.65f, 0.00f, 0.90f, 1.00f };
            t.compHeaderBtnAct    = { 0.30f, 0.00f, 0.48f, 1.00f };

            // Component pills — neon overrides
            t.compTransform       = IM_COL32(255, 220,   0, 255); // neon yellow
            t.compRendering       = IM_COL32(  0, 200, 255, 255); // cyan
            t.compAnimation       = IM_COL32(180,   0, 255, 255); // purple neon
            t.compAudio           = IM_COL32(  0, 255, 180, 255); // neon green
            t.compAI              = IM_COL32(255,   0,  80, 255); // hot pink
            t.compScript          = IM_COL32(  0, 255, 100, 255);
            t.compUI              = IM_COL32(  0, 180, 255, 255);

            t.sceneCardActive     = IM_COL32( 10, 40, 30, 255);
            t.sceneCardSelected   = IM_COL32( 25, 10, 45, 255);
            t.sceneCardDefault    = IM_COL32(  8,  6, 16, 255);
            t.sceneBorderActive   = IM_COL32(  0, 255, 150, 255);
            t.sceneBorderSelected = IM_COL32(200,   0, 255, 255);
            t.sceneBorderDefault  = IM_COL32( 45,  35,  70, 255);
            t.sceneThumb          = IM_COL32(  4,  3,  8, 255);
            t.sceneThumbBorder    = IM_COL32( 55,  40,  90, 255);
            t.sceneThumbText      = IM_COL32(130, 110, 180, 255);

            t.sceneTextActive     = { 0.00f, 1.00f, 0.60f, 1.00f };
            t.sceneTextActiveHov  = { 0.00f, 0.85f, 0.50f, 1.00f };
            t.sceneTextInactive   = { 0.38f, 0.32f, 0.55f, 1.00f };
            t.sceneActiveLabel    = { 0.00f, 0.90f, 1.00f, 1.00f };
            t.sceneHeading        = { 0.92f, 0.92f, 1.00f, 1.00f };
            t.sceneSubheading     = { 1.00f, 0.90f, 0.00f, 1.00f };
            t.scenePreviewLabel   = { 0.00f, 1.00f, 0.65f, 1.00f };
            t.sceneWarning        = { 1.00f, 0.55f, 0.00f, 1.00f };

            t.layerCardSelected   = IM_COL32( 30, 20, 55, 255);
            t.layerCardDefault    = IM_COL32( 10,  8, 20, 255);
            t.layerFrameBg        = { 0.05f, 0.04f, 0.10f, 0.92f };
            t.layerCountActive    = { 0.00f, 1.00f, 0.65f, 1.00f };
            t.layerCountEmpty     = { 0.38f, 0.32f, 0.55f, 1.00f };

            t.prefabWarning       = { 1.00f, 0.90f, 0.00f, 1.00f };
            t.prefabNoChanges     = { 0.40f, 0.34f, 0.58f, 1.00f };
            t.prefabInstances     = { 0.92f, 0.92f, 1.00f, 1.00f };

            t.vpTextMuted         = { 0.50f, 0.44f, 0.65f, 1.00f };
            t.vpSelectFill        = IM_COL32(  0, 220, 255,  25);
            t.vpSelectBorder      = IM_COL32(  0, 220, 255, 255);
            t.vpSelectHandle      = IM_COL32(200,   0, 255, 255);

            t.animEntityName      = { 0.00f, 0.90f, 1.00f, 1.00f };
            t.animError           = { 1.00f, 0.08f, 0.55f, 1.00f };
            t.animControls        = { 0.80f, 0.00f, 1.00f, 1.00f };
            t.animMuted           = { 0.50f, 0.44f, 0.65f, 1.00f };
            t.animPlaying         = { 0.00f, 1.00f, 0.60f, 1.00f };
            t.animPaused          = { 0.50f, 0.44f, 0.65f, 1.00f };
            t.animTimelineBg      = IM_COL32(  4,  3,  8, 255);
            t.animTimelineGrid    = IM_COL32( 35, 28, 60, 255);
            t.animTimelineTick    = IM_COL32( 80, 60, 130, 255);
            t.animTimelineLabel   = IM_COL32(150, 130, 200, 255);
            t.animKeyframe        = IM_COL32(255, 220,   0, 255);

            t.entityWarning       = { 1.00f, 0.65f, 0.00f, 1.00f };
            t.entityDragHint      = { 0.50f, 0.44f, 0.68f, 1.00f };
            t.entityPrefabTint    = { 1.00f, 0.90f, 0.00f, 1.00f };
            t.entityTemplateTint  = { 0.00f, 0.90f, 1.00f, 1.00f };
            t.entityNormalTint    = { 0.80f, 0.70f, 1.00f, 1.00f };

            return t;
        }

        // ============================================================
        //  Forest — earthy green / nature theme
        // ============================================================
        inline EditorTheme makeThemeForest() {
            EditorTheme t;
            t.name = "Forest";

            t.textDefault         = { 0.88f, 0.90f, 0.82f, 1.00f };
            t.textMuted           = { 0.58f, 0.65f, 0.48f, 1.00f };
            t.textDisabled        = { 0.40f, 0.45f, 0.32f, 1.00f };
            t.textWarning         = { 0.95f, 0.78f, 0.18f, 1.00f };
            t.textError           = { 0.92f, 0.28f, 0.22f, 1.00f };
            t.textSuccess         = { 0.45f, 0.92f, 0.38f, 1.00f };
            t.textInfo            = { 0.52f, 0.85f, 0.65f, 1.00f };
            t.textHighlight       = { 0.68f, 0.90f, 0.50f, 1.00f };
            t.textAccent          = { 0.42f, 0.78f, 0.35f, 1.00f };
            t.textSceneName       = { 0.58f, 0.65f, 0.48f, 1.00f };

            t.logTrace            = { 0.50f, 0.58f, 0.40f, 1.00f };
            t.logInfo             = { 0.45f, 0.92f, 0.38f, 1.00f };
            t.logWarn             = { 0.95f, 0.78f, 0.18f, 1.00f };
            t.logError            = { 0.92f, 0.28f, 0.22f, 1.00f };
            t.logCritical         = { 0.88f, 0.12f, 0.30f, 1.00f };

            t.historyUndo         = { 0.45f, 0.92f, 0.38f, 1.00f };
            t.historyRedo         = { 0.52f, 0.85f, 0.65f, 1.00f };
            t.historyNext         = { 0.95f, 0.88f, 0.30f, 1.00f };

            t.btnDanger           = { 0.62f, 0.14f, 0.10f, 1.00f };
            t.btnDangerHovered    = { 0.82f, 0.22f, 0.16f, 1.00f };
            t.btnDangerActive     = { 0.45f, 0.08f, 0.06f, 1.00f };

            t.btnSuccess          = { 0.22f, 0.52f, 0.16f, 1.00f };
            t.btnSuccessHovered   = { 0.32f, 0.68f, 0.24f, 1.00f };
            t.btnSuccessActive    = { 0.14f, 0.38f, 0.10f, 1.00f };

            t.btnPrimary          = { 0.22f, 0.45f, 0.18f, 0.90f };
            t.btnPrimaryHovered   = { 0.32f, 0.60f, 0.26f, 1.00f };
            t.btnPrimaryActive    = { 0.14f, 0.32f, 0.12f, 1.00f };

            t.btnSecondary        = { 0.14f, 0.20f, 0.10f, 0.80f };
            t.btnSecondaryHovered = { 0.22f, 0.30f, 0.16f, 1.00f };
            t.btnSecondaryActive  = { 0.30f, 0.40f, 0.22f, 0.60f };

            t.btnAccent           = { 0.28f, 0.58f, 0.22f, 0.88f };
            t.btnAccentHovered    = { 0.40f, 0.75f, 0.32f, 1.00f };

            t.btnTertiary         = { 0.42f, 0.30f, 0.12f, 0.80f };
            t.btnTertiaryHovered  = { 0.58f, 0.42f, 0.18f, 1.00f };
            t.btnTertiaryActive   = { 0.30f, 0.20f, 0.06f, 1.00f };

            t.btnGhost            = { 0.00f, 0.00f, 0.00f, 0.00f };
            t.btnGhostHovered     = { 0.22f, 0.32f, 0.16f, 0.50f };
            t.btnGhostActive      = { 0.30f, 0.42f, 0.22f, 0.50f };

            t.compHeaderBtn       = { 0.25f, 0.50f, 0.18f, 1.00f };
            t.compHeaderBtnHov    = { 0.35f, 0.65f, 0.26f, 1.00f };
            t.compHeaderBtnAct    = { 0.16f, 0.36f, 0.12f, 1.00f };

            t.sceneCardActive     = IM_COL32( 28, 55, 20, 255);
            t.sceneCardSelected   = IM_COL32( 22, 48, 30, 255);
            t.sceneCardDefault    = IM_COL32( 18, 28, 14, 255);
            t.sceneBorderActive   = IM_COL32( 90, 200,  60, 255);
            t.sceneBorderSelected = IM_COL32( 70, 165,  90, 255);
            t.sceneBorderDefault  = IM_COL32( 45,  68,  32, 255);
            t.sceneThumb          = IM_COL32( 12, 20,  8, 255);
            t.sceneThumbBorder    = IM_COL32( 48,  72, 35, 255);
            t.sceneThumbText      = IM_COL32(130, 155,  90, 255);

            t.sceneTextActive     = { 0.48f, 0.90f, 0.35f, 1.00f };
            t.sceneTextActiveHov  = { 0.38f, 0.78f, 0.28f, 1.00f };
            t.sceneTextInactive   = { 0.38f, 0.48f, 0.28f, 1.00f };
            t.sceneActiveLabel    = { 0.52f, 0.85f, 0.65f, 1.00f };
            t.sceneHeading        = { 0.82f, 0.88f, 0.72f, 1.00f };
            t.sceneSubheading     = { 0.78f, 0.75f, 0.38f, 1.00f };
            t.scenePreviewLabel   = { 0.55f, 0.88f, 0.45f, 1.00f };
            t.sceneWarning        = { 0.92f, 0.52f, 0.08f, 1.00f };

            t.layerCardSelected   = IM_COL32( 35, 62, 24, 255);
            t.layerCardDefault    = IM_COL32( 18, 28, 12, 255);
            t.layerFrameBg        = { 0.10f, 0.16f, 0.06f, 0.90f };
            t.layerCountActive    = { 0.52f, 0.88f, 0.40f, 1.00f };
            t.layerCountEmpty     = { 0.40f, 0.52f, 0.30f, 1.00f };

            t.prefabWarning       = { 0.95f, 0.78f, 0.18f, 1.00f };
            t.prefabNoChanges     = { 0.42f, 0.55f, 0.30f, 1.00f };
            t.prefabInstances     = { 0.88f, 0.90f, 0.82f, 1.00f };

            t.vpTextMuted         = { 0.52f, 0.62f, 0.40f, 1.00f };
            t.vpSelectFill        = IM_COL32( 80, 200,  50,  28);
            t.vpSelectBorder      = IM_COL32( 90, 200,  55, 255);
            t.vpSelectHandle      = IM_COL32(120, 230,  80, 255);

            t.animEntityName      = { 0.52f, 0.85f, 0.65f, 1.00f };
            t.animError           = { 0.92f, 0.28f, 0.22f, 1.00f };
            t.animControls        = { 0.45f, 0.80f, 0.38f, 1.00f };
            t.animMuted           = { 0.52f, 0.62f, 0.40f, 1.00f };
            t.animPlaying         = { 0.45f, 0.92f, 0.38f, 1.00f };
            t.animPaused          = { 0.52f, 0.62f, 0.40f, 1.00f };
            t.animTimelineBg      = IM_COL32( 10, 18,  6, 255);
            t.animTimelineGrid    = IM_COL32( 35, 55, 22, 255);
            t.animTimelineTick    = IM_COL32( 65, 100, 42, 255);
            t.animTimelineLabel   = IM_COL32(120, 155,  80, 255);
            t.animKeyframe        = IM_COL32(160, 220,  60, 255);

            t.entityWarning       = { 0.92f, 0.62f, 0.10f, 1.00f };
            t.entityDragHint      = { 0.50f, 0.60f, 0.38f, 1.00f };
            t.entityPrefabTint    = { 0.95f, 0.85f, 0.32f, 1.00f };
            t.entityTemplateTint  = { 0.52f, 0.85f, 0.65f, 1.00f };
            t.entityNormalTint    = { 0.72f, 0.90f, 0.58f, 1.00f };

            return t;
        }

        // ============================================================
        //  ThemeManager — singleton
        // ============================================================
        class ThemeManager {
        public:
            static ThemeManager& get() {
                static ThemeManager instance;
                return instance;
            }

            // Returns the currently active theme
            const EditorTheme& theme() const { return m_theme; }

            // Returns the name of the active theme
            const std::string& activeName() const { return m_theme.name; }

            // Apply ImGui global style colours that map to theme tokens.
            // Call once after ImGui::StyleColorsDark() in EditorGLFW::init(),
            // and again whenever the theme is switched.
            void applyToImGui() {
                ImGuiStyle& style = ImGui::GetStyle();

                // Window / frame background
                style.Colors[ImGuiCol_WindowBg]          = { 0.13f, 0.14f, 0.17f, 1.00f };
                style.Colors[ImGuiCol_ChildBg]            = { 0.10f, 0.11f, 0.14f, 1.00f };
                style.Colors[ImGuiCol_PopupBg]            = { 0.12f, 0.13f, 0.16f, 0.95f };
                style.Colors[ImGuiCol_Border]             = { 0.28f, 0.30f, 0.38f, 0.60f };

                // Header (collapsing header, selectable)
                style.Colors[ImGuiCol_Header]             = { 0.20f, 0.40f, 0.80f, 0.40f };
                style.Colors[ImGuiCol_HeaderHovered]      = { 0.30f, 0.50f, 0.90f, 0.55f };
                style.Colors[ImGuiCol_HeaderActive]       = { 0.15f, 0.35f, 0.70f, 0.70f };

                // Frame (input, checkbox)
                style.Colors[ImGuiCol_FrameBg]            = { 0.18f, 0.20f, 0.25f, 1.00f };
                style.Colors[ImGuiCol_FrameBgHovered]     = { 0.24f, 0.27f, 0.34f, 1.00f };
                style.Colors[ImGuiCol_FrameBgActive]      = { 0.28f, 0.31f, 0.40f, 1.00f };

                // Tabs
                style.Colors[ImGuiCol_Tab]                = { 0.18f, 0.20f, 0.25f, 1.00f };
                style.Colors[ImGuiCol_TabHovered]         = { 0.30f, 0.50f, 0.90f, 0.80f };
                style.Colors[ImGuiCol_TabActive]          = { 0.22f, 0.44f, 0.80f, 1.00f };
                style.Colors[ImGuiCol_TabUnfocused]       = { 0.15f, 0.17f, 0.22f, 1.00f };
                style.Colors[ImGuiCol_TabUnfocusedActive] = { 0.20f, 0.38f, 0.68f, 1.00f };

                // Title bar
                style.Colors[ImGuiCol_TitleBg]            = { 0.10f, 0.11f, 0.15f, 1.00f };
                style.Colors[ImGuiCol_TitleBgActive]      = { 0.15f, 0.30f, 0.60f, 1.00f };
                style.Colors[ImGuiCol_TitleBgCollapsed]   = { 0.10f, 0.11f, 0.15f, 0.75f };

                // Menu bar
                style.Colors[ImGuiCol_MenuBarBg]          = { 0.10f, 0.11f, 0.15f, 1.00f };

                // Scrollbar
                style.Colors[ImGuiCol_ScrollbarBg]        = { 0.10f, 0.11f, 0.14f, 1.00f };
                style.Colors[ImGuiCol_ScrollbarGrab]      = { 0.28f, 0.30f, 0.40f, 1.00f };
                style.Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.38f, 0.42f, 0.55f, 1.00f };
                style.Colors[ImGuiCol_ScrollbarGrabActive] = { 0.48f, 0.52f, 0.68f, 1.00f };

                // Check mark / slider
                style.Colors[ImGuiCol_CheckMark]          = { 0.40f, 0.80f, 0.40f, 1.00f };
                style.Colors[ImGuiCol_SliderGrab]         = { 0.30f, 0.55f, 0.85f, 1.00f };
                style.Colors[ImGuiCol_SliderGrabActive]   = { 0.40f, 0.65f, 0.95f, 1.00f };

                // Buttons (global defaults; panel-specific overrides use PushStyleColor)
                style.Colors[ImGuiCol_Button]             = { 0.22f, 0.44f, 0.70f, 0.75f };
                style.Colors[ImGuiCol_ButtonHovered]      = { 0.32f, 0.54f, 0.82f, 1.00f };
                style.Colors[ImGuiCol_ButtonActive]       = { 0.18f, 0.36f, 0.60f, 1.00f };

                // Drag-drop highlight
                style.Colors[ImGuiCol_DragDropTarget]     = { 1.00f, 1.00f, 0.00f, 0.90f };

                // Separator
                style.Colors[ImGuiCol_Separator]          = { 0.28f, 0.30f, 0.38f, 0.60f };

                // Text
                style.Colors[ImGuiCol_Text]               = m_theme.textDefault;
                style.Colors[ImGuiCol_TextDisabled]       = m_theme.textDisabled;

                // Docking
                style.Colors[ImGuiCol_DockingPreview]     = { 0.30f, 0.55f, 0.90f, 0.70f };
                style.Colors[ImGuiCol_DockingEmptyBg]     = { 0.13f, 0.14f, 0.17f, 1.00f };

                // Rounding / padding — tweak to taste
                style.WindowRounding    = 4.0f;
                style.ChildRounding     = 4.0f;
                style.FrameRounding     = 3.0f;
                style.PopupRounding     = 4.0f;
                style.ScrollbarRounding = 3.0f;
                style.GrabRounding      = 3.0f;
                style.TabRounding       = 4.0f;
            }

            // Switch to a named preset. Unknown names fall back to Dark.
            void setTheme(const std::string& name) {
                if      (name == "Dark")     m_theme = makeThemeDark();
                else if (name == "Light")    m_theme = makeThemeLight();
                else if (name == "Dracula")  m_theme = makeThemeDracula();
                else if (name == "Midnight") m_theme = makeThemeMidnight();
                else if (name == "Pink")     m_theme = makeThemePink();
                else if (name == "Blue")     m_theme = makeThemeBlue();
                else if (name == "Black")    m_theme = makeThemeBlack();
                else if (name == "Purple")   m_theme = makeThemePurple();
                else if (name == "White")    m_theme = makeThemeWhite();
                else if (name == "Nord")     m_theme = makeThemeNord();
                else if (name == "Monokai")  m_theme = makeThemeMonokai();
                else if (name == "Solarized")m_theme = makeThemeSolarized();
                else if (name == "Cyberpunk")m_theme = makeThemeCyberpunk();
                else if (name == "Forest")   m_theme = makeThemeForest();
                else                         m_theme = makeThemeDark();

                applyToImGui();
            }

            // List of all available preset names
            static const std::vector<std::string>& presetNames() {
                static std::vector<std::string> names = {
                    "Dark", "Light", "Dracula", "Midnight",
                    "Pink", "Blue", "Black", "Purple", "White",
                    "Nord", "Monokai", "Solarized", "Cyberpunk", "Forest"
                };
                return names;
            }

        private:
            ThemeManager() { m_theme = makeThemeDark(); }
            EditorTheme m_theme;
        };

        // Convenience accessor used by all panels:
        //   auto& th = EditorTheme::current();
        //   ImGui::TextColored(th.textError, "...");
        namespace Theme {
            inline const EditorTheme& current() {
                return ThemeManager::get().theme();
            }
        }

    } // namespace Editor
} // namespace PAIN

#endif // _DEBUG
