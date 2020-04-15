#version 330 core
layout 
    (triangles)
    in;
layout
    (triangle_strip,
    max_vertices = 3)
    out;

in VS_OUT
{
    vec2 texCoords;
}
gs_in[];



uniform float _time;

vec4 expand(vec4 position, vec3 normal) 
{
    return position + vec4(normal * _time, 0.0);
}

void main() {    
    vec3 dir1 = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 dir2 = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 normal = normalize(cross(dir1, dir2));

    gl_Position = expand(gl_in[0].gl_Position, normal);
    EmitVertex();
    gl_Position = expand(gl_in[1].gl_Position, normal);
    EmitVertex();
    gl_Position = expand(gl_in[2].gl_Position, normal);
    EmitVertex();
    EndPrimitive();

}  