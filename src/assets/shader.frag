#version 330 core

uniform vec4 output_color;
uniform sampler2D texture_sampler;
uniform sampler2D texture_sampler2;

in vec2 out_tex_coord;

out vec4 FragColor;

void main()
{
    //FragColor =  mix(texture(texture_sampler, out_tex_coord), texture(texture_sampler2, out_tex_coord), 0.3);
    
    // vec4 texc_color = texture(texture_sampler, out_tex_coord);
    // float avg = (texc_color.x + texc_color.y + texc_color.z) / 3;
    // FragColor = vec4(output_color.xyz * avg, 1.0); 

    FragColor = texture(texture_sampler, out_tex_coord); 
}