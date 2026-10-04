#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

static void fail_egl(const char *operation) {
    fprintf(stderr, "%s failed: EGL error 0x%04x\n",
            operation, eglGetError());
    exit(EXIT_FAILURE);
}

int main(void) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        fail_egl("eglGetDisplay");
    }

    EGLint major;
    EGLint minor;
    if (eglInitialize(display, &major, &minor) != EGL_TRUE) {
        fail_egl("eglInitialize");
    }

    const char *egl_extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (egl_extensions == NULL) {
        fail_egl("eglQueryString");
    }

    const EGLBoolean has_surfaceless =
        strstr(egl_extensions, "EGL_KHR_surfaceless_context") != NULL;

    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_NONE,
    };
    EGLConfig config;
    EGLint config_count;
    if (eglChooseConfig(display, config_attributes, &config, 1,
                        &config_count) != EGL_TRUE || config_count == 0) {
        fail_egl("eglChooseConfig");
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        fail_egl("eglBindAPI");
    }

    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE,
    };
    EGLContext context = eglCreateContext(
        display, config, EGL_NO_CONTEXT, context_attributes);
    if (context == EGL_NO_CONTEXT) {
        fail_egl("eglCreateContext");
    }

    EGLSurface surface = EGL_NO_SURFACE;
    if (!has_surfaceless) {
        const EGLint surface_attributes[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE,
        };
        surface = eglCreatePbufferSurface(
            display, config, surface_attributes);
        if (surface == EGL_NO_SURFACE) {
            fail_egl("eglCreatePbufferSurface");
        }
    }

    if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) {
        fail_egl("eglMakeCurrent");
    }

    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    if (version == NULL || vendor == NULL) {
        fprintf(stderr, "glGetString failed\n");
        return EXIT_FAILURE;
    }

    printf("EGL version: %s\n", eglQueryString(display, EGL_VERSION));
    printf("EGL client APIs: %s\n", eglQueryString(display, EGL_CLIENT_APIS));
    printf("EGL_KHR_surfaceless_context: %s\n",
           has_surfaceless ? "yes" : "no, using a pbuffer");
    printf("OpenGL ES version: %s\n", version);
    printf("OpenGL ES vendor: %s\n", vendor);

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surface != EGL_NO_SURFACE) {
        eglDestroySurface(display, surface);
    }
    eglDestroyContext(display, context);
    eglTerminate(display);
    return EXIT_SUCCESS;
}
