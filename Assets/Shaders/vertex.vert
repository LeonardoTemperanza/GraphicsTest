
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragTextureCoord;
layout (location = 3) out vec3 fragTangent;
layout (location = 4) out vec3 fragBitangent;
layout (location = 5) out vec3 fragViewPos;

struct DirectionalLight
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(std140, binding = 0) uniform PerFrame
{
    mat4 world2View;
    mat4 view2Proj;
    vec3 viewPos;
};

layout(std140, binding = 1) uniform PerObj
{
    mat4 model2World;
};

void main()
{
    vec3 viewPos = (world2View * (model2World * vec4(aPos, 1.0f))).xyz;
    
    mat3 model2WorldDir = mat3(transpose(inverse(model2World)));
    
    vec3 worldPos = (model2World * vec4(aPos, 1.0f)).xyz;
    vec3 normal = model2WorldDir * aNormal;
    fragTextureCoord = aTextureCoord;
    
    // Reorthogonalize tangents
    vec3 tangent = aTangent;
    vec3 bitangent = aBitangent;
    
    // Convert to tangent space
    vec3 t = normalize(model2WorldDir * aTangent);
    vec3 b = normalize(model2WorldDir * aBitangent);
    vec3 n = normalize(model2WorldDir * aNormal);
    mat3 TBN = transpose(mat3(t, b, n));
    
    fragPos       = TBN * worldPos;
    fragViewPos   = TBN * viewPos;
    fragNormal    = normal;
    fragTangent   = tangent;
    fragBitangent = bitangent;
    gl_Position   = view2Proj * vec4(viewPos, 1.0f);
}
