#version 450

in vec3 norm;
in vec2 TexCoord;
in vec3 WorldPos;
out vec4 frag;

struct LightProperties
{
	uint type;
	vec3 direction;
	vec3 color;
	vec3 position;
};

uniform LightProperties _light;

uniform vec3 cameraPosition;

uniform sampler2D albedoSampler;
uniform sampler2D normalMapSampler;
uniform sampler2D metallicRoughnessMapSampler;
uniform sampler2D aoMapSampler;


const float PI = 3.14159265359;


float Trowbridge_Reitz_NornalDistributionGGX(float NdotH, float roughness)
{
	float roughnessSqr = roughness * roughness;
	float distribution = NdotH * NdotH * (roughnessSqr - 1.0) + 1.0;

	return roughnessSqr/ (PI * distribution*distribution);
}

//Schlick-GGX GSF
float SchlickGeometricShadowing(float NdotL, float NdotV, float roughness)
{
	float k = roughness / 2;;

	float SmithL = NdotL / (NdotL * (1 - k) +k);
	float SmithV = NdotV / (NdotV * (1 - k) +k);

	return (SmithL * SmithV);
}

float SchlickFresnel(float i)
{
	float x = clamp(1.0 -i, 0.0, 1.0);
	float xsqr = x * x;

	return xsqr*xsqr*x;
}

vec3 SchlickFresnelFunction(vec3 SpecularColor, float  LdotH)
{
	return SpecularColor + (1 - SpecularColor) * SchlickFresnel(LdotH);
}

vec3 getNormalFromMap()
{
	vec2 tcoord = vec2(TexCoord.x, 1 - TexCoord.y);
    vec3 tangentNormal = texture(normalMapSampler, tcoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(tcoord);
    vec2 st2 = dFdy(tcoord);

    vec3 N   = normalize(norm);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() 
{

	vec3 lightdir = vec3(0, -1, .3);
	vec3 lightColor = vec3(.95, .95, .95);

	vec2 tcoord = vec2(TexCoord.x, 1 - TexCoord.y); 
	vec3 albedo = pow(texture(albedoSampler, tcoord).rgb, vec3(2.2));
	float metallic = texture(metallicRoughnessMapSampler, tcoord).b;
	float roughness = texture(metallicRoughnessMapSampler, tcoord).g;
	float ao = texture(aoMapSampler, tcoord).r;
	//frag = texture(diffuseSampler, vec2(TexCoord.x, 1.0 - TexCoord.y));

	//vec3 N = (normalize(norm));
	vec3 N = getNormalFromMap();
	vec3 V = normalize(cameraPosition - WorldPos);

	vec3 FO = vec3(0.04); //fresnel 
	FO = mix(FO, albedo, metallic);


	vec3 Lo = vec3(0.0);

	// per light calcualtion
	vec3 L = normalize(_light.position - WorldPos);
	L = -lightdir;
	vec3 H  = normalize(V+L);
	float d = length(cameraPosition - WorldPos);
	float attenuation = 1.0 ;/// (d * d); // fall off
	vec3 radiance = lightColor * attenuation;

	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	// cook - torrance brdf
	float NDF = Trowbridge_Reitz_NornalDistributionGGX(NdotH, roughness);
	float GSF = SchlickGeometricShadowing(NdotL, NdotV, roughness);
	vec3 F  = SchlickFresnelFunction( FO, max(dot(L, H), 0.0) );

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;

	kD *= 1.0 - metallic;

	vec3 numerator = NDF * GSF * F;
	float denominator = 4.0 * NdotV * NdotL;
	vec3 specular = numerator / max(denominator, 0.001);

	Lo += (kD * albedo/PI + specular) * radiance * NdotL; 
	// per light ends 

	vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient + Lo;

	//Gamma correction
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2) );

	frag = vec4(color, 1.0);
}
