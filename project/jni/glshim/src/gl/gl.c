#include "gl.h"
/*
glstate_t state = {.color = {1.0f, 1.0f, 1.0f, 1.0f},
	.secondary = {0.0f, 0.0f, 0.0f, 0.0f},
	.render_mode = 0,
	.projection_matrix = NULL,
	.modelview_matrix = NULL,
	.texture_matrix = NULL,
	.namestack = {0, NULL},
	.enable.vertex_array = 0,
	.enable.color_array = 0,
	.enable.secondary_array = 0,
	.enable.normal_array = 0,
	.texture.client = 0,
	.texture.active = 0,
	.buffers = {NULL, NULL, NULL, NULL},
	.shim_error = 0,
	.last_error = GL_NO_ERROR,
    .gl_batch = 0
	};
*/
glstate_t state;
void* gles = NULL;

GLuint readhack = 0;
GLint readhack_x = 0;
GLint readhack_y = 0;
GLfloat readhack_depth = 0.0f;
GLuint readhack_seq = 0;
GLuint gl_batch = 0;

__attribute__((constructor))
void initialize_glshim() {
	printf("LIBGL: Initialising glshim\n");
	
	GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	memset(&state, 0, sizeof(state));
	memcpy(state.color, white, sizeof(GLfloat)*4);
	state.last_error = GL_NO_ERROR;
    state.normal[3] = 1.0f; // default normal is 0/0/1
    
    // init read hack 
    char *env_readhack = getenv("LIBGL_READHACK");
    if (env_readhack && strcmp(env_readhack, "1") == 0) {
        readhack = 1;
        printf("LIBGL: glReadPixel Hack (for other-life, 1x1 reading multiple time)\n");
    }
    if (env_readhack && strcmp(env_readhack, "2") == 0) {
        readhack = 2;
        printf("LIBGL: glReadPixel Depth Hack (for games that read GLDepth always at the same place, same 1x1 size)\n");
    }
    char *env_batch = getenv("LIBGL_BATCH");
    if (env_batch && strcmp(env_batch, "1") == 0) {
        gl_batch = 1;
        printf("LIBGL: Batch mode enabled\n");
    }
    if (env_batch && strcmp(env_batch, "0") == 0) {
        gl_batch = 0;
        printf("LIBGL: Batch mode disabled\n");
    }
    
    if (gl_batch) init_batch();
    state.gl_batch = gl_batch;
}

// config functions
const GLubyte *glGetString(GLenum name) {
    LOAD_GLES(glGetString);
    const GLubyte *str;
    errorShim(GL_NO_ERROR);
	if ((str=gles_glGetString(name))==NULL)
		printf("**warning** glGetString(%i) called with bad init\n", name);
    switch (name) {
        case GL_VERSION:
#ifdef USE_ES2
            return (GLubyte *)"4.3 glshim wrapper";
#else
            return (GLubyte *)"1.5 glshim wrapper";
#endif
        case GL_EXTENSIONS:
            return (const GLubyte *)(char *){
#ifndef USE_ES2
                "GL_ARB_vertex_buffer_object "
                "GL_ARB_vertex_buffer "
                "GL_EXT_vertex_array "
                "GL_EXT_secondary_color "
                "GL_EXT_texture_env_combine "
                "GL_ARB_multitexture "
                "GL_ARB_texture_env_add "
                "GL_ARB_texture_border_clamp "
                "GL_ARB_point_parameters "
                "GL_ARB_texture_env_combine "
                "GL_ARB_texture_env_crossbar "
                "GL_ARB_texture_env_dot3 "
                "GL_ARB_texture_mirrored_repeat "
                "GL_SGIS_generate_mipmap "
                "GL_EXT_blend_subtract "
                "GL_EXT_packed_depth_stencil "
                "GL_EXT_draw_range_elements "
                "GL_EXT_bgra "
                "GL_ARB_texture_compression "
                "GL_EXT_texture_compression_s3tc "
                "GL_OES_texture_compression_S3TC "
                "GL_EXT_texture_compression_dxt3 "
                "GL_EXT_texture_compression_dxt5 "
                "GL_EXT_texture_compression_dxt1 "
                "GL_ARB_framebuffer_object "
                "GL_EXT_framebuffer_object "
                "GL_EXT_packed_depth_stencil "
                "GL_ARB_point_parameters "
                "GL_EXT_point_parameters "
                "GL_EXT_stencil_wrap "
                "GL_EXT_blend_func_separate "
                "GL_EXT_blend_equation_separate "
                "GL_ARB_draw_buffers "
//                "GL_EXT_blend_logic_op "
//                "GL_EXT_blend_color "
//                "GL_ARB_texture_cube_map "
#else
                "GL_ARB_vertex_shader "
                "GL_ARB_fragment_shader "
                "GL_ARB_vertex_buffer_object "
                "GL_EXT_framebuffer_object "
                "GL_EXT_vertex_array "
#endif
            };
		case GL_VENDOR:
			return (GLubyte *)"OpenPandora";
		case GL_RENDERER:
#ifdef USE_ES2
			return (GLubyte *)"GLESv2 wrapper";
#else
			return (GLubyte *)"GLES_CM wrapper";
		case GL_SHADING_LANGUAGE_VERSION:
			return (GLubyte *)"";
#endif
        default:
			errorShim(GL_INVALID_ENUM);
            return (str)?str:(GLubyte*)"";
    }
}

// glGet
extern float zoomx, zoomy;
extern GLfloat raster_scale[4];
extern GLfloat raster_bias[4];

