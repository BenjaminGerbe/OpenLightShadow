#version 430

// il n'y a plus de variable predefinie en sortie: au revoir gl_FragColor
// on peut appeler la variable comme on veut mais doit etre marquee 'out vec4'

uniform sampler2D u_Sampler; // GL_TEXTURE0

layout(location = 0) out vec3 color;
out vec4 o_FragColor;

in vec3 v_FragPosition; // dans le repere du monde
in vec3 v_Normal;
in vec2 v_TexCoords;

uniform sampler2D u_ProjSampler; // GL_TEXTURE1
uniform samplerCube u_Skybox;

uniform vec3 u_SpecularColor;
uniform vec3 u_DiffuseColor;
uniform vec3 u_AmbianteColor;
uniform float u_Roughness;
uniform int u_Metalness;
uniform float u_Reflectance;

uniform mat4 u_WorldMatrix;
uniform vec3 u_lightDirection;

uniform LightMatrices {
	mat4 u_LightViewMatrix;
	mat4 u_LightProjectionMatrix;
};

uniform Matrices {
	mat4 u_ViewMatrix;
	mat4 u_ProjectionMatrix;
};

in vec4 v_ShadowCoords; // 
in vec4 vertPosition;

uniform samplerCube skybox;


vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);



void main(void)
{
   
    vec3 N = normalize(v_Normal);

    vec4 projColor = textureProj(u_ProjSampler, v_ShadowCoords);
    // il y'a des effets de bord avec cette technique
    // - d'une part il n'y a pas de clamping a [0,1], meme en changeant le mode 
    // de clamping de la texture, il est preferable de le faire manuellement
    // (ou bien d'utiliser le parametre 'border' de glTexImage2D)
    // - on peut eviter les back-projection avec un test
    // (dues aux coordonnees homogenes qui se chevauchent au dela de -1/+1)
    vec3 projectorTexCoords = v_ShadowCoords.xyz/v_ShadowCoords.w;
    if (projectorTexCoords.x < 0.0 || projectorTexCoords.x > 1.0 
     || projectorTexCoords.y < 0.0 || projectorTexCoords.y > 1.0) 
    {
        projColor = vec4(1.0);
    }


    float shadow = 0.0;
    vec3 L = normalize(u_lightDirection);
    float NdotL = clamp(dot(N,L),0.0,1.0);


    
    float bias = 0.005*tan(acos(NdotL));
    bias = clamp(bias, 0,0.01);
 
    float currentDepth = projectorTexCoords.z;

    for (int i=0;i<4;i++){
        float closestDepth = texture(u_ProjSampler,projectorTexCoords.xy + poissonDisk[i]/700.0).r;
        if(currentDepth-bias > closestDepth ){
           shadow+=0.2;
        }
    }
    



    float Roughness = clamp(u_Roughness,0.001,1);
    Roughness = Roughness*Roughness;
    float Shininess = 2.0 / (Roughness * Roughness)-2.0;
    float normalisation = ((Shininess + 8.0) / 8.0);
    vec3 V = -(vec3(inverse(u_ProjectionMatrix * u_ViewMatrix) * vec4(0, 0, 1.0, 1.0)));
    vec3 H = normalize(V + L);
    float NdotH = clamp(dot(N, H), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);

 
    vec3 R = reflect(V, normalize(v_Normal)); 
    vec3 reflection = texture(skybox, -R).rgb;
   

    vec3 ambiante = u_AmbianteColor;
    vec3 diffuse = (1.0 - u_Metalness)*u_DiffuseColor*NdotL*(1.0 - shadow);
    diffuse = mix(diffuse,diffuse*reflection,clamp(u_Reflectance*u_Reflectance,0,1));

    vec3 SpecularColor = mix(vec3(1,1,1),u_SpecularColor,u_Metalness);
    vec3 specular = SpecularColor * normalisation * pow(NdotH,Shininess);


    vec3 F0 = mix(vec3(0.16 * u_Reflectance * u_Reflectance), vec3(ambiante), u_Metalness);
    vec3 Fresnel = F0 + (1.0 - F0) * (1.0 - pow(NdotV, 5.0));
    vec3 FresnelNL = 1.0 - F0 + (1.0 - F0) * (1.0 - pow(NdotL, 5.0));

     vec3 Coefs = F0;
    //o_FragColor = vec4(ambiante + diffuse + (specular) ,1.0);
    if(u_Metalness>0){
        ambiante= mix(ambiante,ambiante*reflection,clamp(u_Reflectance*u_Reflectance,0,1));
    }
    
    o_FragColor = vec4(ambiante+ (diffuse*FresnelNL)+normalisation*(specular*Coefs) ,1.0);

    //o_FragColor = vec4(texture(skybox,v_TexCoords),1.0);

    // debug des normales
    //o_FragColor = vec4(v_Normal * 0.5 + 0.5, 1.0);
}