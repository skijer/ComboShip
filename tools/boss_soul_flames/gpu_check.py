"""Headless OpenGL check of the shipped flame alpha combiner and wrap stages.

This renders the native flame texture domain, not a game scene. No art is edited.
Only ctypes, NumPy, Mesa/EGL and desktop OpenGL are required.
"""
import ctypes as C
import numpy as np


class FlameGPU:
    def __init__(self, width=256, height=512):
        self.width, self.height = width, height
        self.egl = C.CDLL('libEGL.so.1')
        self.gl = C.CDLL('libGL.so.1')
        self.e = {}
        self.g = {}
        def egl(name, result, *args):
            f = getattr(self.egl, name)
            f.restype, f.argtypes = result, list(args)
            self.e[name] = f
            return f
        def gl(name, result, *args):
            f = getattr(self.gl, name)
            f.restype, f.argtypes = result, list(args)
            self.g[name] = f
            return f
        ptr, integer, uint = C.c_void_p, C.c_int, C.c_uint
        get = egl('eglGetProcAddress', ptr, C.c_char_p)
        platform = C.CFUNCTYPE(ptr, uint, ptr, C.POINTER(integer))(get(b'eglGetPlatformDisplayEXT'))
        self.display = platform(0x31DD, None, None)
        init = egl('eglInitialize', uint, ptr, C.POINTER(integer), C.POINTER(integer))
        major, minor = integer(), integer()
        assert init(self.display, C.byref(major), C.byref(minor)), 'EGL initialization failed'
        assert egl('eglBindAPI', uint, uint)(0x30A2)
        attrs = (integer * 13)(0x3033, 1, 0x3040, 8, 0x3024, 8, 0x3023, 8, 0x3022, 8, 0x3021, 8, 0x3038)
        cfg, count = ptr(), integer()
        choose = egl('eglChooseConfig', uint, ptr, C.POINTER(integer), C.POINTER(ptr), integer, C.POINTER(integer))
        assert choose(self.display, attrs, C.byref(cfg), 1, C.byref(count)) and count.value
        self.context = egl('eglCreateContext', ptr, ptr, ptr, ptr, C.POINTER(integer))(
            self.display, cfg, None, (integer * 1)(0x3038))
        self.surface = egl('eglCreatePbufferSurface', ptr, ptr, ptr, C.POINTER(integer))(
            self.display, cfg, (integer * 5)(0x3057, width, 0x3056, height, 0x3038))
        assert self.context and self.surface
        assert egl('eglMakeCurrent', uint, ptr, ptr, ptr, ptr)(
            self.display, self.surface, self.surface, self.context)
        get_string = gl('glGetString', C.c_char_p, uint)
        self.renderer = get_string(0x1F01).decode()
        gl('glCreateShader', uint, uint)
        gl('glShaderSource', None, uint, integer, C.POINTER(C.c_char_p), C.POINTER(integer))
        gl('glCompileShader', None, uint)
        gl('glGetShaderiv', None, uint, uint, C.POINTER(integer))
        gl('glGetShaderInfoLog', None, uint, integer, C.POINTER(integer), C.c_char_p)
        gl('glCreateProgram', uint)
        gl('glAttachShader', None, uint, uint)
        gl('glLinkProgram', None, uint)
        gl('glGetProgramiv', None, uint, uint, C.POINTER(integer))
        gl('glGetProgramInfoLog', None, uint, integer, C.POINTER(integer), C.c_char_p)
        gl('glUseProgram', None, uint)
        gl('glGetUniformLocation', integer, uint, C.c_char_p)
        gl('glUniform1i', None, integer, integer)
        gl('glUniform1f', None, integer, C.c_float)
        gl('glUniform2f', None, integer, C.c_float, C.c_float)
        gl('glGenTextures', None, integer, C.POINTER(uint))
        gl('glActiveTexture', None, uint)
        gl('glBindTexture', None, uint, uint)
        gl('glTexParameteri', None, uint, uint, integer)
        gl('glTexImage2D', None, uint, integer, integer, integer, integer, integer, uint, uint, ptr)
        gl('glViewport', None, integer, integer, integer, integer)
        gl('glDrawArrays', None, uint, integer, integer)
        gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)
        gl('glGetError', uint)
        gl('glDisable', None, uint)
        for cap in (0x0BE2, 0x0BD0, 0x0B71):
            self.g['glDisable'](cap)  # blend, dither, depth
        vertex = '''#version 330
        void main() {
            vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
            gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
        }'''
        fragment = '''#version 330
        uniform sampler2D envelopeTex;
        uniform sampler2D movingTex;
        uniform vec2 outputSize;
        uniform vec2 scroll;
        uniform float sDivisor;
        uniform float lod;
        uniform int filterMode;
        out vec4 color;
        #define WRAP(x, low, high) mod((x)-(low), (high)-(low)) + (low)
        vec4 filtered(sampler2D tex, vec2 uv) {
            if (filterMode != 2) return texture(tex, uv);
            vec2 size = vec2(textureSize(tex, 0));
            vec2 offset = fract(uv * size - vec2(0.5));
            offset -= step(1.0, offset.x + offset.y);
            vec4 c0 = texture(tex, uv - offset / size);
            vec4 c1 = texture(tex, uv - vec2(offset.x - sign(offset.x), offset.y) / size);
            vec4 c2 = texture(tex, uv - vec2(offset.x, offset.y - sign(offset.y)) / size);
            return c0 + abs(offset.x) * (c1 - c0) + abs(offset.y) * (c2 - c0);
        }
        void main() {
            vec2 logical = gl_FragCoord.xy / outputSize * vec2(16.0, 32.0);
            vec2 baseUv = (logical + vec2(0.5)) / vec2(16.0, 32.0);
            vec2 movingUv = (vec2(logical.x / sDivisor, logical.y * 2.0)
                             - scroll + vec2(0.5)) / vec2(16.0, 32.0);
            float envelope = filtered(envelopeTex, baseUv).a;
            float moving = filtered(movingTex, movingUv).a;
            float alpha = (moving - 1.0) * lod + envelope;
            // Native FC272C60 35FCE378: cycle 1 passes COMBINED alpha.
            // These two WRAP stages are in the production GLSL/HLSL/Metal shaders.
            alpha = WRAP(alpha, -0.51, 1.51);
            alpha = WRAP(alpha, -0.51, 1.51);
            alpha = clamp(alpha, 0.0, 1.0);
            color = vec4(alpha, envelope, 0.0, 1.0);
        }'''
        def shader(kind, source):
            s = self.g['glCreateShader'](kind)
            text = C.c_char_p(source.encode())
            self.g['glShaderSource'](s, 1, C.byref(text), None)
            self.g['glCompileShader'](s)
            ok = integer()
            self.g['glGetShaderiv'](s, 0x8B81, C.byref(ok))
            if not ok.value:
                log = C.create_string_buffer(4096)
                self.g['glGetShaderInfoLog'](s, 4096, None, log)
                raise AssertionError(log.value.decode())
            return s
        self.program = self.g['glCreateProgram']()
        for kind, source in ((0x8B31, vertex), (0x8B30, fragment)):
            self.g['glAttachShader'](self.program, shader(kind, source))
        self.g['glLinkProgram'](self.program)
        ok = integer()
        self.g['glGetProgramiv'](self.program, 0x8B82, C.byref(ok))
        if not ok.value:
            log = C.create_string_buffer(4096)
            self.g['glGetProgramInfoLog'](self.program, 4096, None, log)
            raise AssertionError(log.value.decode())
        self.g['glUseProgram'](self.program)
        self.uniform = {name: self.g['glGetUniformLocation'](self.program, name.encode())
                        for name in ('envelopeTex', 'movingTex', 'outputSize', 'scroll', 'sDivisor', 'lod', 'filterMode')}
        self.g['glUniform1i'](self.uniform['envelopeTex'], 0)
        self.g['glUniform1i'](self.uniform['movingTex'], 1)
        self.g['glUniform2f'](self.uniform['outputSize'], width, height)
        self.g['glViewport'](0, 0, width, height)
        self.textures = (uint * 2)()
        self.g['glGenTextures'](2, self.textures)
        self.pixels = np.empty((height, width, 4), dtype=np.uint8)

    def upload(self, envelope, moving, filter_mode):
        self.g['glUniform1i'](self.uniform['filterMode'], filter_mode)
        for i, data in enumerate((envelope, moving)):
            data = np.ascontiguousarray(data)
            self.g['glActiveTexture'](0x84C0 + i)
            self.g['glBindTexture'](0x0DE1, self.textures[i])
            for prop in (0x2801, 0x2800):
                self.g['glTexParameteri'](0x0DE1, prop, 0x2601 if filter_mode == 1 else 0x2600)
            for prop in (0x2802, 0x2803):
                self.g['glTexParameteri'](0x0DE1, prop, 0x812F if i == 0 else 0x2901)
            self.g['glTexImage2D'](0x0DE1, 0, 0x8058, data.shape[1], data.shape[0], 0,
                                     0x1908, 0x1401, C.c_void_p(data.ctypes.data))

    def render(self, owner, phase):
        if owner == 'MM':
            lod, divisor, xs, ys = 192, 2, 2, -6
        else:
            lod, divisor, xs, ys = 128, 1, 1, -8
        self.g['glUniform1f'](self.uniform['lod'], lod / 255)
        self.g['glUniform1f'](self.uniform['sDivisor'], divisor)
        self.g['glUniform2f'](self.uniform['scroll'], (phase * xs) % 64 / 4, (phase * ys) % 128 / 4)
        self.g['glDrawArrays'](0x0004, 0, 3)
        self.g['glReadPixels'](0, 0, self.width, self.height, 0x1908, 0x1401,
                                 C.c_void_p(self.pixels.ctypes.data))
        assert self.g['glGetError']() == 0
        return self.pixels.copy()

    def close(self):
        self.e['eglMakeCurrent'](self.display, None, None, None)
        for name, handle in (('eglDestroySurface', self.surface), ('eglDestroyContext', self.context)):
            f = getattr(self.egl, name)
            f.argtypes, f.restype = [C.c_void_p, C.c_void_p], C.c_uint
            assert f(self.display, handle)
        self.egl.eglTerminate.argtypes = [C.c_void_p]
        self.egl.eglTerminate(self.display)