void glGetIntegerv(GLenum pname, GLint *params) {
    if (params==NULL) {
        errorShim(GL_INVALID_OPERATION);
        return;
    }
    GLint dummy;
    LOAD_GLES(glGetIntegerv);
    noerrorShim();
    switch (pname) {
        case GL_MAX_ELEMENTS_INDICES:
            *params = 1024;
            break;
        case GL_MAX_ELEMENTS_VERTICES:
			*params = 4096;
			break;
        case GL_AUX_BUFFERS:
            *params = 0;
            break;
        case GL_MAX_DRAW_BUFFERS_ARB:   // fake...
            *params = 1;
            break;
        case GL_UNPACK_ROW_LENGTH:	
			*params = state.texture.unpack_row_length;
			break;
        case GL_UNPACK_SKIP_PIXELS:
			*params = state.texture.unpack_skip_pixels;
			break;
        case GL_UNPACK_SKIP_ROWS:
			*params = state.texture.unpack_skip_rows;
			break;
        case GL_UNPACK_LSB_FIRST:
			*params = state.texture.unpack_lsb_first;
			break;
        case GL_PACK_ROW_LENGTH:	
			*params = state.texture.pack_row_length;
			break;
        case GL_PACK_SKIP_PIXELS:
			*params = state.texture.pack_skip_pixels;
			break;
        case GL_PACK_SKIP_ROWS:
			*params = state.texture.pack_skip_rows;
			break;
        case GL_PACK_LSB_FIRST:
			*params = state.texture.pack_lsb_first;
			break;
	case GL_POINT_SIZE_RANGE:
			gles_glGetIntegerv(GL_POINT_SIZE_MIN, params);
			gles_glGetIntegerv(GL_POINT_SIZE_MAX, params+1);
			break;
	case GL_RENDER_MODE:
			*params = (state.render_mode)?state.render_mode:GL_RENDER;
			break;
	case GL_NAME_STACK_DEPTH:
			*params = state.namestack.top;
			break;
	case GL_MAX_NAME_STACK_DEPTH:
			*params = 1024;
			break;
	case GL_MAX_TEXTURE_IMAGE_UNITS:
			/*gles_glGetIntegerv(GL_MAX_TEXTURE_UNITS, params);*/
			*params = 4;
			break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
			gles_glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, params);
			(*params)+=4;	// adding fake DXTc
			break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
			gles_glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &dummy);
			// get standard ones
			gles_glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, params);
			// add fake DXTc
			params[dummy++]=COMPRESSED_RGB_S3TC_DXT1_EXT;
			params[dummy++]=COMPRESSED_RGBA_S3TC_DXT1_EXT;
			params[dummy++]=COMPRESSED_RGBA_S3TC_DXT3_EXT;
			params[dummy++]=COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
	case GL_MAX_MODELVIEW_STACK_DEPTH:
			*params=MAX_STACK_MODELVIEW;
			break;
	case GL_MAX_PROJECTION_STACK_DEPTH:
			*params=MAX_STACK_PROJECTION;
			break;
	case GL_MAX_TEXTURE_STACK_DEPTH:
			*params=MAX_STACK_TEXTURE;
			break;
	case GL_MODELVIEW_STACK_DEPTH:
			*params=(state.modelview_matrix)?(state.modelview_matrix->top+1):1;
			break;
	case GL_PROJECTION_STACK_DEPTH:
			*params=(state.projection_matrix)?(state.projection_matrix->top+1):1;
			break;
	case GL_TEXTURE_STACK_DEPTH:
			*params=(state.texture_matrix)?(state.texture_matrix[state.texture.active]->top+1):1;
			break;
	case GL_MAX_LIST_NESTING:
			*params=64;	// fake, no limit in fact
			break;
	case  GL_ARRAY_BUFFER_BINDING:
			*params=(state.buffers.vertex)?state.buffers.vertex->buffer:0;
			break;
	case  GL_ELEMENT_ARRAY_BUFFER_BINDING:
			*params=(state.buffers.elements)?state.buffers.elements->buffer:0;
			break;
	case  GL_PIXEL_PACK_BUFFER_BINDING:
			*params=(state.buffers.pack)?state.buffers.pack->buffer:0;
			break;
	case  GL_PIXEL_UNPACK_BUFFER_BINDING:
			*params=(state.buffers.unpack)?state.buffers.unpack->buffer:0;
			break;
    default:
			errorGL();
            gles_glGetIntegerv(pname, params);
    }
}

void glGetFloatv(GLenum pname, GLfloat *params) {
    LOAD_GLES(glGetFloatv);
    noerrorShim();
    switch (pname) {
        case GL_MAX_ELEMENTS_INDICES:
            *params = 1024;
            break;
        case GL_MAX_ELEMENTS_VERTICES:
	    *params = 4096;
	    break;
        case GL_AUX_BUFFERS:
            *params = 0;
            break;
        case GL_UNPACK_ROW_LENGTH:	
	    *params = state.texture.unpack_row_length;
	    break;
        case GL_UNPACK_SKIP_PIXELS:
	    *params = state.texture.unpack_skip_pixels;
	    break;
        case GL_UNPACK_SKIP_ROWS:
	    *params = state.texture.unpack_skip_rows;
	    break;
        case GL_UNPACK_LSB_FIRST:
	    *params = state.texture.unpack_lsb_first;
	    break;
        case GL_PACK_ROW_LENGTH:	
	    *params = state.texture.pack_row_length;
	    break;
        case GL_PACK_SKIP_PIXELS:
	    *params = state.texture.pack_skip_pixels;
	    break;
        case GL_PACK_SKIP_ROWS:
	    *params = state.texture.pack_skip_rows;
	    break;
        case GL_PACK_LSB_FIRST:
	    *params = state.texture.pack_lsb_first;
	    break;
        case GL_ZOOM_X:
	    *params = zoomx;
	    break;
        case GL_ZOOM_Y:
	    *params = zoomy;
	    break;
        case GL_RED_SCALE:
	    *params = raster_scale[0];
	    break;
        case GL_RED_BIAS:
	    *params = raster_bias[0];
	    break;
        case GL_GREEN_SCALE:
        case GL_BLUE_SCALE:
        case GL_ALPHA_SCALE:
	    *params = raster_scale[(pname-GL_GREEN_SCALE)/2+1];
	    break;
        case GL_GREEN_BIAS:
        case GL_BLUE_BIAS:
        case GL_ALPHA_BIAS:
	    *params = raster_bias[(pname-GL_GREEN_BIAS)/2+1];
	    break;
	case GL_POINT_SIZE_RANGE:
	    gles_glGetFloatv(GL_POINT_SIZE_MIN, params);
	    gles_glGetFloatv(GL_POINT_SIZE_MAX, params+1);
	    break;
	case GL_RENDER_MODE:
	    *params = (state.render_mode)?state.render_mode:GL_RENDER;
	    break;
	case GL_NAME_STACK_DEPTH:
	    *params = state.namestack.top;
	    break;
	case GL_MAX_NAME_STACK_DEPTH:
	    *params = 1024;
	    break;
	case GL_MAX_MODELVIEW_STACK_DEPTH:
	    *params=MAX_STACK_MODELVIEW;
	    break;
	case GL_MAX_PROJECTION_STACK_DEPTH:
	    *params=MAX_STACK_PROJECTION;
	    break;
	case GL_MAX_TEXTURE_STACK_DEPTH:
	    *params=MAX_STACK_TEXTURE;
	    break;
	case GL_MODELVIEW_STACK_DEPTH:
	    *params=(state.modelview_matrix)?(state.modelview_matrix->top+1):1;
	    break;
	case GL_PROJECTION_STACK_DEPTH:
	    *params=(state.projection_matrix)?(state.projection_matrix->top+1):1;
	    break;
	case GL_TEXTURE_STACK_DEPTH:
	    *params=(state.texture_matrix)?(state.texture_matrix[state.texture.active]->top+1):1;
	    break;
	case GL_MAX_LIST_NESTING:
	    *params=64;	// fake, no limit in fact
	    break;
	case  GL_ARRAY_BUFFER_BINDING:
		*params=(state.buffers.vertex)?state.buffers.vertex->buffer:0;
		break;
	case  GL_ELEMENT_ARRAY_BUFFER_BINDING:
		*params=(state.buffers.elements)?state.buffers.elements->buffer:0;
		break;
	case  GL_PIXEL_PACK_BUFFER_BINDING:
			*params=(state.buffers.pack)?state.buffers.pack->buffer:0;
			break;
	case  GL_PIXEL_UNPACK_BUFFER_BINDING:
			*params=(state.buffers.unpack)?state.buffers.unpack->buffer:0;
			break;
    default:
		errorGL();
		gles_glGetFloatv(pname, params);
    }
}

extern int alphahack;
extern int texstream;

#ifndef GL_TEXTURE_STREAM_IMG  
#define GL_TEXTURE_STREAM_IMG                                   0x8C0D     
#endif

