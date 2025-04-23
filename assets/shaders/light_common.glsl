#ifndef LIGHT_COMMON_GLSL_INCLUDED
#define LIGHT_COMMON_GLSL_INCLUDED

    #define MAX_LIGHT_COUNT     16
    #define TYPE_DIRECTIONAL    0
    #define TYPE_POINT          1
    #define TYPE_SPOT           2

    struct InputVaryings {
        vec3 world;
        vec3 view;
        vec3 normal;
    };

    struct Attenuation {
        float constant;
        float linear;
        float quadratic;
    };

    struct SpotAngle {
        float inner;
        float outer;
    };

    struct Light {
        int type;
        vec3 color;
        vec3 position;
        vec3 direction;
        Attenuation attenuation;
        SpotAngle spotAngle;
    };

    struct SkyLightColor {
        vec3 top;
        vec3 middle;
        vec3 bottom;
    };

    struct Material {
        vec3 diffuse;
        vec3 specular;
        vec3 ambient;
        vec3 emissive;
        float shininess;
    };

    struct TexturedMaterial {
        sampler2D albedo_tex;
        vec3 albedo_tint;
        sampler2D specular_tex;
        vec3 specular_tint;
        sampler2D ambient_occlusion_tex;
        sampler2D roughness_tex;
        vec2 roughness_range;
        sampler2D emissive_tex;
        vec3 emissive_tint;
    };

    float calculate_lambert(vec3 normal, vec3 light_direction) {
        return max(0.0f, dot(normal, -light_direction));
    }

    float calculate_phong(vec3 normal, vec3 light_direction, vec3 view, float shininess) {
        vec3 reflected = reflect(light_direction, normal);
        return pow(max(0.0f, dot(view, reflected)), shininess);
    }

    vec3 calculate_accumulated_light(Material material, Light[MAX_LIGHT_COUNT] lights, SkyLightColor sky_light_color, InputVaryings input, int light_count) {
        vec3 normal = normalize(input.normal);
        vec3 view = normalize(input.view);
        int count = min(light_count, MAX_LIGHT_COUNT);

        vec3 ambient = material.ambient * (normal.y > 0 ?
            mix(sky_light_color.middle, sky_light_color.top, normal.y) :
            mix(sky_light_color.middle, sky_light_color.bottom, -normal.y));
        vec3 accumulated_light = material.emissive + ambient;

        for(int index = 0; index < count; index++) {
            Light light = lights[index];
            vec3 light_direction;
            float attenuation = 1;
            if(light.type == TYPE_DIRECTIONAL)
                light_direction = light.direction;
            else {
                light_direction = input.world - light.position;
                float distance = length(light_direction);
                light_direction /= distance;

                attenuation *= 1.0f / (light.attenuation.constant +
                light.attenuation.linear * distance +
                light.attenuation.quadratic * distance * distance);

                if(light.type == TYPE_SPOT){
                    float angle = acos(dot(light.direction, light_direction));
                    attenuation *= smoothstep(light.spotAngle.outer, light.spotAngle.inner, angle);
                }
            }

            vec3 diffuse = material.diffuse * light.color * calculate_lambert(normal, light_direction);
            vec3 specular = material.specular * light.color * calculate_phong(normal, light_direction, view, material.shininess);
            accumulated_light += (diffuse + specular) * attenuation;
        }
        return accumulated_light;
    }

    Material sample_material(TexturedMaterial tex_mat, vec2 tex_coord){
        Material mat;

        mat.diffuse = tex_mat.albedo_tint * texture(tex_mat.albedo_tex, tex_coord).rgb;
        mat.specular = tex_mat.specular_tint * texture(tex_mat.specular_tex, tex_coord).rgb;
        mat.emissive = tex_mat.emissive_tint * texture(tex_mat.emissive_tex, tex_coord).rgb;
        mat.ambient = mat.diffuse * texture(tex_mat.ambient_occlusion_tex, tex_coord).r;
        float roughness = mix(tex_mat.roughness_range.x, tex_mat.roughness_range.y,
                              texture(tex_mat.roughness_tex, tex_coord).r);
        mat.shininess = 2.0f / pow(clamp(roughness, 0.001f, 0.999f), 4.0f) - 2.0f;

        return mat;
    }

#endif
