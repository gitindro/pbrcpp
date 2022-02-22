#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/vertexbuffer.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <assert.h>
#include <stdlib.h>
#define SRALLOC_IMPLEMENTATION // Standard single-header-library detail
#include "sralloc/sralloc.h"

#include "common.h"

#include "mesh.h"


//static kinc_g4_shader_t vertex_shader;
//static kinc_g4_shader_t fragment_shader;
//static kinc_g4_pipeline_t pipeline;
//static kinc_g4_vertex_buffer_t vertices;
//static kinc_g4_index_buffer_t indices;

#define HEAP_SIZE 1024 * 1024
static uint8_t* heap = NULL;
static size_t heap_top = 0;
srallocator_t* g_mallocalloc = NULL;
srallocator_t* g_resource_alloc = NULL;

model_t* model_m = NULL;

static void* allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

static void update(void) {
	kinc_g4_begin(0);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR | KINC_G4_CLEAR_DEPTH, 0, 1.0f, 0);

	if(model_m != NULL)
		draw_model(model_m);

	kinc_g4_end(0);
	kinc_g4_swap_buffers();
}

static void load_shader(const char* filename, kinc_g4_shader_t* shader, kinc_g4_shader_type_t shader_type)
{
	kinc_file_reader_t file;
	kinc_file_reader_open(&file, filename, KINC_FILE_TYPE_ASSET);
	size_t data_size = kinc_file_reader_size(&file);
	uint8_t* data = SRALLOC_BYTES(g_resource_alloc, data_size);
	kinc_file_reader_read(&file, data, data_size);
	kinc_file_reader_close(&file);
	kinc_g4_shader_init(shader, data, data_size, shader_type);
	SRALLOC_DEALLOC(g_resource_alloc, data);
}

int kickstart(int argc, char** argv) 
{
	g_mallocalloc = sralloc_create_malloc_allocator( "root" );
	g_resource_alloc  = sralloc_create_stack_allocator( "resource", g_mallocalloc, 207766628);

	kinc_init("pbr", 1080, 768, NULL, NULL);
	kinc_set_update_callback(update);

	model_m = load_model("DamagedHelmet.glb", "shader.vert", "shader.frag");

	kinc_start();

	SRALLOC_DEALLOC(g_mallocalloc, model_m);
	sralloc_destroy_stack_allocator(g_resource_alloc);
	sralloc_destroy_malloc_allocator( g_mallocalloc );
	return 0;
}
