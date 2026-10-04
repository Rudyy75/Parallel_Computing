#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#define LOCAL_SIZE 16
#define HEALTHY 0u
#define BURNING 1u
#define NOTHING 2u

typedef struct {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
} gpu_context_t;

static void fail_egl(const char *operation) {
    fprintf(stderr, "%s failed: EGL error 0x%04x\n",
            operation, eglGetError());
    exit(EXIT_FAILURE);
}

static void fail_gl(const char *operation) {
    fprintf(stderr, "%s failed: OpenGL ES error 0x%04x\n",
            operation, glGetError());
    exit(EXIT_FAILURE);
}

static char *read_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long file_size;
    char *contents;
    size_t bytes_read;

    if (file == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    file_size = ftell(file);
    if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        perror("ftell");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    contents = malloc((size_t)file_size + 1);
    if (contents == NULL) {
        perror("malloc");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    bytes_read = fread(contents, 1, (size_t)file_size, file);
    fclose(file);
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "could not read complete shader file\n");
        free(contents);
        exit(EXIT_FAILURE);
    }
    contents[file_size] = '\0';
    return contents;
}

static GLuint compile_compute_shader(const char *path) {
    char *source = read_text_file(path);
    const GLchar *source_pointer = source;
    GLint compiled;
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

    if (shader == 0) {
        free(source);
        fail_gl("glCreateShader");
    }

    glShaderSource(shader, 1, &source_pointer, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint log_length = 0;
        char *log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        log = calloc((size_t)log_length + 1, 1);
        if (log != NULL) {
            glGetShaderInfoLog(shader, log_length, NULL, log);
            fprintf(stderr, "compute shader compilation failed:\n%s\n", log);
            free(log);
        }
        free(source);
        glDeleteShader(shader);
        exit(EXIT_FAILURE);
    }

    free(source);
    return shader;
}

static GLuint link_program(GLuint shader) {
    GLint linked;
    GLuint program = glCreateProgram();
    if (program == 0) {
        fail_gl("glCreateProgram");
    }

    glAttachShader(program, shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint log_length = 0;
        char *log;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        log = calloc((size_t)log_length + 1, 1);
        if (log != NULL) {
            glGetProgramInfoLog(program, log_length, NULL, log);
            fprintf(stderr, "compute program link failed:\n%s\n", log);
            free(log);
        }
        glDeleteProgram(program);
        exit(EXIT_FAILURE);
    }

    return program;
}

static gpu_context_t create_context(void) {
    gpu_context_t gpu = {0};
    EGLint major;
    EGLint minor;
    EGLConfig config;
    EGLint config_count;
    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_NONE,
    };
    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE,
    };
    const char *extensions;
    bool surfaceless;

    gpu.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (gpu.display == EGL_NO_DISPLAY ||
        eglInitialize(gpu.display, &major, &minor) != EGL_TRUE) {
        fail_egl("EGL initialization");
    }

    extensions = eglQueryString(gpu.display, EGL_EXTENSIONS);
    surfaceless = extensions != NULL &&
                  strstr(extensions, "EGL_KHR_surfaceless_context") != NULL;
    if (eglChooseConfig(gpu.display, config_attributes, &config, 1,
                        &config_count) != EGL_TRUE || config_count == 0) {
        fail_egl("eglChooseConfig");
    }
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        fail_egl("eglBindAPI");
    }

    gpu.context = eglCreateContext(
        gpu.display, config, EGL_NO_CONTEXT, context_attributes);
    if (gpu.context == EGL_NO_CONTEXT) {
        fail_egl("eglCreateContext");
    }

    gpu.surface = EGL_NO_SURFACE;
    if (!surfaceless) {
        const EGLint surface_attributes[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE,
        };
        gpu.surface = eglCreatePbufferSurface(
            gpu.display, config, surface_attributes);
        if (gpu.surface == EGL_NO_SURFACE) {
            fail_egl("eglCreatePbufferSurface");
        }
    }

    if (eglMakeCurrent(gpu.display, gpu.surface, gpu.surface,
                       gpu.context) != EGL_TRUE) {
        fail_egl("eglMakeCurrent");
    }
    return gpu;
}

