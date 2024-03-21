Shader "CG2024/CelShading"
{
    Properties
    {
        _Albedo("Albedo", Color) = (1,1,1,1)
        _AlbedoTexture("Albedo Texture", 2D) = "white" {}
        _Reflectance("Reflectance (Ambient, Diffuse, Specular)", Vector) = (1, 1, 1, 0)
        _SpecularExponent("Specular Exponent", Float) = 100.0
        _Levels("Levels", int) = 3
    }

    SubShader
    {
        Tags { "RenderType" = "Opaque" }

        GLSLINCLUDE
        #include "UnityCG.glslinc"
        #include "ITUCG.glslinc"

        uniform vec4 _Albedo;
        uniform sampler2D _AlbedoTexture;
        uniform vec4 _AlbedoTexture_ST;
        uniform vec4 _Reflectance;
        uniform float _SpecularExponent;
        uniform int _Levels;
        ENDGLSL

        Pass
        {
            Name "FORWARD"
            Tags { "LightMode" = "ForwardBase" }

            GLSLPROGRAM

            struct vertexToFragment
            {
                vec3 worldPos;
                vec3 normal;
                vec2 texCoords;
            };

            #ifdef VERTEX
            out vertexToFragment v2f;

            void main()
            {
                v2f.worldPos = (unity_ObjectToWorld * gl_Vertex).xyz;
                v2f.normal = (unity_ObjectToWorld * vec4(gl_Normal, 0.0f)).xyz;
                v2f.texCoords = TransformTexCoords(gl_MultiTexCoord0.xy, _AlbedoTexture_ST);

                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
            }
            #endif // VERTEX

            #ifdef FRAGMENT
            in vertexToFragment v2f;

            void main()
            {
                vec3 lightDir = GetWorldSpaceLightDir(v2f.worldPos);
                vec3 viewDir = GetWorldSpaceViewDir(v2f.worldPos);

                vec3 normal = normalize(v2f.normal);

                vec3 albedo = texture(_AlbedoTexture, v2f.texCoords).rgb;
                albedo *= _Albedo.rgb;

                //albedo = vec3(1.f);

                //GetColorLuminance();

                vec3 lighting = BlinnPhongLighting(lightDir, viewDir, normal, vec3(1.0f), vec3(1.0f), _Reflectance.x, _Reflectance.y, _Reflectance.z, _SpecularExponent);

                float intensity = GetColorLuminance(lighting);

                float res = ceil(intensity * _Levels)/_Levels;
                //int res = floor(intensity * _Levels);
                //int res = round(intensity * _Levels);

                //vec3 newLightning = intensity * albedo;   

                gl_FragColor = vec4(res * albedo * _LightColor0.xyz, 1.f);// vec4(newLightning, 1.0f);
            }
            #endif // FRAGMENT

            ENDGLSL
        }

        Pass
        {
            Cull Front
            Name "OUTLINE"
            Tags { "LightMode" = "ForwardBase" }

            GLSLPROGRAM

            //VERTEX
            #ifdef VERTEX

            void main()
            {
                vec3 worldPos = (unity_ObjectToWorld * gl_Vertex).xyz;
                vec3 normal = (unity_ObjectToWorld * vec4(gl_Normal, 0.0f)).xyz;

                //normal = normal * 0.01f;

                worldPos +=  normal * 0.01f;

                gl_Position = unity_MatrixVP * vec4(worldPos, 1.0f);
                //gl_Position = unity_MatrixVP * vec4(worldPos, 0.10f);
                // gl_ModelViewProjectionMatrix * gl_Vertex;
                //gl_Position = worldPos * unity_MatrixVP;
            }
            #endif // VERTEX

            //FRAGMENT
            #ifdef FRAGMENT

            void main()
            {
                gl_FragColor = vec4(vec3(0.0f), 1.0f);
            }
            #endif // FRAGMENT

            ENDGLSL
        }

        Pass
        {
            Name "FORWARD"
            Tags { "LightMode" = "ForwardAdd" }

            ZWrite Off
            Blend One One

            GLSLPROGRAM

            struct vertexToFragment
            {
                vec3 worldPos;
                vec3 normal;
                vec2 texCoords;
            };

            #ifdef VERTEX
            out vertexToFragment v2f;

            void main()
            {
                v2f.worldPos = (unity_ObjectToWorld * gl_Vertex).xyz;
                v2f.normal = (unity_ObjectToWorld * vec4(gl_Normal, 0.0f)).xyz;
                v2f.texCoords = TransformTexCoords(gl_MultiTexCoord0.xy, _AlbedoTexture_ST);

                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
            }
            #endif // VERTEX

            #ifdef FRAGMENT
            in vertexToFragment v2f;

            void main()
            {
                vec3 lightDir = GetWorldSpaceLightDir(v2f.worldPos);
                vec3 viewDir = GetWorldSpaceViewDir(v2f.worldPos);

                vec3 normal = normalize(v2f.normal);

                vec3 albedo = texture(_AlbedoTexture, v2f.texCoords).rgb;
                albedo *= _Albedo.rgb;

                //albedo = vec3(1.f);

                vec3 lighting = BlinnPhongLighting(lightDir, viewDir, normal, albedo, vec3(1.0f), 0.0f, _Reflectance.y, _Reflectance.z, _SpecularExponent);

                gl_FragColor = vec4(lighting, 1.0f);
            }
            #endif // FRAGMENT

            ENDGLSL
        }
        Pass
        {
            Name "SHADOWCASTER"
            Tags { "LightMode" = "ShadowCaster" }

            GLSLPROGRAM

            #ifdef VERTEX
            void main()
            {
                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
            }
            #endif // VERTEX

            #ifdef FRAGMENT
            void main()
            {
            }
            #endif // FRAGMENT

            ENDGLSL
        }
    }
}
