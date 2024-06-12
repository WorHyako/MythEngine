#version 330 core
struct Material{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

struct Light{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

void main()
{
      vec3 ambient = light.ambient * material.ambient;
   
      vec3 result = (ambient);
      if(gl_FragCoord.x < 400)
        result = vec3(1.0f);
      FragColor = vec4(result, 1.0f);

      float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
      vec3 brightnessLength = vec3(FragColor.rgb - vec3(10.0f));
      BrightColor = length(brightnessLength) > 1.0f ? vec4(FragColor.rgb, 1.0f) : vec4(0.0f, 0.0f, 0.0f, 1.0f);  
};