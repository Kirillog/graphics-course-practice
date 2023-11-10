#version 330 core

uniform vec3 camera_position;

uniform vec3 ambient_light;
// uniform vec3 albedo;

uniform vec3 sun_direction;
uniform vec3 sun_color;

uniform vec3 point_light_position;
uniform vec3 point_light_color;
uniform vec3 point_light_attenuation;

uniform vec3 glossiness;
uniform float power;

uniform mat4 shadow_projection;
uniform sampler2DShadow shadow_map;

uniform sampler2D ambient_sampler;
uniform sampler2D alpha_sampler;
uniform int exists_alpha;

in vec3 position;
in vec3 normal;
in vec2 texcoord;


layout (location = 0) out vec4 out_color;

vec3 diffuse(vec3 direction, vec3 albedo) {
    return albedo * max(0.0, dot(normal, normalize(direction)));
}

vec3 specular(vec3 direction, vec3 albedo) {
    vec3 light_direction = normalize(direction);
    vec3 reflected = 2.0 * normal * dot(normal, light_direction) - light_direction;
    vec3 view_direction = camera_position - position;
    return glossiness * albedo * pow(max(0.0, dot(reflected, normalize(view_direction))), power);
}

vec3 phong(vec3 direction, vec3 albedo) {
    return diffuse(direction, albedo) + specular(direction, albedo);
}

void main()
{
    vec2 cord = vec2(texcoord.x, texcoord.y);
    vec4 data = texture(ambient_sampler, cord);
    vec3 albedo = vec3(data.x, data.y, data.z);

    vec3 ambient = albedo * ambient_light;
    vec3 point_light_direction = point_light_position - position;
    float point_light_dist = length(point_light_direction);
    float attenuation_coef = 1.0 / (point_light_attenuation.x + point_light_dist * point_light_attenuation.y + point_light_dist * point_light_dist * point_light_attenuation.z);
    vec3 color = ambient;
    //+ phong(point_light_direction, albedo) * point_light_color * attenuation_coef;

    vec4 ndc = shadow_projection * vec4(position, 1);
    if (abs(ndc.x) <= 1 && abs(ndc.y) <= 1) {
        vec2 shadow_texcoord = ndc.xy * 0.5 + 0.5;
        float shadow_depth = ndc.z * 0.5 + 0.5;

        float sum = 0.0;
        float sum_w = 0.0;
        const int N = 7;
        float radius = 5.0;
        for (int x = -N; x <= N; ++x) {
            for (int y = -N; y <= N; ++y) {
                float c = exp(-float(x * x + y * y) / (radius * radius));
                sum += c * texture(shadow_map, vec3(shadow_texcoord + vec2(x,y) / vec2(textureSize(shadow_map, 0)), shadow_depth));
                sum_w += c;
            }
        }
        color += phong(sun_direction, albedo) * sun_color * sum / sum_w;
    } else {
        color += phong(sun_direction, albedo) * sun_color;
    }

    float alpha = 0.0;
    if (exists_alpha == 1) {
        if (texture(alpha_sampler, cord).w < 0.5) {
            discard;
        } else {
            alpha = texture(alpha_sampler, cord).w;
        }
    } else {
        alpha = data.w;
    }
    out_color = vec4(color, 1.0);
}