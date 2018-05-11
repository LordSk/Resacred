#include "imgui_sdl2_setup.h"
#include "imgui.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include <stdlib.h>
#include <SDL2/SDL_scancode.h>

#if 0
static void renderUI(ImDrawData* pDrawData)
{
    ImGuiIO& io = ImGui::GetIO();
    i32 fb_width = (i32)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    i32 fb_height = (i32)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    pDrawData->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,
                                                   &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
    GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    OGL_DBG_GROUP_BEGIN(IMGUI_RENDER_SETUP);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup viewport, orthographic projection matrix
    const ImGuiGLSetup& ui = *(ImGuiGLSetup*)io.UserData;
    glUseProgram(ui.shaderProgram);
    glUniformMatrix4fv(ui.shaderViewUni, 1, GL_FALSE, ui.viewMatrix);
    glUniform1i(ui.shaderTextureUni, 0);

    glBindVertexArray(ui.shaderVao);

    OGL_DBG_GROUP_END(IMGUI_RENDER_SETUP);

    for(i32 n = 0; n < pDrawData->CmdListsCount; ++n) {
        const ImDrawList* cmd_list = pDrawData->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        OGL_DBG_GROUP_BEGIN(IMGUI_RENDER_BUFFER_BINDING);

        glBindBuffer(GL_ARRAY_BUFFER, ui.shaderVertexBuff);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert),
                     (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui.shaderElementsBuff);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx),
                     (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        OGL_DBG_GROUP_END(IMGUI_RENDER_BUFFER_BINDING);

        for(const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin();
            pcmd != cmd_list->CmdBuffer.end(); pcmd++) {
            if(pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w),
                          (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                          (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT,
                               idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFunc(last_blend_src, last_blend_dst);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}
#endif

void imguiInit(u32 width, u32 height, const char* iniFilename)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;
    io.IniFilename = iniFilename;
    io.RenderDrawListsFn = nullptr;
    io.UserData = nullptr;

    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
}

void imguiDeinit(ImGuiGLSetup* ims)
{
    ImGui::GetIO().Fonts->Clear();
    free(ims);
    ims = 0;
}

void imguiUpdate(f64 delta)
{
    static timept t0 = timeNow();
    ImGuiIO& io = ImGui::GetIO();
    timept now = timeNow();
    io.DeltaTime = timeDuration(now - t0);
    t0 = now;
    i32 mx, my;
    u32 mstate = SDL_GetMouseState(&mx, &my);
    io.MousePos = ImVec2((f32)mx, (f32)my);
    io.MouseDown[0] = mstate & SDL_BUTTON(SDL_BUTTON_LEFT);
    io.MouseDown[1] = mstate & SDL_BUTTON(SDL_BUTTON_RIGHT);
    ImGui::NewFrame();
}

void imguiHandleInput(SDL_Event event)
{
    ImGuiIO& io = ImGui::GetIO();

    if(event.type == SDL_MOUSEWHEEL) {
        io.MouseWheel = event.wheel.y;
        return;
    }

    if(event.type == SDL_KEYDOWN) {
        io.KeysDown[event.key.keysym.scancode] = true;

        if(event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER) {
            io.KeysDown[io.KeyMap[ImGuiKey_Enter]] = true;
        }

        if(event.key.keysym.scancode == SDL_SCANCODE_LCTRL ||
           event.key.keysym.scancode == SDL_SCANCODE_RCTRL) {
            io.KeyCtrl = true;
        }
        if(event.key.keysym.scancode == SDL_SCANCODE_LSHIFT ||
           event.key.keysym.scancode == SDL_SCANCODE_RSHIFT) {
            io.KeyShift = true;
        }
        if(event.key.keysym.scancode == SDL_SCANCODE_LALT ||
           event.key.keysym.scancode == SDL_SCANCODE_RALT) {
            io.KeyAlt = true;
        }

        if(event.key.keysym.mod & KMOD_CTRL) {
            io.KeyCtrl = true;
        }
        if(event.key.keysym.mod & KMOD_ALT) {
            io.KeyAlt = true;
        }
        if(event.key.keysym.mod & KMOD_SHIFT) {
            io.KeyShift = true;
        }
        return;
    }

    if(event.type == SDL_KEYUP) {
        io.KeysDown[event.key.keysym.scancode] = false;

        if(event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER) {
            io.KeysDown[io.KeyMap[ImGuiKey_Enter]] = false;
        }

        if(event.key.keysym.scancode == SDL_SCANCODE_LCTRL ||
           event.key.keysym.scancode == SDL_SCANCODE_RCTRL) {
            io.KeyCtrl = false;
        }
        if(event.key.keysym.scancode == SDL_SCANCODE_LSHIFT ||
           event.key.keysym.scancode == SDL_SCANCODE_RSHIFT) {
            io.KeyShift = false;
        }
        if(event.key.keysym.scancode == SDL_SCANCODE_LALT ||
           event.key.keysym.scancode == SDL_SCANCODE_RALT) {
            io.KeyAlt = false;
        }
        return;
    }

    if(event.type == SDL_TEXTINPUT) {
        io.AddInputCharactersUTF8(event.text.text);
        return;
    }
}
