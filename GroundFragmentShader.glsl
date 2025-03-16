#version 330

// Input vertex attributes from vertex shader
in vec3 frag_position;
in vec3 frag_normal;
in float height;

// Output fragment color
layout(location = 0) out vec4 out_color;

// Function to mix two colors
vec3 mixColors(vec3 color1, vec3 color2, float factor) {
    return mix(color1, color2, factor);
}

void main()
{
    // Define base colors for ground
    vec3 lowColor = vec3(0.2, 0.5, 0.1);  // Green
    vec3 highColor = vec3(0.0, 0.2, 0.0);  // Light brown

    // Mix colors based on height
    vec3 finalColor = mixColors(lowColor, highColor, height);

    // Output the final color
    out_color = vec4(finalColor, 1.0);
}
