#version 330 core

// 输出变量
out vec4 FragColor;

// 平行光（定向光）
struct DirectionLight {
    vec3 direction;  // 平行光方向（也就是光照方向，只是不需要计算每一个着色点指向光源的向量了，直接使用光照方向，因为平行光的光源方向是固定的，所有着色点都使用同一个光照方向）

    // 光强度属性
    vec3 ambient; // 环境光强度
    vec3 diffuse; // 漫反射强度
    vec3 specular; // 镜面反射强度
};

// 点光源
struct PointLight {
    vec3 position;
    
    // 点光源衰减项参数
    float constant; // 常数项
    float linear; // 一次项
    float quadratic; // 二次项

    // 光强度属性
    vec3 ambient; // 环境光强度
    vec3 diffuse; // 漫反射强度
    vec3 specular; // 镜面反射强度
};

// 聚光灯
struct SpotLight {
    vec3 position;
    vec3 direction;

    // 聚光灯属性
    float cutOff; // 聚光灯的切光角，是cos值
    float outerCutOff; // 聚光灯的外切光角，是cos值  

    // 点光源衰减项参数
    float constant; // 常数项
    float linear; // 一次项
    float quadratic; // 二次项

    // 光强度属性
    vec3 ambient; // 环境光强度
    vec3 diffuse; // 漫反射强度
    vec3 specular; // 镜面反射强度
};

#define NR_POINT_LIGHTS 4

// 光照
uniform DirectionLight directionLight; // 平行光
uniform PointLight pointLights[NR_POINT_LIGHTS]; // 点光源
uniform SpotLight spotLight; // 聚光灯

uniform sampler2D brickMap; // 贴图

// 输入变量
in vec2 outTexCoord;
in vec3 outNormal; // 顶点法线
in vec3 outFragPos; // 片段位置

uniform vec3 viewPos; // 相机位置
uniform float factor; // 变化值 

// 计算定向光
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir);
// 计算点光源
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
// 计算聚光灯
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
// 计算深度值
// float LinearizeDepth(float depth, float near, float far);

void main() {

    vec3 viewDir = normalize(viewPos - outFragPos);
    vec3 normal = normalize(outNormal);
    
    // 定向光照
    vec3 result = CalcDirectionLight(directionLight, normal, viewDir);

    // 点光源
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], normal, outFragPos, viewDir);
    }

    vec4 texMap = texture(brickMap, outTexCoord);

    vec4 color = vec4(result, 1.0) * texMap;

    FragColor = vec4(color);
}


// 平行光计算函数
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir) {
    // 光照方向
    vec3 lightDir = normalize(light.direction);
    
    // 合并结果
    // 环境光
    vec3 ambient = light.ambient;

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff;

    // 镜面高光
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec;

    return ambient + diffuse + specular; // 综合三个分量
}

// 计算点光源
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // 计算光线方向：从片段位置指向光源位置
    vec3 lightDir = normalize(light.position - fragPos);

    // 环境光
    vec3 ambient = light.ambient;

    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff;

    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec;

    // 所有分量衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    // 合并结果
    return (ambient + diffuse + specular);
}

// 计算聚光灯
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // 计算光线方向：从片段位置指向光源位置
    vec3 lightDir = normalize(light.position - fragPos);

    // 环境光
    vec3 ambient = light.ambient;

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff;

    // 镜面光
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec;

    // 聚光灯特有计算（聚光灯边缘渐变）
    // 计算光线与聚光灯方向的夹角
    float theta = dot(lightDir, normalize(-light.direction));
    // 计算光锥过渡区域范围
    float epsilon = light.cutOff - light.outerCutOff;
    // 计算强度渐变（在内外切角之间平滑过渡）
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    ambient *= intensity;
    diffuse *= intensity;
    specular *= intensity;

    // 所有分量衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    // 合并结果
    return (ambient + diffuse + specular);
}

// // 计算深度值
// float LinearizeDepth(float depth, float near, float far) {
//     float z = depth * 2.0 - 1.0;
//     return (2.0 * near * far) / (far + near - z * (far - near));
// }