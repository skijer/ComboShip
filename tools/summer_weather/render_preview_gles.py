"""Render the preview's captured WebGL draws with a real offscreen GLES context."""
import argparse
import ctypes as c
import ctypes.util
import json
import os
from pathlib import Path
from PIL import Image, ImageChops

def function(lib, name, result, *args):
    value = getattr(lib, name)
    value.restype = result
    value.argtypes = args
    return value

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('capture', type=Path)
    parser.add_argument('background', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--gles-library')
    args = parser.parse_args()
    os.environ.setdefault('EGL_PLATFORM', 'surfaceless')
    egl = c.CDLL(ctypes.util.find_library('EGL'))
    gl = c.CDLL(args.gles_library or ctypes.util.find_library('GLESv2'))
    data = json.loads(args.capture.read_text())
    width, height = data['modes'][0]['width'], data['modes'][0]['height']
    display = function(egl, 'eglGetDisplay', c.c_void_p, c.c_void_p)(None)
    major, minor = c.c_int(), c.c_int()
    assert function(egl, 'eglInitialize', c.c_uint, c.c_void_p, c.POINTER(c.c_int), c.POINTER(c.c_int))(display, c.byref(major), c.byref(minor))
    assert function(egl, 'eglBindAPI', c.c_uint, c.c_uint)(0x30A0)
    attributes = (c.c_int * 15)(0x3033, 1, 0x3040, 4, 0x3024, 8, 0x3023, 8, 0x3022, 8, 0x3021, 8, 0x3038, 0, 0)
    config, count = c.c_void_p(), c.c_int()
    assert function(egl, 'eglChooseConfig', c.c_uint, c.c_void_p, c.POINTER(c.c_int), c.POINTER(c.c_void_p), c.c_int, c.POINTER(c.c_int))(display, attributes, c.byref(config), 1, c.byref(count))
    surface_attributes = (c.c_int * 5)(0x3057, width, 0x3056, height, 0x3038)
    surface = function(egl, 'eglCreatePbufferSurface', c.c_void_p, c.c_void_p, c.c_void_p, c.POINTER(c.c_int))(display, config, surface_attributes)
    context_attributes = (c.c_int * 3)(0x3098, 2, 0x3038)
    context = function(egl, 'eglCreateContext', c.c_void_p, c.c_void_p, c.c_void_p, c.c_void_p, c.POINTER(c.c_int))(display, config, None, context_attributes)
    assert function(egl, 'eglMakeCurrent', c.c_uint, c.c_void_p, c.c_void_p, c.c_void_p, c.c_void_p)(display, surface, surface, context)

    create_shader = function(gl, 'glCreateShader', c.c_uint, c.c_uint)
    shader_source = function(gl, 'glShaderSource', None, c.c_uint, c.c_int, c.POINTER(c.c_char_p), c.POINTER(c.c_int))
    compile_shader = function(gl, 'glCompileShader', None, c.c_uint)
    shader_status = function(gl, 'glGetShaderiv', None, c.c_uint, c.c_uint, c.POINTER(c.c_int))
    program = function(gl, 'glCreateProgram', c.c_uint)()
    for source in data['shaders']:
        shader = create_shader(source['type'])
        text = c.c_char_p(source['source'].encode())
        shader_source(shader, 1, c.byref(text), None)
        compile_shader(shader)
        status = c.c_int()
        shader_status(shader, 0x8B81, c.byref(status))
        assert status.value, 'Preview shader failed to compile'
        function(gl, 'glAttachShader', None, c.c_uint, c.c_uint)(program, shader)
    function(gl, 'glLinkProgram', None, c.c_uint)(program)
    linked = c.c_int()
    function(gl, 'glGetProgramiv', None, c.c_uint, c.c_uint, c.POINTER(c.c_int))(program, 0x8B82, c.byref(linked))
    assert linked.value
    function(gl, 'glUseProgram', None, c.c_uint)(program)
    gen_buffer = function(gl, 'glGenBuffers', None, c.c_int, c.POINTER(c.c_uint))
    buffer = c.c_uint()
    gen_buffer(1, c.byref(buffer))
    function(gl, 'glBindBuffer', None, c.c_uint, c.c_uint)(0x8892, buffer)
    location = function(gl, 'glGetAttribLocation', c.c_int, c.c_uint, c.c_char_p)
    for name, size, offset in ((b'aPosition', 4, 0), (b'aUv', 2, 16), (b'aColor', 4, 24)):
        index = location(program, name)
        function(gl, 'glEnableVertexAttribArray', None, c.c_uint)(index)
        function(gl, 'glVertexAttribPointer', None, c.c_uint, c.c_int, c.c_uint, c.c_ubyte, c.c_int, c.c_void_p)(index, size, 0x1406, 0, 40, c.c_void_p(offset))
    textures = {}
    bind_texture = function(gl, 'glBindTexture', None, c.c_uint, c.c_uint)
    for item in data['textures']:
        texture = c.c_uint()
        function(gl, 'glGenTextures', None, c.c_int, c.POINTER(c.c_uint))(1, c.byref(texture))
        bind_texture(0x0DE1, texture)
        for option, value in ((0x2801, 0x2601), (0x2800, 0x2601), (0x2802, 0x812F), (0x2803, 0x812F)):
            function(gl, 'glTexParameteri', None, c.c_uint, c.c_uint, c.c_int)(0x0DE1, option, value)
        if item.get('photo'):
            image = Image.open(args.background).convert('RGBA').transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            w, h = image.size
            pixels = image.tobytes()
        else:
            w, h, pixels = item['width'], item['height'], bytes(item['rgba'])
        raw = (c.c_ubyte * len(pixels)).from_buffer_copy(pixels)
        function(gl, 'glTexImage2D', None, c.c_uint, c.c_int, c.c_int, c.c_int, c.c_int, c.c_int, c.c_uint, c.c_uint, c.c_void_p)(0x0DE1, 0, 0x1908, w, h, 0, 0x1908, 0x1401, raw)
        textures[item['id']] = texture.value
    uniform_location = function(gl, 'glGetUniformLocation', c.c_int, c.c_uint, c.c_char_p)
    uniform = function(gl, 'glUniform1i', None, c.c_int, c.c_int)
    uniform(uniform_location(program, b'uTexture'), 0)
    textured = uniform_location(program, b'uTextured')
    args.output.mkdir(parents=True, exist_ok=True)
    for mode in data['modes']:
        function(gl, 'glViewport', None, c.c_int, c.c_int, c.c_int, c.c_int)(0, 0, width, height)
        function(gl, 'glClearColor', None, c.c_float, c.c_float, c.c_float, c.c_float)(0, 0, 0, 1)
        function(gl, 'glClear', None, c.c_uint)(0x4000)
        for index, batch in enumerate(mode['draws']):
            function(gl, 'glEnable' if index else 'glDisable', None, c.c_uint)(0x0BE2)
            function(gl, 'glBlendFunc', None, c.c_uint, c.c_uint)(0x0302, 0x0303)
            uniform(textured, int(batch['texture'] is not None))
            if batch['texture'] is not None:
                bind_texture(0x0DE1, textures[batch['texture']])
            vertices = (c.c_float * len(batch['vertices']))(*batch['vertices'])
            function(gl, 'glBufferData', None, c.c_uint, c.c_ssize_t, c.c_void_p, c.c_uint)(0x8892, c.sizeof(vertices), vertices, 0x88E8)
            function(gl, 'glDrawArrays', None, c.c_uint, c.c_int, c.c_int)(4, 0, len(vertices) // 10)
        pixels = (c.c_ubyte * (width * height * 4))()
        function(gl, 'glReadPixels', None, c.c_int, c.c_int, c.c_int, c.c_int, c.c_uint, c.c_uint, c.c_void_p)(0, 0, width, height, 0x1908, 0x1401, pixels)
        assert function(gl, 'glGetError', c.c_uint)() == 0
        Image.frombytes('RGBA', (width, height), bytes(pixels)).transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(args.output / (mode['name'] + '.png'))
    a = Image.open(args.output / 'day-beams.png').convert('RGB')
    b = Image.open(args.output / 'day-clear.png').convert('RGB')
    assert ImageChops.difference(a, b).getbbox(), 'Sunbeam switch produced identical images'
    print('PASS real GLES: actual preview shaders compile/link, captured draws render, sunbeams change pixels')

if __name__ == '__main__':
    main()
