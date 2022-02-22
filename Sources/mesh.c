
#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include "common.h"
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include "mesh.h"


int process_aiMesh(model_t* model, const C_STRUCT aiMesh* aimesh, kinc_g4_shader_t* vertex_shader, kinc_g4_shader_t* fragment_shader)
{
	int index = model->submesh_count;
	int material_index = aimesh->mMaterialIndex;

	kinc_g4_vertex_structure_t structure;
	kinc_g4_vertex_structure_init(&structure);
	kinc_g4_vertex_structure_add(&structure, "pos", KINC_G4_VERTEX_DATA_F32_3X);
	kinc_g4_vertex_structure_add(&structure, "normal", KINC_G4_VERTEX_DATA_F32_3X);
	kinc_g4_vertex_structure_add(&structure, "texcoord", KINC_G4_VERTEX_DATA_F32_2X);


	
	kinc_g4_pipeline_init(&model->materials[material_index].pipeline);
	model->materials[material_index].pipeline.depth_attachment_bits = 16;
	model->materials[material_index].pipeline.vertex_shader = &model->vertex_shader;
	model->materials[material_index].pipeline.fragment_shader = &model->fragment_shader;
	model->materials[material_index].pipeline.input_layout[0] = &structure;
	model->materials[material_index].pipeline.input_layout[1] = NULL;
	//model->materials[material_index].pipeline.cull_mode = KINC_G4_CULL_NOTHING;
	model->materials[material_index].pipeline.depth_write = true;
	model->materials[material_index].pipeline.depth_mode = KINC_G4_COMPARE_LESS;
	model->materials[material_index].pipeline.color_attachment_count = 1;
	model->materials[material_index].pipeline.color_attachment[0] = KINC_G4_RENDER_TARGET_FORMAT_32BIT;
	model->materials[material_index].pipeline.depth_attachment_bits = 16;
	//model->materials[material_index].pipeline.depth_mode
	kinc_g4_pipeline_compile(&model->materials[material_index].pipeline);


	model->materials[material_index].mat4_model = kinc_g4_pipeline_get_constant_location(&model->materials[material_index].pipeline, "model");
	model->materials[material_index].mat4_view = kinc_g4_pipeline_get_constant_location(&model->materials[material_index].pipeline, "view");
	model->materials[material_index].mat4_proj = kinc_g4_pipeline_get_constant_location(&model->materials[material_index].pipeline, "projection");
	model->materials[material_index].camera_pos = kinc_g4_pipeline_get_constant_location(&model->materials[material_index].pipeline, "cameraPosition");
	//model->submeshes[index].

	model->materials[material_index].diffuseTextureUnit =  kinc_g4_pipeline_get_texture_unit(&model->materials[material_index].pipeline, "albedoSampler");
	model->materials[material_index].normalTextureUnit = kinc_g4_pipeline_get_texture_unit(&model->materials[material_index].pipeline, "normalMapSampler");
	model->materials[material_index].metallicRoughnessTextureUnit = kinc_g4_pipeline_get_texture_unit(&model->materials[material_index].pipeline, "metallicRoughnessMapSampler");
	model->materials[material_index].aoTextureUnit = kinc_g4_pipeline_get_texture_unit(&model->materials[material_index].pipeline, "aoMapSampler"); 

	//const C_STRUCT aiMaterial* amat =  aimesh->mMaterialIndex

	
	int vert_count = aimesh->mNumVertices;
	model->submeshes[index].material_Index = aimesh->mMaterialIndex;

	kinc_g4_vertex_buffer_init(&model->submeshes[index].vb, vert_count, &structure, KINC_G4_USAGE_STATIC, 0);
	{
		float* v_ptr = kinc_g4_vertex_buffer_lock_all(&model->submeshes[index].vb);
		int v = 0;

		for (size_t i = 0; i < vert_count; i++)
		{
			v_ptr[v++] = aimesh->mVertices[i].x;
			v_ptr[v++] = aimesh->mVertices[i].y;
			v_ptr[v++] = aimesh->mVertices[i].z;

			v_ptr[v++] = aimesh->mNormals[i].x;
			v_ptr[v++] = aimesh->mNormals[i].y;
			v_ptr[v++] = aimesh->mNormals[i].z;

			v_ptr[v++] = aimesh->mTextureCoords[0][i].x;
			v_ptr[v++] = aimesh->mTextureCoords[0][i].y;
		}

		kinc_g4_vertex_buffer_unlock_all(&model->submeshes[index].vb);
	}

	int total_indices = 0;
	for (size_t i = 0; i < aimesh->mNumFaces; i++)
	{
		for (size_t j = 0; j < aimesh->mFaces[i].mNumIndices; j++)
			++total_indices;
	}

	// we could have done [ total_indices = aimesh->mNumFaces * 3] . we are only processing triangle primitives

	kinc_g4_index_buffer_init(&model->submeshes[index].ib, total_indices, KINC_G4_INDEX_BUFFER_FORMAT_32BIT, KINC_G4_USAGE_STATIC);
	{
		int idx = 0;
		uint32_t* i_ptr = (uint32_t*)kinc_g4_index_buffer_lock(&model->submeshes[index].ib);
		for (size_t i = 0; i < aimesh->mNumFaces; i++)
		{
			for (size_t j = 0; j < aimesh->mFaces[i].mNumIndices; j++)
			{
				i_ptr[idx] = aimesh->mFaces[i].mIndices[j];
				++idx;
			}
		}
		
		kinc_g4_index_buffer_unlock(&model->submeshes[index].ib);
	}

	model->submesh_count++;
}