static void proxy_glEnable(GLenum cap, bool enable, void (*next)(GLenum)) {
    #define proxy_enable(constant, name) \
        case constant: state.enable.name = enable; next(cap); break
    #define enable(constant, name) \
        case constant: state.enable.name = enable; break;

    // TODO: maybe could be weird behavior if someone tried to:
    // 1. enable GL_TEXTURE_1D
    // 2. enable GL_TEXTURE_2D
    // 3. disable GL_TEXTURE_1D
    // 4. render. GL_TEXTURE_2D would be disabled.
    // cap = map_tex_target(cap);
    
    // Alpha Hack
    if (alphahack && (cap==GL_ALPHA_TEST) && enable)
	if (state.texture.bound[state.texture.active])
	    if (!state.texture.bound[state.texture.active]->alpha)
		enable = false;
	noerrorShim();
    if (cap==GL_TEXTURE_STREAM_IMG)
	state.enable.texture_2d[state.texture.active] = enable;
    switch (cap) {
        proxy_enable(GL_BLEND, blend);
        proxy_enable(GL_TEXTURE_2D, texture_2d[state.texture.active]);
        enable(GL_TEXTURE_GEN_S, texgen_s[state.texture.active]);
        enable(GL_TEXTURE_GEN_T, texgen_t[state.texture.active]);
        enable(GL_TEXTURE_GEN_R, texgen_r[state.texture.active]);
        enable(GL_LINE_STIPPLE, line_stipple);
        
        // Secondary color
        enable(GL_COLOR_SUM, color_sum);
        enable(GL_SECONDARY_COLOR_ARRAY, secondary_array);
	
        // for glDrawArrays
        proxy_enable(GL_VERTEX_ARRAY, vertex_array);
        proxy_enable(GL_NORMAL_ARRAY, normal_array);
        proxy_enable(GL_COLOR_ARRAY, color_array);
        proxy_enable(GL_TEXTURE_COORD_ARRAY, tex_coord_array[state.texture.client]);
        
        // Texture 1D and 3D
        enable(GL_TEXTURE_1D, texture_1d[state.texture.active]);
        enable(GL_TEXTURE_3D, texture_3d[state.texture.active]);
        
        default: errorGL(); next(cap); break;
    }
    #undef proxy_enable
    #undef enable
}

void glEnable(GLenum cap) {
	PUSH_IF_COMPILING(glEnable)
        
	if (texstream && (cap==GL_TEXTURE_2D)) {
		if (state.texture.bound[state.texture.active])
			if (state.texture.bound[state.texture.active]->streamed)
				cap = GL_TEXTURE_STREAM_IMG;
	}

    LOAD_GLES(glEnable);
    proxy_glEnable(cap, true, gles_glEnable);
}

void glDisable(GLenum cap) {
	PUSH_IF_COMPILING(glDisable)
        
	if (texstream && (cap==GL_TEXTURE_2D)) {
		if (state.texture.bound[state.texture.active])
			if (state.texture.bound[state.texture.active]->streamed)
				cap = GL_TEXTURE_STREAM_IMG;
	}

    LOAD_GLES(glDisable);
    proxy_glEnable(cap, false, gles_glDisable);
}

#ifndef USE_ES2
void glEnableClientState(GLenum cap) {
    LOAD_GLES(glEnableClientState);
    proxy_glEnable(cap, true, gles_glEnableClientState);
}

void glDisableClientState(GLenum cap) {
    LOAD_GLES(glDisableClientState);
    proxy_glEnable(cap, false, gles_glDisableClientState);
}
#endif

GLboolean glIsEnabled(GLenum cap) {
    // should flush for now... to be optimized later!
    if (state.gl_batch) flush();
    LOAD_GLES(glIsEnabled);
    noerrorShim();
    switch (cap) {
        case GL_LINE_STIPPLE:
            return state.enable.line_stipple;
        case GL_TEXTURE_GEN_S:
            return state.enable.texgen_s[state.texture.active];
        case GL_TEXTURE_GEN_T:
            return state.enable.texgen_t[state.texture.active];
        case GL_TEXTURE_GEN_R:
            return state.enable.texgen_t[state.texture.active];
		case GL_TEXTURE_COORD_ARRAY:
			return state.enable.tex_coord_array[state.texture.client];
		case GL_COLOR_SUM:
			return state.enable.color_sum;
		case GL_SECONDARY_COLOR_ARRAY:
			return state.enable.secondary_array;
        case GL_TEXTURE_1D:
            return state.enable.texture_1d[state.texture.active];
        case GL_TEXTURE_3D:
            return state.enable.texture_1d[state.texture.active];
        default:
			errorGL();
            return gles_glIsEnabled(cap);
    }
}

static renderlist_t *arrays_to_renderlist(renderlist_t *list, GLenum mode,
                                        GLsizei skip, GLsizei count) {
    if (! list)
        list = alloc_renderlist();
//if (state.list.compiling) printf("arrary_to_renderlist while compiling list, skip=%d, count=%d\n", skip, count);
    list->mode = mode;
    list->mode_init = mode;
    list->len = count-skip;
    list->cap = count-skip;
    
	if (state.enable.vertex_array) {
		list->vert = copy_gl_pointer_raw(&state.pointers.vertex, 3, skip, count, state.pointers.vertex.buffer);	//TODO, what if size == 4
	}
	if (state.enable.color_array) {
		list->color = copy_gl_pointer_color(&state.pointers.color, 4, skip, count, state.pointers.color.buffer);
	}
	if (state.enable.secondary_array/* && state.enable.color_array*/) {
		list->secondary = copy_gl_pointer(&state.pointers.secondary, 4, skip, count, state.pointers.secondary.buffer);		// alpha chanel is always 0 for secondary...
	}
	if (state.enable.normal_array) {
		list->normal = copy_gl_pointer_raw(&state.pointers.normal, 3, skip, count, state.pointers.normal.buffer);
	}
	for (int i=0; i<MAX_TEX; i++) {
		if (state.enable.tex_coord_array[i]) {
		    list->tex[i] = copy_gl_pointer_raw(&state.pointers.tex_coord[i], 2, skip, count, state.pointers.tex_coord[i].buffer);
		}
	}
	
    end_renderlist(list);
    return list;
}

