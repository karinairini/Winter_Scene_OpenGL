#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec4 fPosLight;

out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

uniform vec3 posLightPoint1;
uniform vec3 posLightPoint2;
uniform vec3 posLightPoint3;

vec3 ambient;
vec3 diffuse;
vec3 specular;

float ambientStrength = 0.2f;
float specularStrength = 0.5f;
float shininess = 32.0f;

uniform float linear;
uniform float constant;
uniform float quadratic;

vec3 lampIglooColor = vec3(0.9f, 0.35f, 0.0f);
vec3 towerFlameColor = vec3(1.0f, 0.0f, 0.0f);

float computeShadow() { 
	// Check whether current frag pos is in shadow 
	float bias = max(0.05f * (1.0f - dot(fNormal, lightDir)), 0.005f); 

	// Perform perspective divide 
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// Transform to [0,1] range 
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	if (normalizedCoords.z > 1.0f) return 0.0f;
	
	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	
	// Get depth of current fragment from light's perspective 
	float currentDepth = normalizedCoords.z;

	// Check whether current frag pos is in shadow 
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	return shadow;
}

void computeLightComponents() {		
	vec3 cameraPosEye = vec3(0.0f); // in eye coordinates, the viewer is situated at the origin
	
	vec3 normalEye = normalize(fNormal); // transform normal
	
	vec3 lightDirN = normalize(lightDir); // compute light direction
	
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz); // compute view direction 
		
	ambient = ambientStrength * lightColor; // compute ambient light
	
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor; // compute diffuse light
	
	vec3 halfVector = normalize(lightDirN + viewDirN);

	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
    	specular = specularStrength * specCoeff * lightColor; // compute specular light
}

void computePointLight(vec3 posLight, vec3 colorLight, float linear, float quadratic) {
	vec3 cameraPosEye = vec3(0.0f);
	vec3 normalEye = normalize(fNormal);	
	vec3 lightDirN = normalize(posLight - fPosLight.xyz);
	vec3 viewDirN = normalize(cameraPosEye - fPosLight.xyz);
	vec3 halfVector = normalize(lightDirN + viewDirN);

	float diff = max(dot(normalEye,lightDirN),1.0);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);

	// Compute distance to light
	float distance = length(posLight - fPosLight.xyz);

	// Compute attenuation
	float att = 1.0 / (constant + quadratic * (distance * distance) + linear * distance);
				 
	vec3 ambientNew = att * colorLight * texture(diffuseTexture, fTexCoords).rgb;
	vec3 diffuseNew = diff * att * colorLight * texture(diffuseTexture, fTexCoords).rgb;
	vec3 specularNew = specCoeff  * att * colorLight * texture(specularTexture, fTexCoords).rgb;

	ambient += ambientNew;
	diffuse += diffuseNew;
	specular += specularNew;
}

void main() {
	computeLightComponents();

	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	float shadow = computeShadow();
	computePointLight(posLightPoint1, lampIglooColor, linear, quadratic);
	computePointLight(posLightPoint2, lampIglooColor, linear, quadratic);
	computePointLight(posLightPoint3, towerFlameColor, 0.14f, 0.07f);

	vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);

	fColor = vec4(color, 1.0f);
}