void process_node(model_t* model, const C_STRUCT aiScene* scene, const C_STRUCT  aiNode* node, kinc_g4_shader_t* vertex_shader, kinc_g4_shader_t* fragment_shader)
{
	for(size_t m=0;m<node->mNumMeshes;++m)
	{
		process_aiMesh(model, scene->mMeshes[ node->mMeshes[m]], vertex_shader, fragment_shader);
	}

	for (size_t n = 0; n < node->mNumChildren; n++)
	{
		process_node(model, scene, node->mChildren[n], vertex_shader, fragment_shader);
	}
}

static void load_shader(const char* filename, kinc_g4_shader_t* shader, kinc_g4_shader_type_t shader_type) {
	kinc_file_reader_t file;
	kinc_file_reader_open(&file, filename, KINC_FILE_TYPE_ASSET);
	size_t data_size = kinc_file_reader_size(&file);
	uint8_t* data = SRALLOC_BYTES(g_resource_alloc, data_size);
	kinc_file_reader_read(&file, data, data_size);
	kinc_file_reader_close(&file);
	kinc_g4_shader_init(shader, data, data_size, shader_type);
	SRALLOC_DEALLOC(g_resource_alloc, data);
}

void load_all_texture(const C_STRUCT aiScene* scene, model_t* model)
{
	for (size_t i = 0; i < scene->mNumTextures; i++)
	{
		kinc_image_t image;
		uint8_t* data = SRALLOC_BYTES(g_resource_alloc, scene->mTextures[i]->mWidth);
		int ret = kinc_image_init_from_encoded_bytes(&image, data, scene->mTextures[i]->pcData, scene->mTextures[i]->mWidth, "png");

		kinc_g4_texture_init_from_image(&model->textures[model->texture_count], &image);
		model->texture_count++;
		SRALLOC_DEALLOC(g_resource_alloc, data);
	}
}


