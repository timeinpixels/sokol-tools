/*
    Generate the output header for sokol_gfx.h
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

static const char* uniform_type_str(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT: return "float";
        case uniform_t::FLOAT2: return "vec2";
        case uniform_t::FLOAT3: return "vec3";
        case uniform_t::FLOAT4: return "vec4";
        case uniform_t::MAT4: return "mat4";
        default: return "FIXME";
    }
}

static int uniform_type_size(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT:  return 4;
        case uniform_t::FLOAT2: return 8;
        case uniform_t::FLOAT3: return 12;
        case uniform_t::FLOAT4: return 16;
        case uniform_t::MAT4:   return 64;
        default: return 0;
    }
}

static int roundup(int val, int round_to) {
    return (val + (round_to - 1)) & ~(round_to - 1);
}

static const char* img_type_to_sokol_type_str(image_t::type_t type) {
    switch (type) {
        case image_t::IMAGE_2D: return "SG_IMAGETYPE_2D";
        case image_t::IMAGE_CUBE: return "SG_IMAGETYPE_CUBE";
        case image_t::IMAGE_3D: return "SG_IMAGETYPE_3D";
        case image_t::IMAGE_ARRAY: return "SG_IMAGETYPE_ARRAY";
        default: return "INVALID";
    }
}

static const uniform_block_t* find_uniform_block(const spirvcross_refl_t& refl, int slot) {
    for (const uniform_block_t& ub: refl.uniform_blocks) {
        if (ub.slot == slot) {
            return &ub;
        }
    }
    return nullptr;
}

static const image_t* find_image(const spirvcross_refl_t& refl, int slot) {
    for (const image_t& img: refl.images) {
        if (img.slot == slot) {
            return &img;
        }
    }
    return nullptr;
}

static const char* sokol_define(slang_t::type_t slang) {
    switch (slang) {
        case slang_t::GLSL330:      return "SOKOL_GLCORE33";
        case slang_t::GLSL100:      return "SOKOL_GLES2";
        case slang_t::GLSL300ES:    return "SOKOL_GLES3";
        case slang_t::HLSL5:        return "SOKOL_D3D11";
        case slang_t::METAL_MACOS:  return "SOKOL_METAL";
        case slang_t::METAL_IOS:    return "SOKOL_METAL";
        case slang_t::METAL_SIM:    return "SOKOL_METAL";
        default: return "<INVALID>";
    }
}

static const char* sokol_backend(slang_t::type_t slang) {
    switch (slang) {
        case slang_t::GLSL330:      return "SG_BACKEND_GLCORE33";
        case slang_t::GLSL100:      return "SG_BACKEND_GLES2";
        case slang_t::GLSL300ES:    return "SG_BACKEND_GLES3";
        case slang_t::HLSL5:        return "SG_BACKEND_D3D11";
        case slang_t::METAL_MACOS:  return "SG_BACKEND_METAL_MACOS";
        case slang_t::METAL_IOS:    return "SG_BACKEND_METAL_IOS";
        case slang_t::METAL_SIM:    return "SG_BACKEND_METAL_SIMULATOR";
        default: return "<INVALID>";
    }
}

static void write_header(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross) {
    L("/*\n");
    L("    #version:{}# (machine generated, don't edit!)\n\n", args.gen_version);
    L("    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)\n\n");
    L("    Overview:\n\n");
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;

        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spirvcross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spirvcross.sources[fs_src_index];
        L("        Shader program '{}':\n", prog.name);
        L("            Get shader desc: {}{}_shader_desc()\n", mod_prefix(inp), prog.name);
        L("            Vertex shader: {}\n", prog.vs_name);
        L("                Attribute slots:\n");
        const snippet_t& vs_snippet = inp.snippets[vs_src.snippet_index];
        for (const attr_t& attr: vs_src.refl.inputs) {
            if (attr.slot >= 0) {
                L("                    ATTR_{}{}_{} = {}\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
            }
        }
        for (const uniform_block_t& ub: vs_src.refl.uniform_blocks) {
            L("                Uniform block '{}':\n", ub.name);
            L("                    C struct: {}{}_t\n", mod_prefix(inp), ub.name);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.name, ub.slot);
        }
        for (const image_t& img: vs_src.refl.images) {
            L("                Image '{}':\n", img.name);
            L("                    Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        L("            Fragment shader: {}\n", prog.fs_name);
        for (const uniform_block_t& ub: fs_src.refl.uniform_blocks) {
            L("                Uniform block '{}':\n", ub.name);
            L("                    C struct: {}{}_t\n", mod_prefix(inp), ub.name);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.name, ub.slot);
        }
        for (const image_t& img: fs_src.refl.images) {
            L("                Image '{}':\n", img.name);
            L("                    Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        L("\n");
    }
    L("\n");
    L("    Shader descriptor structs:\n\n");
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        L("        sg_shader {} = sg_make_shader({}{}_shader_desc());\n", prog.name, mod_prefix(inp), prog.name);
    }
    L("\n");
    for (const spirvcross_source_t& src: spirvcross.sources) {
        if (src.refl.stage == stage_t::VS) {
            const snippet_t& vs_snippet = inp.snippets[src.snippet_index];
            L("    Vertex attribute locations for vertex shader '{}':\n\n", vs_snippet.name);
            L("        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){{\n");
            L("            .layout = {{\n");
            L("                .attrs = {{\n");
            for (const attr_t& attr: src.refl.inputs) {
                if (attr.slot >= 0) {
                    L("                    [ATTR_{}{}_{}] = {{ ... }},\n", mod_prefix(inp), vs_snippet.name, attr.name);
                }
            }
            L("                }},\n");
            L("            }},\n");
            L("            ...}});\n");
            L("\n");
        }
    }
    L("    Image bind slots, use as index in sg_bindings.vs_images[] or .fs_images[]\n\n");
    for (const image_t& img: spirvcross.unique_images) {
        L("        SLOT_{}{} = {};\n", mod_prefix(inp), img.name, img.slot);
    }
    L("\n");
    for (const uniform_block_t& ub: spirvcross.unique_uniform_blocks) {
        L("    Bind slot and C-struct for uniform block '{}':\n\n", ub.name);
        L("        {}{}_t {} = {{\n", mod_prefix(inp), ub.name, ub.name);
        for (const uniform_t& uniform: ub.uniforms) {
            L("            .{} = ...;\n", uniform.name);
        };
        L("        }};\n");
        L("        sg_apply_uniforms(SG_SHADERSTAGE_[VS|FS], SLOT_{}{}, &{}, sizeof({}));\n", mod_prefix(inp), ub.name, ub.name, ub.name);
        L("\n");
    }
    L("*/\n");
    L("#include <stdint.h>\n");
    L("#include <stdbool.h>\n");
}

