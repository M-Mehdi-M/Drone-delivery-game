#version 330

// Input vertex attributes
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;

// Uniform variables
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Output vertex attributes (for fragment shader)
out vec3 frag_position;
out vec3 frag_normal;

// Simple pass-through shader with basic transformations
void main()
{
    // Transform vertex position to world space
    frag_position = vec3(Model * vec4(v_position, 1.0));
    
    // Transform normal to world space
    frag_normal = normalize(mat3(Model) * v_normal);
    
    // Final vertex position
    gl_Position = Projection * View * Model * vec4(v_position, 1.0);
}
