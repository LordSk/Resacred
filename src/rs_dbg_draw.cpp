#include "rs_dbg_draw.h"
#include "rs_renderer.h"

struct QuadVertex
{
    f32 x, y;
    u32 color;

    QuadVertex() = default;
    QuadVertex(f32 _x, f32 _y, u32 _c) {
        x = _x;
        y = _y;
        color = _c;
    }
};

struct ColorShader
{
    GLuint program;
    i32 uProjMatrix;
    i32 uViewMatrix;
    i32 uModelMatrix;
    GLuint vao;
    GLuint vbo;

    void loadAndCompile()
    {
        // ui shader
        constexpr const char* vertexShader = R"FOO(
            #version 330 core
            layout(location = 0) in vec2 position;
            layout(location = 1) in vec4 color;
            uniform mat4 uProjMatrix;
            uniform mat4 uViewMatrix;
            uniform mat4 uModelMatrix;

            out vec4 vert_color;

            void main()
            {
                vert_color = color;
                gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(position, 0.0, 1.0);
            }
            )FOO";

        constexpr const char* fragmentShader = R"FOO(
            #version 330 core

            in vec4 vert_color;
            out vec4 fragmentColor;

            void main()
            {
                fragmentColor = vert_color;
            }
            )FOO";

        u32 vertexShaderLen = strlen(vertexShader);
        u32 fragmentShaderLen = strlen(fragmentShader);

        static MemBlock shaderBuffers[2];
        shaderBuffers[0].ptr = (void*)vertexShader;
        shaderBuffers[0].size = vertexShaderLen;
        shaderBuffers[1].ptr = (void*)fragmentShader;
        shaderBuffers[1].size = fragmentShaderLen;

        constexpr i32 types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };

        CommandList list;
        list.createShaderAndCompile(shaderBuffers, types, 2, &program);

        static i32* locations[] = {&uProjMatrix, &uViewMatrix, &uModelMatrix};
        static const char* uniformNames[] = {"uProjMatrix", "uViewMatrix", "uModelMatrix"};
        list.getUniformLocations(&program, locations, uniformNames, sizeof(locations)/sizeof(locations[0]));

        list.genVertexArrays(&vao, 1);
        list.bindVertexArray(&vao);
        list.genBuffers(&vbo, 1);
        list.bindArrayBuffer(&vbo);

        enum Location {
            POSITION = 0,
            COLOR = 1,
        };

        static i32 indexes[] = {POSITION, COLOR};
        list.enableVertexAttribArrays(indexes, sizeof(indexes)/sizeof(Location));

        list.vertexAttribPointer(Location::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                                (GLvoid*)OFFSETOF(QuadVertex, x));
        list.vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadVertex),
                                (GLvoid*)OFFSETOF(QuadVertex, color));
        renderer_pushCommandList(list);
    }
};

struct DbgDraw
{

ColorShader colorShader;
QuadVertex quadVertexData[6 * 256];
MutexSpin quadVertexDataMutex;
MutexSpin modelMutex;

void init()
{
    colorShader.loadAndCompile();
    CommandList list;
    list.arrayBufferData(&colorShader.vbo, 0, sizeof(quadVertexData), GL_DYNAMIC_DRAW);
    renderer_pushCommandList(list);
}

void setView(const mat4& proj, const mat4& view)
{
    CommandList list;
    list.useProgram(&colorShader.program);
    list.uniformMat4(colorShader.uProjMatrix, &proj);
    list.uniformMat4(colorShader.uViewMatrix, &view);
    renderer_pushCommandList(list);
}

void drawSolidSquare(const vec3f& pos, const vec3f& size, const u32 color)
{
    // ugly but works

    static mat4 model;
    modelMutex.lock();
    model = mat4Mul(mat4Translate(pos), mat4Scale(size));

    quadVertexDataMutex.lock();
    QuadVertex* qv = quadVertexData;
    qv[0] = QuadVertex(0, 0, color);
    qv[1] = QuadVertex(1, 0, color);
    qv[2] = QuadVertex(1, 1, color);
    qv[3] = QuadVertex(0, 0, color);
    qv[4] = QuadVertex(0, 1, color);
    qv[5] = QuadVertex(1, 1, color);

    CommandList list;
    list.useProgram(&colorShader.program);

    list.uniformMat4(colorShader.uModelMatrix, &model);
    list.mutexUnlock(&modelMutex);

    list.arrayBufferSubData(&colorShader.vbo, 0, quadVertexData, sizeof(QuadVertex) * 6);
    list.mutexUnlock(&quadVertexDataMutex);

    list.setTransparencyEnabled(true);
    list.bindVertexArray(&colorShader.vao);
    list.drawTriangles(0, 6);
    renderer_pushCommandList(list);
}

};

DbgDraw g_ddraw;

void dbgDrawInit()
{
    g_ddraw.init();
}

void dbgDrawSetView(const mat4& proj, const mat4& view)
{
    g_ddraw.setView(proj, view);
}

void dbgDrawSolidSquare(const vec3f& pos, const vec3f& size, const u32 color)
{
    g_ddraw.drawSolidSquare(pos, size, color);
}