static void write_vertex_attrs(const input_t& inp, const spirvcross_t& spirvcross) {
    // vertex attributes
    for (const spirvcross_source_t& src: spirvcross.sources) {
        if (src.refl.stage == stage_t::VS) {
            const snippet_t& vs_snippet = inp.snippets[src.snippet_index];
            for (const attr_t& attr: src.refl.inputs) {
                if (attr.slot >= 0) {
                    L("#define ATTR_{}{}_{} ({})\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
                }
            }
        }
    }
}

static void write_images_bind_slots(const input_t& inp, const spirvcross_t& spirvcross) {
    for (const image_t& img: spirvcross.unique_images) {
        L("#define SLOT_{}{} ({})\n", mod_prefix(inp), img.name, img.slot);
    }
}

static void write_uniform_blocks(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    for (const uniform_block_t& ub: spirvcross.unique_uniform_blocks) {
        L("#define SLOT_{}{} ({})\n", mod_prefix(inp), ub.name, ub.slot);
        L("#pragma pack(push,1)\n");
        int cur_offset = 0;
        L("SOKOL_SHDC_ALIGN(16) typedef struct {}{}_t {{\n", mod_prefix(inp), ub.name);
        for (const uniform_t& uniform: ub.uniforms) {
            int next_offset = uniform.offset;
            if (next_offset > cur_offset) {
                L("    uint8_t _pad_{}[{}];\n", cur_offset, next_offset - cur_offset);
                cur_offset = next_offset;
            }
            if (inp.type_map.count(uniform_type_str(uniform.type)) > 0) {
                // user-provided type names
                if (uniform.array_count == 1) {
                    L("    {} {};\n", inp.type_map.at(uniform_type_str(uniform.type)), uniform.name);
                }
                else {
                    L("    {} {}[{}];\n", inp.type_map.at(uniform_type_str(uniform.type)), uniform.name, uniform.array_count);
                }
            }
            else {
                // default type names (float)
                if (uniform.array_count == 1) {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   L("    float {};\n", uniform.name); break;
                        case uniform_t::FLOAT2:  L("    float {}[2];\n", uniform.name); break;
                        case uniform_t::FLOAT3:  L("    float {}[3];\n", uniform.name); break;
                        case uniform_t::FLOAT4:  L("    float {}[4];\n", uniform.name); break;
                        case uniform_t::MAT4:    L("    float {}[16];\n", uniform.name); break;
                        default:                 L("    INVALID_UNIFORM_TYPE;\n"); break;
                    }
                }
                else {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   L("    float {}[{}];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT2:  L("    float {}[{}][2];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT3:  L("    float {}[{}][3];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT4:  L("    float {}[{}][4];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::MAT4:    L("    float {}[{}][16];\n", uniform.name, uniform.array_count); break;
                        default:                 L("    INVALID_UNIFORM_TYPE;\n"); break;
                    }
                }
            }
            cur_offset += uniform_type_size(uniform.type) * uniform.array_count;
        }
        /* pad to multiple of 16-bytes struct size */
        const int round16 = roundup(cur_offset, 16);
        if (cur_offset != round16) {
            L("    uint8_t _pad_{}[{}];\n", cur_offset, round16-cur_offset);
        }
        L("}} {}{}_t;\n", mod_prefix(inp), ub.name);
        L("#pragma pack(pop)\n");
    }
}

static void write_shader_sources_and_blobs(const input_t& inp,
                                           const spirvcross_t& spirvcross,
                                           const bytecode_t& bytecode,
                                           slang_t::type_t slang)
{
    for (int snippet_index = 0; snippet_index < (int)inp.snippets.size(); snippet_index++) {
        const snippet_t& snippet = inp.snippets[snippet_index];
        if ((snippet.type != snippet_t::VS) && (snippet.type != snippet_t::FS)) {
            continue;
        }
        int src_index = spirvcross.find_source_by_snippet_index(snippet_index);
        assert(src_index >= 0);
        const spirvcross_source_t& src = spirvcross.sources[src_index];
        int blob_index = bytecode.find_blob_by_snippet_index(snippet_index);
        const bytecode_blob_t* blob = 0;
        if (blob_index != -1) {
            blob = &bytecode.blobs[blob_index];
        }
        std::vector<std::string> lines;
        pystring::splitlines(src.source_code, lines);
        /* first write the source code in a comment block */
        L("/*\n");
        for (const std::string& line: lines) {
            L("    {}\n", line);
        }
        L("*/\n");
        if (blob) {
            std::string c_name = fmt::format("{}{}_bytecode_{}", mod_prefix(inp), snippet.name, slang_t::to_str(slang));
            L("static const uint8_t {}[{}] = {{\n", c_name.c_str(), blob->data.size());
            const size_t len = blob->data.size();
            for (size_t i = 0; i < len; i++) {
                if ((i & 15) == 0) {
                    L("    ");
                }
                L("{:#04x},", blob->data[i]);
                if ((i & 15) == 15) {
                    L("\n");
                }
            }
            L("\n}};\n");
        }
        else {
            /* if no bytecode exists, write the source code, but also a a byte array with a trailing 0 */
            std::string c_name = fmt::format("{}{}_source_{}", mod_prefix(inp), snippet.name, slang_t::to_str(slang));
            const size_t len = src.source_code.length() + 1;
            L("static const char {}[{}] = {{\n", c_name.c_str(), len);
            for (size_t i = 0; i < len; i++) {
                if ((i & 15) == 0) {
                    L("    ");
                }
                L("{:#04x},", src.source_code[i]);
                if ((i & 15) == 15) {
                    L("\n");
                }
            }
            L("\n}};\n");
        }
    }
}

static void write_stage(const char* stage_name,
                        const program_t& prog,
                        const spirvcross_source_t& src,
                        const std::string& src_name,
                        const bytecode_blob_t* blob,
                        const std::string& blob_name,
                        slang_t::type_t slang)
{
    L("  {{ /* {} */\n", stage_name);
    if (blob) {
        L("    0, /* source */\n");
        L("    {}, /* bytecode */\n", blob_name.c_str());
        L("    {}, /* bytecode_size */\n", blob->data.size());
    }
    else {
        L("    {}, /* source */\n", src_name);
        L("    0,  /* bytecode */\n");
        L("    0,  /* bytecode_size */\n");
    }
    L("    \"{}\", /* entry */\n", src.refl.entry_point);
    L("    {{ /* uniform blocks */\n");
    for (int ub_index = 0; ub_index < uniform_block_t::NUM; ub_index++) {
        const uniform_block_t* ub = find_uniform_block(src.refl, ub_index);
        L("      {{\n");
        if (ub) {
            L("        {}, /* size */\n", roundup(ub->size,16));
            L("        {{ /* uniforms */");
            for (int u_index = 0; u_index < uniform_t::NUM; u_index++) {
                if (0 == u_index) {
                    L("{{\"{}\",SG_UNIFORMTYPE_FLOAT4,{}}},", ub->name, roundup(ub->size,16)/16);
                }
                else {
                    L("{{0,SG_UNIFORMTYPE_INVALID,0}},");
                }
            }
            L(" }},\n");
        }
        else {
            L("        0, /* size */\n");
            L("        {{ /* uniforms */");
            for (int u_index = 0; u_index < uniform_t::NUM; u_index++) {
                L("{{0,SG_UNIFORMTYPE_INVALID,0}},");
            }
            L(" }},\n");
        }
        L("      }},\n");
    }
    L("    }},\n");
    L("    {{ /* images */ ");
    for (int img_index = 0; img_index < image_t::NUM; img_index++) {
        const image_t* img = find_image(src.refl, img_index);
        if (img) {
            L("{{\"{}\",{}}},", img->name, img_type_to_sokol_type_str(img->type));
        }
        else {
            L("{{0,_SG_IMAGETYPE_DEFAULT}},");
        }
    }
    L(" }},\n");
    L("  }},\n");
}

static void write_shader_descs(const input_t& inp, const spirvcross_t& spirvcross, const bytecode_t& bytecode, slang_t::type_t slang) {
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spirvcross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spirvcross.sources[fs_src_index];
        int vs_blob_index = bytecode.find_blob_by_snippet_index(vs_snippet_index);
        int fs_blob_index = bytecode.find_blob_by_snippet_index(fs_snippet_index);
        const bytecode_blob_t* vs_blob = 0;
        const bytecode_blob_t* fs_blob = 0;
        if (vs_blob_index != -1) {
            vs_blob = &bytecode.blobs[vs_blob_index];
        }
        if (fs_blob_index != -1) {
            fs_blob = &bytecode.blobs[fs_blob_index];
        }
        std::string vs_src_name, fs_src_name;
        std::string vs_blob_name, fs_blob_name;
        if (vs_blob_index != -1) {
            vs_blob_name = fmt::format("{}{}_bytecode_{}", mod_prefix(inp), prog.vs_name, slang_t::to_str(slang));
        }
        else {
            vs_src_name = fmt::format("{}{}_source_{}", mod_prefix(inp), prog.vs_name, slang_t::to_str(slang));
        }
        if (fs_blob_index != -1) {
            fs_blob_name = fmt::format("{}{}_bytecode_{}", mod_prefix(inp), prog.fs_name, slang_t::to_str(slang));
        }
        else {
            fs_src_name = fmt::format("{}{}_source_{}", mod_prefix(inp), prog.fs_name, slang_t::to_str(slang));
        }

        /* write shader desc */
        L("static const sg_shader_desc {}{}_shader_desc_{} = {{\n", mod_prefix(inp), prog.name, slang_t::to_str(slang));
        L("  0, /* _start_canary */\n");
        L("  {{ /*attrs*/");
        for (int attr_index = 0; attr_index < attr_t::NUM; attr_index++) {
            const attr_t& attr = vs_src.refl.inputs[attr_index];
            if (attr.slot >= 0) {
                L("{{\"{}\",\"{}\",{}}},", attr.name, attr.sem_name, attr.sem_index);
            }
            else {
                L("{{0,0,0}},");
            }
        }
        L(" }},\n");
        write_stage("vs", prog, vs_src, vs_src_name, vs_blob, vs_blob_name, slang);
        write_stage("fs", prog, fs_src, fs_src_name, fs_blob, fs_blob_name, slang);
        L("  \"{}{}_shader\", /* label */\n", mod_prefix(inp), prog.name);
        L("  0, /* _end_canary */\n");
        L("}};\n");
    }
}

