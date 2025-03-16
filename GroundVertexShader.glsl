#version 330

// Input vertex attributes
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;

// Uniform variables
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Noise function
float noise(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Output vertex attributes (for fragment shader)
out vec3 frag_position;
out vec3 frag_normal;
out float height;  // Pass height to fragment shader

void main()
{
    // Generate height based on noise
    float frequency = 0.2;  // Adjust frequency for finer or coarser noise
    float amplitude = 0.2;  // Reduce amplitude for minimal height variation
    height = noise(v_position.xz * frequency) * amplitude;

    // Adjust vertex position height
    vec3 updated_position = v_position;
    updated_position.y += height;

    // Transform vertex position to world space
    frag_position = vec3(Model * vec4(updated_position, 1.0));
    
    // Transform normal to world space
    frag_normal = normalize(mat3(Model) * v_normal);

    // Final vertex position
    gl_Position = Projection * View * Model * vec4(updated_position, 1.0);
}