static inline bool should_intercept_render(GLenum mode) {
    return (
        (state.enable.vertex_array && ! valid_vertex_type(state.pointers.vertex.type)) ||
        (/*state.enable.texture_2d[0] && */(state.enable.texgen_s[0] || state.enable.texgen_t[0] || state.enable.texgen_r[0])) ||
        (/*state.enable.texture_2d[1] && */(state.enable.texgen_s[1] || state.enable.texgen_t[1] || state.enable.texgen_r[1])) ||
        (/*state.enable.texture_2d[2] && */(state.enable.texgen_s[2] || state.enable.texgen_t[2] || state.enable.texgen_r[2])) ||
        (/*state.enable.texture_2d[3] && */(state.enable.texgen_s[3] || state.enable.texgen_t[3] || state.enable.texgen_r[3])) ||
        (mode == GL_LINES && state.enable.line_stipple) ||
        (mode == GL_QUADS) || (state.list.active && (state.list.compiling || state.gl_batch))
    );
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
//printf("glDrawElements(0x%04X, %d, 0x%04X, %p), map=%p\n", mode, count, type, indices, (state.buffers.elements)?state.buffers.elements->data:NULL);
    // TODO: split for count > 65535?
    if (count<0) {
		errorShim(GL_INVALID_VALUE);
		return;
	}
    if (count==0) {
        noerrorShim();
        return;
    }

	noerrorShim();
    GLushort *sindices = copy_gl_array((state.buffers.elements)?state.buffers.elements->data + (uintptr_t)indices:indices,
		type, 1, 0, GL_UNSIGNED_SHORT, 1, 0, count);
    bool compiling = (state.list.active && (state.list.compiling || state.gl_batch));

    if (compiling) {
        renderlist_t *list = NULL;
        GLsizei min, max;

		NewStage(state.list.active, STAGE_DRAW);
        list = state.list.active;

        normalize_indices(sindices, &max, &min, count);
        list = arrays_to_renderlist(list, mode, min, max + 1 + min);
        list->indices = sindices;
        list->ilen = count;
        end_renderlist(list);
        
        state.list.active = extend_renderlist(list);
        return;
     }

     if (should_intercept_render(mode)) {
    // TODO: do this in a more direct fashion.
        glBegin(mode);
        for (int i = 0; i < count; i++) {
            glArrayElement(sindices[i]);
        }
        glEnd();
        free(sindices);
        return;
     } else {
		LOAD_GLES(glDrawElements);
		LOAD_GLES(glNormalPointer);
		LOAD_GLES(glVertexPointer);
		LOAD_GLES(glColorPointer);
		LOAD_GLES(glTexCoordPointer);
        LOAD_GLES(glEnable);
        LOAD_GLES(glDisable);
        GLuint len = 0;
        for (int i=0; i<count; i++)
            if (len<sindices[i]) len = sindices[i]; // get the len of the arrays
#define shift_pointer(a, b) \
		if (state.enable.b && state.pointers.a.buffer) state.pointers.a.pointer += (uintptr_t)state.pointers.a.buffer->data;
	
		shift_pointer(color, color_array);
		shift_pointer(secondary, secondary_array);
		shift_pointer(vertex, vertex_array);
		for (int aa=0; aa<MAX_TEX; aa++)
			shift_pointer(tex_coord[aa], tex_coord_array[aa]);
		shift_pointer(normal, normal_array);
#undef shift_pointer		


		GLenum mode_init = mode;
		if (state.polygon_mode == GL_LINE && mode>=GL_TRIANGLES)
			mode = GL_LINE_LOOP;
		if (state.polygon_mode == GL_POINT && mode>=GL_TRIANGLES)
			mode = GL_POINTS;

		if (mode == GL_QUAD_STRIP)
			mode = GL_TRIANGLE_STRIP;
		if (mode == GL_POLYGON)
			mode = GL_TRIANGLE_FAN;
		if (state.render_mode == GL_SELECT) {
			select_glDrawElements(&state.pointers.vertex, mode, count, GL_UNSIGNED_SHORT, sindices);
		} else {
			// secondary color...
			GLfloat *final_colors = NULL;
			pointer_state_t old_color;
			if (/*state.enable.color_sum && */(state.enable.secondary_array) && (state.enable.color_array)) {
				final_colors=copy_gl_pointer_color(&state.pointers.color, 4, 0, len, 0);
				GLfloat* seconds_colors=(GLfloat*)copy_gl_pointer(&state.pointers.secondary, 4, 0, len, 0);
				for (int i=0; i<len*4; i++)
					final_colors[i]+=seconds_colors[i];
				gles_glColorPointer(4, GL_FLOAT, 0, final_colors);
				free(seconds_colors);
			} else if (state.enable.color_array && (state.pointers.color.size != 4)) {
				// Pandora doesn't like Color Pointer with size != 4
				final_colors=copy_gl_pointer_color(&state.pointers.color, 4, 0, len, 0);
				gles_glColorPointer(4, GL_FLOAT, 0, final_colors);
			} else if (state.enable.color_array)
				gles_glColorPointer(state.pointers.color.size, state.pointers.color.type, state.pointers.color.stride, state.pointers.color.pointer);
			if (state.enable.normal_array)
				gles_glNormalPointer(state.pointers.normal.type, state.pointers.normal.stride, state.pointers.normal.pointer);
			if (state.enable.vertex_array)
				gles_glVertexPointer(state.pointers.vertex.size, state.pointers.vertex.type, state.pointers.vertex.stride, state.pointers.vertex.pointer);
			GLuint old_tex = state.texture.client;
			for (int aa=0; aa<MAX_TEX; aa++) {
                if (!state.enable.texture_2d[aa] && (state.enable.texture_1d[aa] || state.enable.texture_3d[aa])) {
                    glClientActiveTexture(aa+GL_TEXTURE0);
                    gles_glEnable(GL_TEXTURE_2D);
                }
				if (state.enable.tex_coord_array[aa]) {
                    tex_setup_texcoord(aa, len);
					/*glClientActiveTexture(aa+GL_TEXTURE0);
					gles_glTexCoordPointer(state.pointers.tex_coord[aa].size, state.pointers.tex_coord[aa].type, state.pointers.tex_coord[aa].stride, state.pointers.tex_coord[aa].pointer);*/
				}
            }
			if (state.texture.client!=old_tex)
				glClientActiveTexture(old_tex+GL_TEXTURE0);
				
			if (state.polygon_mode == GL_LINE && mode_init>=GL_TRIANGLES) {
				int n, s;
				switch (mode_init) {
					case GL_TRIANGLES:
						n = 3;
						s = 3;
						break;
					case GL_TRIANGLE_STRIP:
						n = 3;
						s = 1;
						break;
					case GL_TRIANGLE_FAN:	// wrong here...
						n = count;
						s = count;
						break;
					case GL_QUADS:
						n = 4;
						s = 4;
						break;
					case GL_QUAD_STRIP:
						n = 4;
						s = 1;
						break;
					default:		// Polygon and other?
						n = count;
						s = count;
						break;
				}
				for (int i=n; i<count; i+=s)
					gles_glDrawElements(mode, n, GL_UNSIGNED_SHORT, sindices+i-n);
			} else
				gles_glDrawElements(mode, count, GL_UNSIGNED_SHORT, sindices);
			
			// secondary color
			if (final_colors) {
				free(final_colors);
//				glColorPointer(old_color.size, old_color.type, old_color.stride, old_color.pointer);
			}
			for (int aa=0; aa<MAX_TEX; aa++) {
                if (!state.enable.texture_2d[aa] && (state.enable.texture_1d[aa] || state.enable.texture_3d[aa])) {
                    glClientActiveTexture(aa+GL_TEXTURE0);
                    gles_glDisable(GL_TEXTURE_2D);
                }
            }
			if (state.texture.client!=old_tex)
				glClientActiveTexture(old_tex+GL_TEXTURE0);
		}
#define shift_pointer(a, b) \
	if (state.enable.b && state.pointers.a.buffer) state.pointers.a.pointer -= (uintptr_t)state.pointers.a.buffer->data;
		
			shift_pointer(color, color_array);
			shift_pointer(secondary, secondary_array);
			shift_pointer(vertex, vertex_array);
			for (int aa=0; aa<MAX_TEX; aa++)
				shift_pointer(tex_coord[aa], tex_coord_array[aa]);
			shift_pointer(normal, normal_array);
#undef shift_pointer		
        free(sindices);
    }
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
	if (count<0) {
		errorShim(GL_INVALID_VALUE);
		return;
	}
    if (count==0) {
        noerrorShim();
        return;
    }
	noerrorShim();
	LOAD_GLES(glNormalPointer);
	LOAD_GLES(glVertexPointer);
	LOAD_GLES(glColorPointer);
	LOAD_GLES(glTexCoordPointer);
    LOAD_GLES(glEnable);
    LOAD_GLES(glDisable);
    renderlist_t *list, *active = state.list.active;

    if (active && (state.list.compiling || state.gl_batch)) {
        list = state.list.active;
		NewStage(list, STAGE_DRAW);
        state.list.active = arrays_to_renderlist(list, mode, first, count+first);
        end_renderlist(state.list.active);// = extend_renderlist(list);
        return;
    }

    if (state.polygon_mode == GL_LINE && mode>=GL_TRIANGLES)
		mode = GL_LINE_LOOP;
    if (state.polygon_mode == GL_POINT && mode>=GL_TRIANGLES)
		mode = GL_POINTS;

    if (should_intercept_render(mode)) {
        list = arrays_to_renderlist(NULL, mode, first, count+first);
        end_renderlist(list);
        draw_renderlist(list);
        free_renderlist(list);
    } else {
        // TODO: some draw states require us to use the full pipeline here
        // like texgen, stipple, npot
        LOAD_GLES(glDrawArrays);

#define shift_pointer(a, b) \
	if (state.enable.b && state.pointers.a.buffer) state.pointers.a.pointer = state.pointers.a.buffer->data + (uintptr_t)state.pointers.a.pointer;
	
	shift_pointer(color, color_array);
	shift_pointer(secondary, secondary_array);
	shift_pointer(vertex, vertex_array);
	for (int aa=0; aa<MAX_TEX; aa++) {
		shift_pointer(tex_coord[aa], tex_coord_array[aa]);
    }
	shift_pointer(normal, normal_array);
#undef shift_pointer		

		GLenum mode_init = mode;
		if (mode == GL_QUAD_STRIP)
			mode = GL_TRIANGLE_STRIP;
		if (mode == GL_POLYGON)
			mode = GL_TRIANGLE_FAN;
			
		if (state.render_mode == GL_SELECT) {
			select_glDrawArrays(&state.pointers.vertex, mode, first, count);
		} else {
			// setup the Array Pointers
			// secondary color...
			GLfloat *final_colors = NULL;
			if (/*state.enable.color_sum && */(state.enable.secondary_array) && (state.enable.color_array)) {
				final_colors=copy_gl_pointer_color(&state.pointers.color, 4, 0, count+first, 0);
				GLfloat* seconds_colors=(GLfloat*)copy_gl_pointer(&state.pointers.secondary, 4, first, count+first, 0);
				for (int i=0; i<(count+first)*4; i++)
					final_colors[i]+=seconds_colors[i];
				gles_glColorPointer(4, GL_FLOAT, 0, final_colors);
				free(seconds_colors);
			} else if ((state.enable.color_array && (state.pointers.color.size != 4)) 
                    || (state.enable.color_array && (state.pointers.color.stride!=0) && (state.pointers.color.type != GL_FLOAT))) {
				// Pandora doesn't like Color Pointer with size != 4
				final_colors=copy_gl_pointer_color(&state.pointers.color, 4, 0, count+first, 0);
				gles_glColorPointer(4, GL_FLOAT, 0, final_colors);
			} else if (state.enable.color_array)
				gles_glColorPointer(state.pointers.color.size, state.pointers.color.type, state.pointers.color.stride, state.pointers.color.pointer);
			if (state.enable.normal_array)
				gles_glNormalPointer(state.pointers.normal.type, state.pointers.normal.stride, state.pointers.normal.pointer);
			if (state.enable.vertex_array)
				gles_glVertexPointer(state.pointers.vertex.size, state.pointers.vertex.type, state.pointers.vertex.stride, state.pointers.vertex.pointer);
			GLuint old_tex = state.texture.client;
			for (int aa=0; aa<MAX_TEX; aa++) {
                if (!state.enable.texture_2d[aa] && (state.enable.texture_1d[aa] || state.enable.texture_3d[aa])) {
                    glClientActiveTexture(aa+GL_TEXTURE0);
                    gles_glEnable(GL_TEXTURE_2D);
                }
				if (state.enable.tex_coord_array[aa]) {
                    tex_setup_texcoord(aa, count+first);
					/*glClientActiveTexture(aa+GL_TEXTURE0);
					gles_glTexCoordPointer(state.pointers.tex_coord[aa].size, state.pointers.tex_coord[aa].type, state.pointers.tex_coord[aa].stride, state.pointers.tex_coord[aa].pointer);*/
				}
            }
			if (state.texture.client!=old_tex)
				glClientActiveTexture(old_tex+GL_TEXTURE0);

			if (state.polygon_mode == GL_LINE && mode_init>=GL_TRIANGLES) {
				int n, s;
				switch (mode_init) {
					case GL_TRIANGLES:
						n = 3;
						s = 3;
						break;
					case GL_TRIANGLE_STRIP:
						n = 3;
						s = 1;
						break;
					case GL_TRIANGLE_FAN:	// wrong here...
						n = count;
						s = count;
						break;
					case GL_QUADS:
						n = 4;
						s = 4;
						break;
					case GL_QUAD_STRIP:
						n = 4;
						s = 1;
						break;
					default:		// Polygon and other?
						n = count;
						s = count;
						break;
				}
				for (int i=n; i<count; i+=s)
					gles_glDrawArrays(mode, i-n, n);
			} else
				gles_glDrawArrays(mode, first, count);
			
			// secondary color
			if (final_colors) {
				free(final_colors);
			}
			for (int aa=0; aa<MAX_TEX; aa++) {
                if (!state.enable.texture_2d[aa] && (state.enable.texture_1d[aa] || state.enable.texture_3d[aa])) {
                    glClientActiveTexture(aa+GL_TEXTURE0);
                    gles_glDisable(GL_TEXTURE_2D);
                }
            }
			if (state.texture.client!=old_tex)
				glClientActiveTexture(old_tex+GL_TEXTURE0);
#define shift_pointer(a, b) \
	if (state.enable.b && state.pointers.a.buffer) state.pointers.a.pointer = state.pointers.a.pointer - (uintptr_t)state.pointers.a.buffer->data;
	
		shift_pointer(color, color_array);
		shift_pointer(secondary, secondary_array);
		shift_pointer(vertex, vertex_array);
		for (int aa=0; aa<MAX_TEX; aa++)
			shift_pointer(tex_coord[aa], tex_coord_array[aa]);
		shift_pointer(normal, normal_array);
#undef shift_pointer		
		}
    }
}

