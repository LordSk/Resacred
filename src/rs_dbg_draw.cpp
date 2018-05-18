#include "rs_dbg_draw.h"
#include "rs_renderer.h"
#include "rs_array.h"

struct DbgDraw
{

struct SolidSquare
{
    vec3 pos;
    vec3 size;
    u32 color;
};

Array<SolidSquare,32> solidSquares[DbgCoordSpace::COUNT];
mat4 matProj[DbgCoordSpace::COUNT]; // TODO: port this to new renderer
mat4 matView[DbgCoordSpace::COUNT];

void init()
{
}

void setView(const mat4& proj, const mat4& view, DbgCoordSpace coordSpace)
{
    matProj[(i32)coordSpace] = proj;
    matView[(i32)coordSpace] = view;
}

void drawSolidSquare(const vec3& pos, vec3 size, const u32 color, DbgCoordSpace coordSpace)
{
    size.z = 1;
    SolidSquare ssq = {pos, size, color};
    solidSquares[(i32)coordSpace].pushPOD(&ssq);
}

void render(RendererFrameData& frame)
{
    for(i32 space = 0; space < (i32)DbgCoordSpace::COUNT; space++) {
        auto& spaceSolidSquares = solidSquares[space];
        const i32 solidSquareCount = spaceSolidSquares.count();
        if(!solidSquareCount) continue;

        QuadVertex qv[6];
        for(i32 i = 0; i < solidSquareCount; i++) {
            const SolidSquare& ss = spaceSolidSquares[i];
            const u32 c = ss.color;
            qv[0] = QuadVertex(0, 0, 0, c);
            qv[1] = QuadVertex(1, 0, 0, c);
            qv[2] = QuadVertex(1, 1, 0, c);
            qv[3] = QuadVertex(0, 0, 0, c);
            qv[4] = QuadVertex(0, 1, 0, c);
            qv[5] = QuadVertex(1, 1, 0, c);

            mat4 model = mat4Mul(mat4Translate(ss.pos), mat4Scale(ss.size));

            MeshDef meshDef;
            meshDef.vertOffset = frame.dbgQuadVertData.count();
            meshDef.vertCount = 6;

            frame.dbgQuadVertData.pushPOD(qv, 6);
            frame.dbgQuadModelMat.pushPOD(&model);
            frame.dbgQuadMeshDef.pushPOD(&meshDef);
        }

        spaceSolidSquares.clear();
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

void dbgDrawSolidSquare(const vec3& pos, const vec3& size, const u32 color, DbgCoordSpace coordSpace)
{
    g_ddraw.drawSolidSquare(pos, size, color, coordSpace);
}

void dbgDrawSetFrameData(RendererFrameData* frameData)
{
    g_ddraw.render(*frameData);
}
