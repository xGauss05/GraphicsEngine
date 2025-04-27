struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition; // In world space
out vec3 vNormal; 	// In world space
out vec3 vViewDir;  // In world space

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition; // In world space
in vec3 vNormal;   // In world space
in vec3 vViewDir;  // In world space

uniform sampler2D uTexture;
uniform sampler2D uNormal;
uniform sampler2D uAO;
uniform sampler2D uEmissive;
uniform sampler2D uSpecular;
uniform sampler2D uRoughness;
uniform sampler2D uDepth;

layout(binding = 0, std140) uniform GlobalParams 
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

//layout(location = 0) out vec4 oColor;
layout(location = 0) out vec4 rt0; // Albedo, Ambient Occlusion
layout(location = 1) out vec4 rt1; // Specular, Roughness
layout(location = 2) out vec4 rt2; // Normals
layout(location = 3) out vec4 rt3; // Emissive + Lightmaps
layout(location = 4) out vec4 rt4; // Position
layout(location = 5) out vec4 rt5; // Depth

float near = 0.1f;
float far = 100.0f;

float linearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
	vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vPosition);
    vec3 resultColor = vec3(0.0);

    for (uint i = 0; i < uLightCount; i++) 
	{
        Light light = uLight[i];
        vec3 lightDir;
        if (light.type == 0) // Directional light
		{ 
            lightDir = normalize(-light.direction);
        } 
		else if (light.type == 1) // Point light
		{ 
            lightDir = normalize(light.position - vPosition);
        }

        // Diffuse
        vec3 diffuse = max(dot(normal, lightDir), 0.0) * light.color;

        // Specular
        vec3 reflectDir = reflect(-lightDir, normal);    
        vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * light.color;

        resultColor += diffuse + specular;
    }

    // Texture + light
    vec3 textureColor = texture(uTexture, vTexCoord).rgb;
    //oColor = vec4(textureColor * resultColor, 1.0);

	rt0 = vec4(textureColor * resultColor, 1.0); // Albedo, Ambient Occlusion
	rt2 = vec4(normal, 1.0); 		 			 // Normals
	rt3 = vec4(resultColor, 1.0);				 // Lighting
	rt4 = vec4(vPosition, 1.0);		 			 // Position
	
    float linearDepth = linearizeDepth(gl_FragCoord.z) / far; 
	rt5 = vec4(vec3(linearDepth), 1.0); 		 // Depth
	
}

#endif
#endif