#ifndef USE_ES2
#define clone_gl_pointer(t, s)\
    t.size = s; t.type = type; t.stride = stride; t.pointer = pointer; t.buffer = state.buffers.vertex
void glVertexPointer(GLint size, GLenum type,
                     GLsizei stride, const GLvoid *pointer) {
    noerrorShim();
    clone_gl_pointer(state.pointers.vertex, size);
}
void glColorPointer(GLint size, GLenum type,
                     GLsizei stride, const GLvoid *pointer) {
    noerrorShim();
    clone_gl_pointer(state.pointers.color, size);
}
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    noerrorShim();
    clone_gl_pointer(state.pointers.normal, 3);
}
void glTexCoordPointer(GLint size, GLenum type,
                     GLsizei stride, const GLvoid *pointer) {
    noerrorShim();
    clone_gl_pointer(state.pointers.tex_coord[state.texture.client], size);
}
void glSecondaryColorPointer(GLint size, GLenum type, 
					GLsizei stride, const GLvoid *pointer) {
	if (size!=3)
		return;		// Size must be 3...
    clone_gl_pointer(state.pointers.secondary, size);
    noerrorShim();
}

#undef clone_gl_pointer
#endif

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer) {
    uintptr_t ptr = (uintptr_t)pointer;
    // element lengths
    GLsizei tex=0, color=0, normal=0, vert=0;
    // element formats
    GLenum tf, cf, nf, vf;
    tf = cf = nf = vf = GL_FLOAT;
    noerrorShim();
    switch (format) {
        case GL_V2F: vert = 2; break;
        case GL_V3F: vert = 3; break;
        case GL_C4UB_V2F:
            color = 4; cf = GL_UNSIGNED_BYTE;
            vert = 2;
            break;
        case GL_C4UB_V3F:
            color = 4; cf = GL_UNSIGNED_BYTE;
            vert = 3;
            break;
        case GL_C3F_V3F:
            color = 3;
            vert = 4;
            break;
        case GL_N3F_V3F:
            normal = 3;
            vert = 3;
            break;
        case GL_C4F_N3F_V3F:
            color = 4;
            normal = 3;
            vert = 3;
            break;
        case GL_T2F_V3F:
            tex = 2;
            vert = 3;
            break;
        case GL_T4F_V4F:
            tex = 4;
            vert = 4;
            break;
        case GL_T2F_C4UB_V3F:
            tex = 2;
            color = 4; cf = GL_UNSIGNED_BYTE;
            vert = 3;
            break;
        case GL_T2F_C3F_V3F:
            tex = 2;
            color = 3;
            vert = 3;
            break;
        case GL_T2F_N3F_V3F:
            tex = 2;
            normal = 3;
            vert = 3;
            break;
        case GL_T2F_C4F_N3F_V3F:
            tex = 2;
            color = 4;
            normal = 3;
            vert = 3;
            break;
        case GL_T4F_C4F_N3F_V4F:
            tex = 4;
            color = 4;
            normal = 3;
            vert = 4;
            break;
        default:
            errorShim(GL_INVALID_ENUM);
            return;
    }
    if (!stride)
        stride = tex * gl_sizeof(tf) +
                 color * gl_sizeof(cf) +
                 normal * gl_sizeof(nf) +
                 vert * gl_sizeof(vf);
    if (tex) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(tex, tf, stride, (GLvoid *)ptr);
        ptr += tex * gl_sizeof(tf);
    }
    if (color) {
		glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(color, cf, stride, (GLvoid *)ptr);
        ptr += color * gl_sizeof(cf);
    }
    if (normal) {
		glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(nf, stride, (GLvoid *)ptr);
        ptr += normal * gl_sizeof(nf);
    }
    if (vert) {
		glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(vert, vf, stride, (GLvoid *)ptr);
    }
}

