#version 330

// Input vertex attributes from vertex shader
in vec3 frag_position;
in vec3 frag_normal;

// Uniform variables
uniform vec3 object_color;

// Output fragment color
layout(location = 0) out vec4 out_color;

void main()
{
    // Simply output the uniform color
    out_color = vec4(object_color, 1.0);
}
