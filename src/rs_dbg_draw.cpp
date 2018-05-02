#include "rs_dbg_draw.h"
#include "rs_renderer.h"
#include "rs_array.h"

struct QuadVertex
{
    f32 x, y, z;
    u32 color;

    QuadVertex() = default;
    QuadVertex(f32 _x, f32 _y, f32 _z, u32 _c) {
        x = _x;
        y = _y;
        z = _z;
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
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec4 color;
            uniform mat4 uProjMatrix;
            uniform mat4 uViewMatrix;
            uniform mat4 uModelMatrix;

            out vec4 vert_color;

            void main()
            {
                vert_color = color;
                gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(position, 1.0);
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

        list.vertexAttribPointer(Location::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                                (GLvoid*)OFFSETOF(QuadVertex, x));
        list.vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadVertex),
                                (GLvoid*)OFFSETOF(QuadVertex, color));
        renderer_pushCommandList(list);
    }
};

#define MAX_QUAD_VERTICES 2048
struct DbgDraw
{

ColorShader colorShader;
QuadVertex quadVertexData[DbgCoordSpace::COUNT][MAX_QUAD_VERTICES];
MutexSpin quadVertexDataMutex[DbgCoordSpace::COUNT];

struct SolidSquare
{
    vec3f pos;
    vec3f size;
    u32 color;
};

Array<SolidSquare,32> solidSquares[DbgCoordSpace::COUNT];
mat4 matProj[DbgCoordSpace::COUNT];
mat4 matView[DbgCoordSpace::COUNT];

void init()
{
    colorShader.loadAndCompile();
    CommandList list;
    list.arrayBufferData(&colorShader.vbo, 0, sizeof(quadVertexData), GL_DYNAMIC_DRAW);
    renderer_pushCommandList(list);
}

void setView(const mat4& proj, const mat4& view, DbgCoordSpace coordSpace)
{
    matProj[(i32)coordSpace] = proj;
    matView[(i32)coordSpace] = view;
}

void drawSolidSquare(const vec3f& pos, const vec3f& size, const u32 color, DbgCoordSpace coordSpace)
{
    quadVertexDataMutex[(i32)coordSpace].lock();
    SolidSquare ssq = {pos, size, color};
    solidSquares[(i32)coordSpace].pushPOD(&ssq);
    quadVertexDataMutex[(i32)coordSpace].unlock();
}

void render()
{
    static mat4 model = mat4Identity();

    // ugly but works
    for(i32 space = 0; space < (i32)DbgCoordSpace::COUNT; space++) {
        auto& spaceSolidSquares = solidSquares[space];
        const i32 solidSquareCount = spaceSolidSquares.count();
        if(!solidSquareCount) continue;

        quadVertexDataMutex[space].lock();
        QuadVertex* qv = quadVertexData[space];
        for(i32 i = 0; i < solidSquareCount; i++) {
            const SolidSquare& ss = spaceSolidSquares[i];
            const f32 x = ss.pos.x;
            const f32 y = ss.pos.y;
            const f32 z = ss.pos.z;
            const f32 sx = ss.size.x;
            const f32 sy = ss.size.y;
            const u32 c = ss.color;
            qv[0] = QuadVertex(x, y, z, c);
            qv[1] = QuadVertex(x + sx, y, z, c);
            qv[2] = QuadVertex(x + sx, y + sy, z, c);
            qv[3] = QuadVertex(x, y, z, c);
            qv[4] = QuadVertex(x, y + sy, z, c);
            qv[5] = QuadVertex(x + sx, y + sy, z, c);
            qv += 6;
        }

        assert(qv - quadVertexData[space] <= MAX_QUAD_VERTICES);

        spaceSolidSquares.clear();

        CommandList list;
        list.useProgram(&colorShader.program);
        list.uniformMat4(colorShader.uProjMatrix, &matProj[space]);
        list.uniformMat4(colorShader.uViewMatrix, &matView[space]);
        list.uniformMat4(colorShader.uModelMatrix, &model);

        list.arrayBufferSubData(&colorShader.vbo, 0, quadVertexData[space],
                                sizeof(QuadVertex) * 6 * solidSquareCount);
        list.mutexUnlock(&quadVertexDataMutex[space]);

        list.setTransparencyEnabled(true);
        list.bindVertexArray(&colorShader.vao);
        list.drawTriangles(0, 6 * solidSquareCount);
        renderer_pushCommandList(list);
    }
}

};

static DbgDraw g_ddraw;

void dbgDrawInit()
{
    g_ddraw.init();
}

void dbgDrawSetView(const mat4& proj, const mat4& view, DbgCoordSpace coordSpace)
{
    g_ddraw.setView(proj, view, coordSpace);
}

void dbgDrawSolidSquare(const vec3f& pos, const vec3f& size, const u32 color, DbgCoordSpace coordSpace)
{
    g_ddraw.drawSolidSquare(pos, size, color, coordSpace);
}

void dbgDrawRender()
{
    g_ddraw.render();
}