// immediate mode functions
void glBegin(GLenum mode) {
    if (!state.list.active)
        state.list.active = alloc_renderlist();
    NewStage(state.list.active, STAGE_DRAW);
    state.list.active->mode = mode;
    state.list.active->mode_init = mode;
    noerrorShim();	// TODO, check Enum validity
}

void glEnd() {
    if (!state.list.active) return;
    // check if TEXTUREx is activate and no TexCoord (or texgen), in that cas, create a dummy one base on state...
    for (int a=0; a<MAX_TEX; a++)
		if (state.enable.texture_2d[a] && ((state.list.active->tex[a]==0) && (!state.enable.texgen_s[a])))
			rlMultiTexCoord2f(state.list.active, GL_TEXTURE0+a, state.texcoord[a][0], state.texcoord[a][1]);
    // render if we're not in a display list
    if (!(state.list.compiling || state.gl_batch)) {
        renderlist_t *mylist = state.list.active;
        state.list.active = NULL;
        end_renderlist(mylist);
        draw_renderlist(mylist);
        free_renderlist(mylist);
    } else {
        state.list.active = extend_renderlist(state.list.active);
    }
    noerrorShim();
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    state.normal[0] = nx; state.normal[1] = ny; state.normal[2] = nz;
    if (state.list.active) {
        if (state.list.active->stage != STAGE_DRAW) {
            if (state.list.active->stage != STAGE_DRAW) {
                PUSH_IF_COMPILING(glNormal3f);
            }
        } else {
            rlNormal3f(state.list.active, nx, ny, nz);
            noerrorShim();
        }
    }
#ifndef USE_ES2
    else {
        LOAD_GLES(glNormal3f);
        gles_glNormal3f(nx, ny, nz);
        errorGL();
    }
#endif
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (state.list.active) {
        rlVertex3f(state.list.active, x, y, z);
        noerrorShim();
    }
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    // change the state first thing
    state.color[0] = red; state.color[1] = green;
    state.color[2] = blue; state.color[3] = alpha;
    if (state.list.active) {
        if (state.list.active->stage != STAGE_DRAW) {
            PUSH_IF_COMPILING(glColor4f);
        }
        rlColor4f(state.list.active, red, green, blue, alpha);
        noerrorShim();
    }
#ifndef USE_ES2
    else {
        LOAD_GLES(glColor4f);
        gles_glColor4f(red, green, blue, alpha);
        errorGL();
    }
#endif
}

void glSecondaryColor3f(GLfloat r, GLfloat g, GLfloat b) {
    // change the state first thing
    state.secondary[0] = r; state.secondary[1] = g;
    state.secondary[2] = b;
    if (state.list.active) {
        rlSecondary3f(state.list.active, r, g, b);
        noerrorShim();
    } else {
        noerrorShim();
    }
}

#ifndef USE_ES2
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    LOAD_GLES(glMaterialfv);
    if ((state.list.compiling || state.gl_batch) && state.list.active) {
		//TODO: Materialfv can be done per vertex, how to handle that ?!
		//NewStage(state.list.active, STAGE_MATERIAL);
        rlMaterialfv(state.list.active, face, pname, params);
        noerrorShim();
    } else {
	    if (face!=GL_FRONT_AND_BACK) {
		    face=GL_FRONT_AND_BACK;
		}
        gles_glMaterialfv(face, pname, params);
        errorGL();
    }
}
void glMaterialf(GLenum face, GLenum pname, const GLfloat param) {
    LOAD_GLES(glMaterialf);
    if ((state.list.compiling || state.gl_batch) && state.list.active) {
		GLfloat params[4];
		memset(params, 0, 4*sizeof(GLfloat));
		params[0] = param;
		NewStage(state.list.active, STAGE_MATERIAL);
        rlMaterialfv(state.list.active, face, pname, params);
        noerrorShim();
    } else {
	    if (face!=GL_FRONT_AND_BACK) {
		    face=GL_FRONT_AND_BACK;
		}
        gles_glMaterialf(face, pname, param);
        errorGL();
    }
}
#endif

void glTexCoord2f(GLfloat s, GLfloat t) {
    state.texcoord[0][0] = s; state.texcoord[0][1] = t;
    if (state.list.active) {
        rlTexCoord2f(state.list.active, s, t);
        noerrorShim();
    } else {
        noerrorShim();
    }
}

void glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t) {
    state.texcoord[target-GL_TEXTURE0][0] = s; state.texcoord[target-GL_TEXTURE0][1] = t;
	// TODO, error if target is unsuported texture....
    if (state.list.active) {
        rlMultiTexCoord2f(state.list.active, target, s, t);
		noerrorShim();
    } else {
        noerrorShim();
    }
}
void glArrayElement(GLint i) {
    GLfloat *v;
    pointer_state_t *p;
    p = &state.pointers.color;
    if (state.enable.color_array) {
        v = gl_pointer_index(p, i);
        GLfloat scale = gl_max_value(p->type);
        // color[3] defaults to 1.0f
        if (p->size < 4)
            v[3] = 1.0f;

        // scale color coordinates to a 0 - 1.0 range
        for (int i = 0; i < p->size; i++) {
            v[i] /= scale;
        }
        glColor4fv(v);
    }
    p = &state.pointers.secondary;
    if (state.enable.secondary_array) {
        v = gl_pointer_index(p, i);
        GLfloat scale = gl_max_value(p->type);
        // color[3] defaults to 0.0f
        if (p->size < 4)
            v[3] = 0.0f;

        // scale color coordinates to a 0 - 1.0 range
        for (int i = 0; i < p->size; i++) {
            v[i] /= scale;
        }
        glSecondaryColor3fv(v);
    }
    p = &state.pointers.normal;
    if (state.enable.normal_array) {
        v = gl_pointer_index(p, i);
        glNormal3fv(v);
    }
    p = &state.pointers.tex_coord[0];
    if (state.enable.tex_coord_array[0]) {
        v = gl_pointer_index(p, i);
        glTexCoord2fv(v);
    }
    int a;
    for (a=1; a<MAX_TEX; a++) {
	    p = &state.pointers.tex_coord[a];
	    if (state.enable.tex_coord_array[a]) {
			v = gl_pointer_index(p, i);
			glMultiTexCoord2fv(GL_TEXTURE0+a, v);
	    }
    }
    p = &state.pointers.vertex;
    if (state.enable.vertex_array) {
        v = gl_pointer_index(p, i);
        if (p->size == 4) {
            glVertex4fv(v);
        } else {
            glVertex3fv(v);
        }
    }
}

