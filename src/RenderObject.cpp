#include "RenderObject.h"

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector3 facing, const char* obj_path, GLuint texture, GLuint normalMap = NULL) :
position(pos), scale(scale), facing(facing), texture(texture), normalMap(normalMap)
{
	CreateMesh(&mesh, obj_path);
}

RenderObject::RenderObject(Vector3 pos, Vector3 scale, Vector3 facing, ShapeType shape, GLuint texture, GLuint normalMap = NULL) :
	position(pos), scale(scale), facing(facing), texture(texture), normalMap(normalMap)
{
	CreateMesh(&mesh, shape);
}

RenderObject::~RenderObject()
{
}


void Render()
{


}
