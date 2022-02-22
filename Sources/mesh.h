#ifndef  _mesh_h_
#define _mesh_h_
#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/texture.h>
#include <kinc/graphics4/vertexbuffer.h>
#include "HandmadeMath.h"

enum pbr_tex_type
{

};
typedef struct property_texture
{
    kinc_g4_texture_unit_t* tex_unit;
    kinc_g4_texture_t* source_texture;
}property_texture_t;

typedef struct material
{
    kinc_g4_pipeline_t pipeline;
    kinc_g4_constant_location_t camera_pos;
    kinc_g4_constant_location_t mat4_model;
    kinc_g4_constant_location_t mat4_view;
    kinc_g4_constant_location_t mat4_proj;

    kinc_g4_texture_unit_t  diffuseTextureUnit;
    kinc_g4_texture_unit_t  normalTextureUnit;
    kinc_g4_texture_unit_t  metallicRoughnessTextureUnit;
    kinc_g4_texture_unit_t  aoTextureUnit;
    
    int diffuse_tex_index;
    int normal_tex_index;
    int emissive_tex_index;
    int mettalic_roughness_tex_index;
    int ao_tex_index;
    hmm_vec3    albedo;
    float metallic;
    float roughness;
    float ao;
    property_texture_t    textures[8];
}material_t;

typedef 
struct submesh
{
    material_t material_used;
    int material_Index;
    
    kinc_g4_vertex_buffer_t vb;
    kinc_g4_index_buffer_t ib;
}submesh_t;

typedef
struct model
{
    int submesh_count;
    submesh_t submeshes[8];

    int texture_count;
    kinc_g4_texture_t textures[8];
    int material_count;
    material_t materials[8];
    kinc_g4_shader_t vertex_shader;
    kinc_g4_shader_t fragment_shader;
 
}model_t;

model_t* load_model(const char* filepath, const char* vertexShader, const char* fragmentShader);
void draw_model(model_t* amodel);



#endif //_mesh_h_