// TODO: between a lock and unlock, I can assume the array pointers are unchanged
// so I can build a renderlist_t on the first call and hold onto it
// maybe I need a way to call a renderlist_t with (first, count)
void glLockArraysEXT(GLint first, GLsizei count) {
    state.list.locked = true;
    noerrorShim();
}

void glUnlockArraysEXT() {
    state.list.locked = false;
    noerrorShim();
}

// display lists

static renderlist_t *glGetList(GLuint list) {
    if (glIsList(list))
        return state.lists[list - 1];

    return NULL;
}

GLuint glGenLists(GLsizei range) {
	if (range<0) {
		errorShim(GL_INVALID_VALUE);
		return 0;
	}
	noerrorShim();
    int start = state.list.count;
    if (state.lists == NULL) {
        state.list.cap += range + 100;
        state.lists = malloc(state.list.cap * sizeof(uintptr_t));
    } else if (state.list.count + range > state.list.cap) {
        state.list.cap += range + 100;
        state.lists = realloc(state.lists, state.list.cap * sizeof(uintptr_t));
    }
    state.list.count += range;

    for (int i = 0; i < range; i++) {
        state.lists[start+i] = NULL;
    }
    return start + 1;
}

void glNewList(GLuint list, GLenum mode) {
	errorShim(GL_INVALID_VALUE);
	if (list==0)
		return;
    if (! glIsList(list))
        return;
    noerrorShim();
    if (state.gl_batch) {
        state.gl_batch = 0;
        flush();
    }
    state.list.name = list;
    state.list.mode = mode;
    // TODO: if state.list.active is already defined, we probably need to clean up here
    state.list.active = state.list.first = alloc_renderlist();
    state.list.compiling = true;
}

void glEndList() {
	noerrorShim();
    GLuint list = state.list.name;
    if (state.list.compiling) {
	// Free the previous list if it exist...
        free_renderlist(state.lists[list - 1]);
        state.lists[list - 1] = state.list.first;
        state.list.compiling = false;
        end_renderlist(state.list.active);
        if (gl_batch) {
            init_batch();
        } else state.list.active = NULL;
        if (state.list.mode == GL_COMPILE_AND_EXECUTE) {
            glCallList(list);
        }
    }
}

void glCallList(GLuint list) {
	noerrorShim();
    if ((state.list.compiling || state.gl_batch) && state.list.active) {
		NewStage(state.list.active, STAGE_CALLLIST);
		state.list.active->glcall_list = list;
		return;
	}
    // TODO: the output of this call can be compiled into another display list
    renderlist_t *l = glGetList(list);
    if (l)
        draw_renderlist(l);
}

void glPushCall(void *call) {
    if ((state.list.compiling || state.gl_batch) && state.list.active) {
		NewStage(state.list.active, STAGE_GLCALL);
        rlPushCall(state.list.active, call);
    }
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists) {
    #define call(name, type) \
        case name: glCallList(((type *)lists)[i] + state.list.base); break

    // seriously wtf
    #define call_bytes(name, stride)                             \
        case name:                                               \
            l = (GLubyte *)lists;                                \
            list = 0;                                            \
            for (j = 0; j < stride; j++) {                       \
                list += *(l + (i * stride + j)) << (stride - j); \
            }                                                    \
            glCallList(list + state.list.base);                  \
            break

    unsigned int i, j;
    GLuint list;
    GLubyte *l;
    for (i = 0; i < n; i++) {
        switch (type) {
            call(GL_BYTE, GLbyte);
            call(GL_UNSIGNED_BYTE, GLubyte);
            call(GL_SHORT, GLshort);
            call(GL_UNSIGNED_SHORT, GLushort);
            call(GL_INT, GLint);
            call(GL_UNSIGNED_INT, GLuint);
            call(GL_FLOAT, GLfloat);
            call_bytes(GL_2_BYTES, 2);
            call_bytes(GL_3_BYTES, 3);
            call_bytes(GL_4_BYTES, 4);
        }
    }
    #undef call
    #undef call_bytes
}

void glDeleteList(GLuint list) {
    renderlist_t *l = glGetList(list);
    if (l) {
        free_renderlist(l);
        state.lists[list-1] = NULL;
    }

    // lists just grow upwards, maybe use a better storage mechanism?
}

void glDeleteLists(GLuint list, GLsizei range) {
	noerrorShim();
    for (int i = 0; i < range; i++) {
        glDeleteList(list+i);
    }
}

void glListBase(GLuint base) {
	noerrorShim();
    state.list.base = base;
}

GLboolean glIsList(GLuint list) {
	noerrorShim();
    if (list - 1 < state.list.count) {
        return true;
    }
    return false;
}

void glPolygonMode(GLenum face, GLenum mode) {
	noerrorShim();
	if (face != GL_FRONT_AND_BACK)
		errorShim(GL_INVALID_ENUM);
	if (face == GL_BACK)
		return;		//TODO, handle face enum for polygon mode != GL_FILL
	if ((state.list.compiling || state.gl_batch) && (state.list.active)) {
		NewStage(state.list.active, STAGE_POLYGON);
/*		if (state.list.active->polygon_mode)
			state.list.active = extend_renderlist(state.list.active);*/
		state.list.active->polygon_mode = mode;
		return;
	}
	switch(mode) {
		case GL_LINE:
		case GL_POINT:
			state.polygon_mode = mode;
			break;
		case GL_FILL:
			state.polygon_mode = 0;
			break;
		default:
			state.polygon_mode = 0;
	}
}

void alloc_matrix(matrixstack_t **matrixstack, int depth) {
	*matrixstack = (matrixstack_t*)malloc(sizeof(matrixstack_t));
	(*matrixstack)->top = 0;
	(*matrixstack)->stack = (GLfloat*)malloc(sizeof(GLfloat)*depth*16);
}

void glPushMatrix() {
	PUSH_IF_COMPILING(glPushMatrix);
	LOAD_GLES(glPushMatrix);
	// Alloc matrix stacks if needed
	if (!state.projection_matrix)
		alloc_matrix(&state.projection_matrix, MAX_STACK_PROJECTION);
	if (!state.modelview_matrix)
		alloc_matrix(&state.modelview_matrix, MAX_STACK_MODELVIEW);
	if (!state.texture_matrix) {
		state.texture_matrix = (matrixstack_t**)malloc(sizeof(matrixstack_t*)*MAX_TEX);
		for (int i=0; i<MAX_TEX; i++)
			alloc_matrix(&state.texture_matrix[i], MAX_STACK_TEXTURE);
	}
	// get matrix mode
	GLint matrix_mode;
	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	noerrorShim();
	// go...
	switch(matrix_mode) {
		case GL_PROJECTION:
			if (state.projection_matrix->top<MAX_STACK_PROJECTION) {
				glGetFloatv(GL_PROJECTION_MATRIX, state.projection_matrix->stack+16*state.projection_matrix->top++);
			} else
				errorShim(GL_STACK_OVERFLOW);
			break;
		case GL_MODELVIEW:
			if (state.modelview_matrix->top<MAX_STACK_MODELVIEW) {
				glGetFloatv(GL_MODELVIEW_MATRIX, state.modelview_matrix->stack+16*state.modelview_matrix->top++);
			} else
				errorShim(GL_STACK_OVERFLOW);
			break;
		case GL_TEXTURE:
			if (state.texture_matrix[state.texture.active]->top<MAX_STACK_PROJECTION) {
				glGetFloatv(GL_TEXTURE_MATRIX, state.texture_matrix[state.texture.active]->stack+16*state.texture_matrix[state.texture.active]->top++);
			} else
				errorShim(GL_STACK_OVERFLOW);
			break;
			
		default:
			//Warning?
			errorShim(GL_INVALID_OPERATION);
			//printf("LIBGL: PushMatrix with Unrecognise matrix mode (0x%04X)\n", matrix_mode);
			//gles_glPushMatrix();
	}
}