errmsg_t sokol_t::gen(const args_t& args, const input_t& inp,
                     const std::array<spirvcross_t,slang_t::NUM>& spirvcross,
                     const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    // first write everything into a string, and only when no errors occur,
    // dump this into a file (so we don't have half-written files lying around)
    file_content.clear();

    L("#pragma once\n");
    errmsg_t err;
    bool comment_header_written = false;
    bool common_decls_written = false;
    bool decl_guard_written = false;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            errmsg_t err = output_t::check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }
            if (!comment_header_written) {
                write_header(args, inp, spirvcross[i]);
                comment_header_written = true;
            }
            if (!common_decls_written) {
                common_decls_written = true;
                /* SOKOL_SHDC_ALIGN macro */
                L("#if !defined(SOKOL_SHDC_ALIGN)\n");
                L("#if defined(_MSC_VER)\n");
                L("#define SOKOL_SHDC_ALIGN(a) __declspec(align(a))\n");
                L("#else\n");
                L("#define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))\n");
                L("#endif\n");
                L("#endif\n");
                write_vertex_attrs(inp, spirvcross[i]);
                write_images_bind_slots(inp, spirvcross[i]);
                write_uniform_blocks(inp, spirvcross[i], slang);
            }
            if (!decl_guard_written) {
                decl_guard_written = true;
                L("#if !defined(SOKOL_SHDC_DECL)\n");
                L("#if !defined(SOKOL_GFX_INCLUDED)\n");
                L("#error \"Please include sokol_gfx.h before {}\"\n", pystring::os::path::basename(args.output));
                L("#endif\n");
            }
            if (!args.no_ifdef) {
                L("#if defined({})\n", sokol_define(slang));
            }
            write_shader_sources_and_blobs(inp, spirvcross[i], bytecode[i], slang);
            write_shader_descs(inp, spirvcross[i], bytecode[i], slang);
            if (!args.no_ifdef) {
                L("#endif /* {} */\n", sokol_define(slang));
            }
        }
    }

    // write access functions which return sg_shader_desc pointers
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        L("static inline const sg_shader_desc* {}{}_shader_desc(void) {{\n", mod_prefix(inp), prog.name);
        for (int i = 0; i < slang_t::NUM; i++) {
            slang_t::type_t slang = (slang_t::type_t) i;
            if (args.slang & slang_t::bit(slang)) {
                if (!args.no_ifdef) {
                    L("    #if defined({})\n", sokol_define(slang));
                }
                L("    if (sg_query_backend() == {}) {{\n", sokol_backend(slang));
                L("        return &{}{}_shader_desc_{};\n", mod_prefix(inp), prog.name, slang_t::to_str(slang));
                L("    }}\n");
                if (!args.no_ifdef) {
                    L("    #endif /* {} */\n", sokol_define(slang));
                }
            }
        }
        L("    return 0; /* can't happen */\n");
        L("}}\n");
    }

    if (decl_guard_written) {
        L("#endif /* SOKOL_SHDC_DECL */\n");
    }

    // write result into output file
    FILE* f = fopen(args.output.c_str(), "w");
    if (!f) {
        return errmsg_t::error(inp.path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);
    return errmsg_t();
}

} // namespace shdc