static void destroy_context(gpu_context_t *gpu) {
    eglMakeCurrent(gpu->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    if (gpu->surface != EGL_NO_SURFACE) {
        eglDestroySurface(gpu->display, gpu->surface);
    }
    eglDestroyContext(gpu->display, gpu->context);
    eglTerminate(gpu->display);
}

static void print_grid(const uint32_t *cells, size_t size) {
    for (size_t row = 0; row < size; row++) {
        for (size_t column = 0; column < size; column++) {
            const uint32_t state = cells[row * size + column];
            putchar(state == HEALTHY ? 'H' :
                    state == BURNING ? 'B' : 'N');
        }
        putchar('\n');
    }
}

static size_t count_burning(const uint32_t *cells, size_t cell_count) {
    size_t burning_count = 0;

    for (size_t index = 0; index < cell_count; index++) {
        if (cells[index] == BURNING) {
            burning_count++;
        }
    }

    return burning_count;
}

int main(int argc, char **argv) {
    const size_t size = argc > 1 ? (size_t)strtoul(argv[1], NULL, 10) : 16;
    const size_t cell_count = size * size;
    const size_t byte_count = cell_count * sizeof(uint32_t);
    uint32_t *input_cells;
    gpu_context_t gpu;
    GLuint shader;
    GLuint program;
    GLuint buffers[2];
    GLint grid_size_location;
    GLint epoch_location;
    GLuint input_buffer = 0;
    GLuint output_buffer = 1;
    size_t epochs = 0;
    size_t burning_count = 1;
    const size_t maximum_epochs = 100000;

    if (size == 0 || size > 4096) {
        fprintf(stderr, "grid size must be between 1 and 4096\n");
        return EXIT_FAILURE;
    }

    input_cells = calloc(cell_count, sizeof(*input_cells));
    if (input_cells == NULL) {
        perror("calloc");
        free(input_cells);
        return EXIT_FAILURE;
    }
    input_cells[(size / 2) * size + size / 2] = BURNING;

    gpu = create_context();
    shader = compile_compute_shader("forest_fire.comp");
    program = link_program(shader);
    glDeleteShader(shader);

    glGenBuffers(2, buffers);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)byte_count,
                 input_cells, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[0]);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)byte_count,
                 NULL, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1]);

    glUseProgram(program);
    grid_size_location = glGetUniformLocation(program, "grid_size");
    epoch_location = glGetUniformLocation(program, "epoch");
    glUniform1ui(grid_size_location, (GLuint)size);

    while (burning_count != 0 && epochs < maximum_epochs) {
        uint32_t *output_cells;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[input_buffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[output_buffer]);
        glUniform1ui(epoch_location, (GLuint)epochs);
        glDispatchCompute((GLuint)((size + LOCAL_SIZE - 1) / LOCAL_SIZE),
                          (GLuint)((size + LOCAL_SIZE - 1) / LOCAL_SIZE), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[output_buffer]);
        output_cells = glMapBufferRange(
            GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)byte_count,
            GL_MAP_READ_BIT);
        if (output_cells == NULL) {
            fail_gl("glMapBufferRange");
        }

        epochs++;
        burning_count = count_burning(output_cells, cell_count);
        if (size <= 20) {
            printf("Epoch %zu, burning cells: %zu\n", epochs, burning_count);
            print_grid(output_cells, size);
        }

        if (glUnmapBuffer(GL_SHADER_STORAGE_BUFFER) != GL_TRUE) {
            fail_gl("glUnmapBuffer");
        }

        {
            const GLuint temporary = input_buffer;
            input_buffer = output_buffer;
            output_buffer = temporary;
        }
    }

    if (burning_count != 0) {
        fprintf(stderr, "simulation exceeded %zu epochs\n", maximum_epochs);
        glDeleteBuffers(2, buffers);
        glDeleteProgram(program);
        destroy_context(&gpu);
        free(input_cells);
        return EXIT_FAILURE;
    }

    printf("Fire extinguished after %zu epochs for M=%zu\n", epochs, size);

    glDeleteBuffers(2, buffers);
    glDeleteProgram(program);
    destroy_context(&gpu);
    free(input_cells);
    return EXIT_SUCCESS;
}
