
#version 150

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uLightMatrix;

uniform vec3 uCameraPosition;

out vec3 fNormal;
out vec3 fWorldPosition;
out vec4 fLightSpacePosition;
out vec2 fTexCoords;


void main()
{
	vec4 Position = uModelMatrix * vec4(vPosition, 1.0);

	fWorldPosition = Position.xyz;
	fLightSpacePosition = uLightMatrix * Position;
	fNormal = (inverse(transpose(uModelMatrix)) * vec4(vNormal, 0.0)).xyz;
	fTexCoords = vTexCoords;

	gl_Position = uProjectionMatrix * uViewMatrix * Position;
}
