import re
import sys
import os
import glob

def get_implemented_funcs(src_dir):
    funcs = set()
    for c_file in glob.glob(os.path.join(src_dir, "*.c")):
        with open(c_file, 'r') as f:
            data = f.read()
        matches = re.findall(r'GL_API\s+[\w\s\*]+\s*GL_APIENTRY\s+(gl\w+)\s*\(', data)
        for m in matches:
            funcs.add(m)
    return funcs

def main():
    src_dir = sys.argv[1]
    gl_h = os.path.join(sys.argv[2], 'gl.h')
    glext_h = os.path.join(sys.argv[2], 'glext.h')
    
    implemented = get_implemented_funcs(src_dir)
    funcs = sorted(list(implemented))
    
    prefix_h = sys.argv[3]
    shim_c = sys.argv[4]
    
    os.makedirs(os.path.dirname(os.path.abspath(prefix_h)), exist_ok=True)
    with open(prefix_h, 'w') as f:
        f.write("#ifndef NXDK_GL_PREFIX_H\n")
        f.write("#define NXDK_GL_PREFIX_H\n\n")
        f.write("/* Private to nxdk-gles11. Renames ES1.1 entry points to nxdk_glFoo\n")
        f.write(" * so they can coexist with gl4es' desktop GL ones without corrupting\n")
        f.write(" * both layers' matrix/state stacks. */\n\n")
        for func in funcs:
            f.write(f"#define {func} nxdk_{func}\n")
        f.write("\n#endif\n")
        
    with open(shim_c, 'w') as f:
        f.write("#include <string.h>\n")
        f.write("#define GL_GLEXT_PROTOTYPES\n")
        f.write("#include <GLES/gl.h>\n")
        f.write("#include <GLES/glext.h>\n\n")
        f.write("#include <stdlib.h>\n")
        f.write("struct proc_map {\n")
        f.write("    const char *name;\n")
        f.write("    void *ptr;\n")
        f.write("};\n\n")
        f.write("static const struct proc_map funcs[] = {\n")
        for func in funcs:
            f.write(f'    {{"{func}", (void*)nxdk_{func}}},\n')
        f.write("};\n\n")
        f.write("static int cmp_proc(const void *a, const void *b) {\n")
        f.write("    return strcmp((const char *)a, ((const struct proc_map *)b)->name);\n")
        f.write("}\n\n")
        f.write("void* __stdcall nxdk_gl4es_getprocaddress(const char* name) {\n")
        f.write("    struct proc_map *res = bsearch(name, funcs, sizeof(funcs)/sizeof(funcs[0]), sizeof(funcs[0]), cmp_proc);\n")
        f.write("    if (res) return res->ptr;\n")
        f.write("    return NULL;\n")
        f.write("}\n\n")
        
        f.write("double atof(const char *nptr) {\n")
        f.write("    return strtod(nptr, NULL);\n")
        f.write("}\n\n")

        f.write("void __stdcall KeQuerySystemTime(void*);\n")
        f.write("void __stdcall GetSystemTimeAsFileTime(void* p) {\n")
        f.write("    KeQuerySystemTime(p);\n")
        f.write("}\n")
        f.write("void* __imp__GetSystemTimeAsFileTime __asm__(\"__imp__GetSystemTimeAsFileTime@4\") = &GetSystemTimeAsFileTime;\n")

if __name__ == '__main__':
    main()
