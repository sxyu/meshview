#pragma once
#ifndef VIEWER_SHADER_INLINE_91D27C05_59C0_4F9F_A6C5_6AA9E2000CDA
#define VIEWER_SHADER_INLINE_91D27C05_59C0_4F9F_A6C5_6AA9E2000CDA

namespace meshview {

// Shading for mesh using texture
static const char* MESH_VERTEX_SHADER = R"SHADER(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

uniform mat4 M;
uniform mat4 MVP;
uniform mat3 NormalMatrix;

void main() {
    TexCoord = aTexCoord.xy;
    FragPos = (M * vec4(aPosition, 1.0f)).xyz;
    Normal = NormalMatrix * aNormal;
    gl_Position = MVP * vec4(aPosition, 1.0f);
})SHADER";

static const char* MESH_FRAGMENT_SHADER = R"SHADER(
#version 330 core
out vec4 FragColor;
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos; // Position (world)
in vec2 TexCoord; // UV coords
in vec3 Normal; // Normal vector (world)
uniform vec3 viewPos; // Camera position (world)
uniform Material material; // Material info
uniform Light light; // Light info

void main(){
    vec3 objectColor = texture(material.diffuse, TexCoord).rgb;
    vec3 ambient = light.ambient * objectColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * objectColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoord).rgb;

    FragColor = vec4(ambient + diffuse + specular, 1.0f);
})SHADER";

// Shading for mesh, using interpolated per-vertex colors instead of texture
static const char* MESH_VERTEX_SHADER_VERT_COLOR = R"SHADER(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aVertColor;
layout(location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 VertColor;
out vec3 Normal;

uniform mat4 M;
uniform mat4 MVP;
uniform mat3 NormalMatrix;

void main() {
    VertColor = aVertColor;
    FragPos = (M * vec4(aPosition, 1.0f)).xyz;
    Normal = NormalMatrix * aNormal;
    gl_Position = MVP * vec4(aPosition, 1.0f);
})SHADER";

static const char* MESH_FRAGMENT_SHADER_VERT_COLOR = R"SHADER(
#version 330 core
out vec4 FragColor;
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct Material {
    float shininess;
};

in vec3 FragPos; // Position (world)
in vec3 VertColor; // Vertex color
in vec3 Normal; // Normal vector (world)
uniform vec3 viewPos; // Camera position (world)
uniform Light light; // Light info
uniform Material material; // Limited material info

void main() {
    vec3 ambient = light.ambient * VertColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * VertColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec;

    FragColor = vec4(ambient + diffuse + specular, 1.0f);
})SHADER";

// Very simple shading for point cloud (points and polylines)
static const char* POINTCLOUD_VERTEX_SHADER = R"SHADER(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
out vec3 Color;
uniform mat4 M;
uniform mat4 MVP;
void main() {
    Color = aColor;
    gl_Position = MVP * vec4(aPosition, 1.0f);
}
)SHADER";

static const char* POINTCLOUD_FRAGMENT_SHADER = R"SHADER(
#version 330 core

out vec4 FragColor; // Ouput data
in vec3 Color; // Color
void main(){
    // Finish
    FragColor = vec4(Color, 1.0f);
}
)SHADER";

}  // namespace meshview

#endif  // ifndef VIEWER_SHADER_INLINE_91D27C05_59C0_4F9F_A6C5_6AA9E2000CDA