void glPopMatrix() {
	PUSH_IF_COMPILING(glPopMatrix);
	LOAD_GLES(glPopMatrix);
	// Alloc matrix stacks if needed
	if (!state.projection_matrix)
		alloc_matrix(&state.projection_matrix, MAX_STACK_PROJECTION);
	if (!state.modelview_matrix)
		alloc_matrix(&state.modelview_matrix, MAX_STACK_MODELVIEW);
	if (!state.texture_matrix) {
		state.texture_matrix = (matrixstack_t**)malloc(sizeof(matrixstack_t*)*MAX_TEX);
		for (int i=0; i<MAX_TEX; i++)
			alloc_matrix(&state.texture_matrix[i], MAX_STACK_TEXTURE);
	}
	// get matrix mode
	GLint matrix_mode;
	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	// go...
	noerrorShim();
	switch(matrix_mode) {
		case GL_PROJECTION:
			if (state.projection_matrix->top) {
				glLoadMatrixf(state.projection_matrix->stack+16*--state.projection_matrix->top);
			} else
				errorShim(GL_STACK_UNDERFLOW);
			break;
		case GL_MODELVIEW:
			if (state.modelview_matrix->top) {
				glLoadMatrixf(state.modelview_matrix->stack+16*--state.modelview_matrix->top);
			} else
				errorShim(GL_STACK_UNDERFLOW);
			break;
		case GL_TEXTURE:
			if (state.texture_matrix[state.texture.active]->top) {
				glLoadMatrixf(state.texture_matrix[state.texture.active]->stack+16*--state.texture_matrix[state.texture.active]->top);
			} else
				errorShim(GL_STACK_UNDERFLOW);
			break;
			
		default:
			//Warning?
			errorShim(GL_INVALID_OPERATION);
			//printf("LIBGL: PopMatrix withUnrecognise matrix mode (0x%04X)\n", matrix_mode);
			//gles_glPopMatrix();
	}
}

GLenum glGetError( void) {
	LOAD_GLES(glGetError);
	if (state.shim_error) {
		GLenum tmp = state.last_error;
		state.last_error = GL_NO_ERROR;
		return tmp;
	}
	return gles_glGetError();
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    PUSH_IF_COMPILING(glBlendColor);
    LOAD_GLES_OES(glBlendColor);
	if  (gles_glBlendColor)
		gles_glBlendColor(red, green, blue, alpha);
	else
		printf("stub glBlendColor(%f, %f, %f, %f)\n", red, green, blue, alpha);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    PUSH_IF_COMPILING(glBlendFunc);
    LOAD_GLES(glBlendFunc);
    LOAD_GLES_OES(glBlendFuncSeparate);
    errorGL();
    // There are some limitations in GLES1.1 Blend functions
    switch(sfactor) {
        case GL_SRC_COLOR:
            if (gles_glBlendFuncSeparate) {
                gles_glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
                return;
            }
            sfactor = GL_ONE;   // approx...
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            if (gles_glBlendFuncSeparate) {
                gles_glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
                return;
            }
            sfactor = GL_ONE;  // not sure it make sense...
            break;
        // here, we need support for glBlendColor...
        case GL_CONSTANT_COLOR:
        case GL_CONSTANT_ALPHA:
            sfactor = GL_ONE;
            break;
        case GL_ONE_MINUS_CONSTANT_COLOR:
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            sfactor = GL_ZERO;
            break;
        default:
            break;
    }
    
    switch(dfactor) {
        case GL_DST_COLOR:
            sfactor = GL_ONE;   // approx...
            break;
        case GL_ONE_MINUS_DST_COLOR:
            sfactor = GL_ZERO;  // not sure it make sense...
            break;
        // here, we need support for glBlendColor...
        case GL_CONSTANT_COLOR:
        case GL_CONSTANT_ALPHA:
            sfactor = GL_ONE;
            break;
        case GL_ONE_MINUS_CONSTANT_COLOR:
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            sfactor = GL_ZERO;
            break;
        default:
            break;
    }
    
    if ((sfactor==GL_SRC_ALPHA) && (dfactor==GL_ONE)) {
        // special case, as seen in Xash3D
        sfactor = GL_ONE;
    }
    
    gles_glBlendFunc(sfactor, dfactor);
}

void flush() {
    // flush internal list
//printf("flush state.list.active=%p, gl_batch=%i(%i)\n", state.list.active, state.gl_batch, gl_batch);
    renderlist_t *mylist = state.list.active;
    if (mylist) {
        GLuint old = state.gl_batch;
        state.list.active = NULL;
        state.gl_batch = 0;
        end_renderlist(mylist);
        draw_renderlist(mylist);
        free_renderlist(mylist);
        state.gl_batch = old;
    }
    state.list.active = (state.gl_batch)?alloc_renderlist():NULL;
}

void init_batch() {
    state.list.active = alloc_renderlist();
    state.gl_batch = 1;
}

void glFlush() {
	LOAD_GLES(glFlush);
    
    if (state.list.active && !state.gl_batch) {
        errorShim(GL_INVALID_OPERATION);
        return;
    }
    
    if (state.gl_batch) flush();
    
    gles_glFlush();
    errorGL();
}
void glFinish() {
	LOAD_GLES(glFinish);
    
    if (state.list.active && !state.gl_batch) {
        errorShim(GL_INVALID_OPERATION);
        return;
    }
    if (state.gl_batch) flush();
    
    gles_glFinish();
    errorGL();
}

void glLoadMatrixf(const GLfloat * m) {
    LOAD_GLES(glLoadMatrixf);
    
    if ((state.list.active || state.gl_batch) && state.list.active) {
        NewStage(state.list.active, STAGE_MATRIX);
        state.list.active->matrix_op = 1;
        memcpy(state.list.active->matrix_val, m, 16*sizeof(GLfloat));
        return;
    }
    gles_glLoadMatrixf(m);
}

void glMultMatrixf(const GLfloat * m) {
    LOAD_GLES(glMultMatrixf);
    
    if ((state.list.active || state.gl_batch) && state.list.active) {
        NewStage(state.list.active, STAGE_MATRIX);
        state.list.active->matrix_op = 2;
        memcpy(state.list.active->matrix_val, m, 16*sizeof(GLfloat));
        return;
    }
    gles_glMultMatrixf(m);
}

void glFogfv(GLenum pname, const GLfloat* params) {
    LOAD_GLES(glFogfv);

    if ((state.list.active || state.gl_batch) && state.list.active) {
        if (pname == GL_FOG_COLOR) {
            NewStage(state.list.active, STAGE_FOG);
            rlFogOp(state.list.active, 1, params);
            return;
        }
    }
    PUSH_IF_COMPILING(glFogfv);
    
    gles_glFogfv(pname, params);
}