void load_all_material(const C_STRUCT aiScene* scene, model_t* model)
{
	const C_STRUCT aiString path;
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		const C_STRUCT aiMaterial* amat = scene->mMaterials[i];


		model->materials[model->material_count].diffuse_tex_index = -1;
		unsigned int base_texture_count = aiGetMaterialTextureCount(amat, aiTextureType_DIFFUSE);
		if (base_texture_count > 0) //Has base texture
		{
			aiGetMaterialTexture(amat, aiTextureType_DIFFUSE, 0, &path, NULL,NULL,NULL, NULL, NULL, NULL);
	
			// lookup using texture ID (if referenced like: "*1", "*2", etc.)
			if ('*' == path.data[0]) {
				int index = atoi(&path.data[1]);
				model->materials[model->material_count].diffuse_tex_index = index;
			}
		}

		model->materials[model->material_count].normal_tex_index = -1;
		unsigned int normal_texture_count = aiGetMaterialTextureCount(amat, aiTextureType_NORMALS);
		if (normal_texture_count > 0) //Has base texture
		{
			aiGetMaterialTexture(amat, aiTextureType_NORMALS, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			// lookup using texture ID (if referenced like: "*1", "*2", etc.)
			if ('*' == path.data[0]) {
				int index = atoi(&path.data[1]);
				model->materials[model->material_count].normal_tex_index = index;
			}
		}

		model->materials[model->material_count].emissive_tex_index = -1;
		unsigned int emissive_texture_count = aiGetMaterialTextureCount(amat, aiTextureType_EMISSIVE);
		if (emissive_texture_count > 0) //Has base texture
		{
			aiGetMaterialTexture(amat, aiTextureType_EMISSIVE, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			// lookup using texture ID (if referenced like: "*1", "*2", etc.)
			if ('*' == path.data[0]) {
				int index = atoi(&path.data[1]);
				model->materials[model->material_count].emissive_tex_index = index;
			}
		}


		//unsigned int metallic_texture_count = aiGetMaterialTextureCount(amat, AI_MATKEY_METALLIC_TEXTURE);
		//if (metallic_texture_count > 0) //Has base texture
		//{
		//	aiGetMaterialTexture(amat, AI_MATKEY_METALLIC_TEXTURE, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

		//	// lookup using texture ID (if referenced like: "*1", "*2", etc.)
		//	if ('*' == path.data[0]) {
		//		int index = atoi(&path.data[1]);
		//		model->materials[model->material_count].mettalic_tex_index = index;
		//	}
		//}
		
		model->materials[model->material_count].mettalic_roughness_tex_index = -1;
		unsigned int roughnesss_texture_count = aiGetMaterialTextureCount(amat, aiTextureType_UNKNOWN);
		if (roughnesss_texture_count > 0) //Has base texture
		{
			C_STRUCT aiString aa;
			aiGetMaterialTexture(amat, aiTextureType_UNKNOWN, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			// lookup using texture ID (if referenced like: "*1", "*2", etc.)
			if ('*' == path.data[0]) {
				int index = atoi(&path.data[1]);
				model->materials[model->material_count].mettalic_roughness_tex_index = index;
			}
		}

		unsigned int ao_texture_count = aiGetMaterialTextureCount(amat, aiTextureType_LIGHTMAP);
		if (ao_texture_count > 0) //Has base texture
		{
			aiGetMaterialTexture(amat, aiTextureType_LIGHTMAP, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			// lookup using texture ID (if referenced like: "*1", "*2", etc.)
			if ('*' == path.data[0]) {
				int index = atoi(&path.data[1]);
				model->materials[model->material_count].ao_tex_index = index;
			}
		}

		model->material_count++;
	}
}

model_t* load_model(const char* filepath, const char* vertexShader, const char* fragmentShader)
{
	kinc_file_reader_t file;
	kinc_file_reader_open(&file, filepath, KINC_FILE_TYPE_ASSET);
	size_t data_size = kinc_file_reader_size(&file);

	char* buf = SRALLOC_BYTES(g_resource_alloc, data_size);
	kinc_file_reader_read(&file, buf, data_size);
	kinc_file_reader_close(&file);

	const C_STRUCT aiScene* scene = aiImportFileFromMemory(buf, data_size, aiProcess_Triangulate | aiProcess_CalcTangentSpace  , "glb");

	SRALLOC_DEALLOC(g_resource_alloc, buf);


	const C_STRUCT aiNode* root = scene->mRootNode;

	model_t* model = SRALLOC_OBJECT(g_mallocalloc, model_t);
	model->submesh_count = 0;
	model->texture_count = 0;
	model->material_count = 0;

	load_all_texture(scene, model);
	load_all_material(scene, model);

	load_shader(vertexShader, &model->vertex_shader, KINC_G4_SHADER_TYPE_VERTEX);
	load_shader(fragmentShader, &model->fragment_shader, KINC_G4_SHADER_TYPE_FRAGMENT);

	process_node(model, scene, root, &model->vertex_shader, &model->fragment_shader);

	return model;
}

 kinc_matrix4x4_t perspectiveProjection(float fovY, float aspect, float zn, float zf )
{
	
	float uh = 1.0 / kinc_tan(fovY / 2);
	float uw = uh / aspect;

	kinc_matrix4x4_t mat;
	kinc_matrix4x4_set(&mat, 0, 0, uw); kinc_matrix4x4_set(&mat, 1, 0, 0); kinc_matrix4x4_set(&mat, 2, 0, 0); kinc_matrix4x4_set(&mat, 3, 0, 0);
	kinc_matrix4x4_set(&mat, 0, 1, 0); kinc_matrix4x4_set(&mat, 1, 1, uh); kinc_matrix4x4_set(&mat, 2, 1, 0); kinc_matrix4x4_set(&mat, 3, 1, 0);
	kinc_matrix4x4_set(&mat, 0, 2, 0); kinc_matrix4x4_set(&mat, 1, 2, 0); kinc_matrix4x4_set(&mat, 2, 2, (zf + zn) / (zn - zf)); kinc_matrix4x4_set(&mat, 3, 2, 2 * zf * zn / (zn - zf));
	kinc_matrix4x4_set(&mat, 0, 3, 0); kinc_matrix4x4_set(&mat, 1, 3, 0); kinc_matrix4x4_set(&mat, 2, 3, -1); kinc_matrix4x4_set(&mat, 3, 3, 0);

	return  mat;
}

 /*
public static function lookAt(eye: Vector3, at : Vector3, up : Vector3) : Matrix4{
	var zaxis = at.sub(eye).normalized();
	var xaxis = zaxis.cross(up).normalized();
	var yaxis = xaxis.cross(zaxis);

	return new Matrix4(xaxis.x, xaxis.y, xaxis.z, -xaxis.dot(eye), yaxis.x, yaxis.y, yaxis.z, -yaxis.dot(eye), -zaxis.x, -zaxis.y, -zaxis.z,
		zaxis.dot(eye), 0, 0, 0, 1);
}
*/

void draw_model(model_t* amodel)// , hmm_mat4 model, hmm_mat4 view, hmm_mat4 proj)
{
	for (size_t i = 0; i < amodel->submesh_count; i++)
	{
		material_t* using_material = &amodel->materials[amodel->submeshes[i].material_Index];
		//set material
		kinc_g4_set_pipeline(&using_material->pipeline);
		kinc_g4_set_vertex_buffer(&amodel->submeshes[i].vb);
		kinc_g4_set_index_buffer(&amodel->submeshes[i].ib);
		kinc_matrix4x4_t temp_mat;
		hmm_mat4 mat_model =  HMM_MultiplyMat4(HMM_Rotate(20.0f*(float)kinc_time(), HMM_Vec3(0.0f, 1.0f, 0.0f)),  HMM_Mat4d(1.0f)); // identity;
	
		hmm_mat4 mat_view = HMM_LookAt(HMM_Vec3(0, 0, -6), HMM_Vec3(0, 0, 0), HMM_Vec3(0, 1, 0));
		hmm_mat4 mat_projection = HMM_Perspective(45.0f, 1.33, 1.0f, 100.0f);

		kinc_matrix4x4_t mmm = perspectiveProjection(45.0f, 1.33f, 0.1f, 100.0f);

		kinc_g4_set_float3(using_material->camera_pos, 0, 0, -10);
		memcpy(&temp_mat.m[0], &mat_model.Elements[0], sizeof(float) * 16);
		kinc_g4_set_matrix4(using_material->mat4_model, &temp_mat);

		memcpy(&temp_mat.m[0], &mat_view.Elements[0], sizeof(float) * 16);
		kinc_g4_set_matrix4(using_material->mat4_view, &temp_mat);

		memcpy(&temp_mat.m[0], &mat_projection.Elements[0], sizeof(float) * 16);
		kinc_g4_set_matrix4(using_material->mat4_proj, &temp_mat);
		
		if(using_material->diffuse_tex_index > -1)
			kinc_g4_set_texture(using_material->diffuseTextureUnit, &amodel->textures[using_material->diffuse_tex_index]);

		if (using_material->normal_tex_index > -1)
			kinc_g4_set_texture(using_material->normalTextureUnit, &amodel->textures[using_material->normal_tex_index]);

		if (using_material->mettalic_roughness_tex_index > -1)
			kinc_g4_set_texture(using_material->metallicRoughnessTextureUnit, &amodel->textures[using_material->mettalic_roughness_tex_index]);

		if (using_material->ao_tex_index > -1)
			kinc_g4_set_texture(using_material->aoTextureUnit, &amodel->textures[using_material->ao_tex_index]);

		kinc_g4_draw_indexed_vertices();
		
	}
}