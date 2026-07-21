// Definicion de lenguaje BennuGD2 para ImGuiColorTextEdit (resaltado de sintaxis).
// Se basa en la de C (mismos comentarios // y /* */, strings, numeros) y cambia
// las palabras clave e identificadores por los de BennuGD2 + la API del motor.
#pragma once
#include "TextEditor.h"

inline const TextEditor::LanguageDefinition& BennuGD2Language() {
    static bool inited = false;
    static TextEditor::LanguageDefinition lang;
    if (inited) return lang;
    inited = true;

    lang = TextEditor::LanguageDefinition::C();   // base: comentarios, strings, numeros
    lang.mName = "BennuGD2";
    lang.mCaseSensitive = false;                  // BennuGD2 no distingue mayus/minus

    // ---- palabras clave (estructura + tipos) ----
    static const char* const kw[] = {
        "PROGRAM","IMPORT","GLOBAL","LOCAL","PRIVATE","PUBLIC","CONST","DECLARE",
        "BEGIN","END","PROCESS","FUNCTION","TYPE","STRUCT",
        "IF","ELSE","ELSEIF","ELSIF","THEN","WHILE","WEND","REPEAT","UNTIL",
        "FOR","LOOP","SWITCH","CASE","DEFAULT","RETURN","BREAK","CONTINUE",
        "FRAME","CLONE","DEBUG","FROM","TO","STEP","ONEXIT",
        "AND","OR","NOT","XOR","MOD",
        "int","int8","int16","int32","int64","float","double","string",
        "byte","word","dword","short","char","pointer","void"
    };
    lang.mKeywords.clear();
    for (auto k : kw) lang.mKeywords.insert(k);

    // ---- identificadores conocidos (motor + built-ins) con tooltip ----
    static const char* const ids[] = {
        // built-ins / gfx / input
        "write","write_var","write_int","say","key","out_region","set_mode","set_fps",
        "load_png","load_map","load_fnt","fopen","fclose","fputs","fgets","rand","abs",
        "sin","cos","tan","atan2","sqrt","exit","get_timer","advance","x","y","z","graph",
        // motor 3D (libmod_3d)
        "g3d_scene_create","g3d_scene_set_active","g3d_camera_create","g3d_camera_set_active",
        "g3d_camera_set_position","g3d_camera_look_at","g3d_light_create","g3d_light_set_direction",
        "g3d_light_set_intensity","g3d_load_gltf","g3d_load_fbx","g3d_model_spawn",
        "g3d_model_animate","g3d_model_animate_blend","g3d_entity_set_position",
        "g3d_entity_set_rotation","g3d_entity_set_scale","g3d_primitive_terrain",
        "g3d_terrain_get_height","g3d_terrain_raise","g3d_terrain_smooth","g3d_terrain_paint",
        "g3d_char_create","g3d_char_move","g3d_char_jump","g3d_char_update","g3d_char_grounded",
        "g3d_pick_terrain","g3d_pick_entity","g3d_pick_x","g3d_pick_y","g3d_pick_z",
        "g3d_sky_set_gradient","g3d_sky_enable","g3d_set_shadows","g3d_set_hdr",
        "g3d_model_node_find","g3d_model_node_x","g3d_model_node_y","g3d_model_node_z"
    };
    for (auto i : ids) {
        TextEditor::Identifier id;
        id.mDeclaration = "BennuGD2 / libmod_3d";
        lang.mIdentifiers.insert(std::make_pair(std::string(i), id));
    }
    return lang;
}
