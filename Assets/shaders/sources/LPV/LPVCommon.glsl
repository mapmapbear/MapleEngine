#ifndef LPV_COMMON_GLSL
#define LPV_COMMON_GLSL

#define SH_C0 0.282094792f // 1 / 2sqrt(pi)
#define SH_C1 0.488602512f // sqrt(3/pi) / 2

#define SH_cosLobe_C0 0.886226925f // sqrt(pi)/2 
#define SH_cosLobe_C1 1.02332671f // sqrt(pi/3) 

#define PI 3.1415926535897932384626433832795

#define DEG_TO_RAD PI / 180.0f

#define imgStoreAdd(img, pos, data) \
  imageAtomicAdd(img, pos + ivec3(pos.x * 4 + 0, 0, 0), floatBitsToInt(data.x)); \
  imageAtomicAdd(img, pos + ivec3(pos.x * 4 + 1, 0, 0), floatBitsToInt(data.y)); \
  imageAtomicAdd(img, pos + ivec3(pos.x * 4 + 2, 0, 0), floatBitsToInt(data.z)); \
  imageAtomicAdd(img, pos + ivec3(pos.x * 4 + 3, 0, 0), floatBitsToInt(data.w));

vec4 evalCosineLobeToDir(vec3 dir)
{
	return vec4( SH_cosLobe_C0, -SH_cosLobe_C1 * dir.y, SH_cosLobe_C1 * dir.z, -SH_cosLobe_C1 * dir.x );
}

// Get SH coeficients out of direction
vec4 dirToSH(vec3 dir)
{
    return vec4(SH_C0, -SH_C1 * dir.y, SH_C1 * dir.z, -SH_C1 * dir.x);
}

#endif // LPV_COMMON_GLSL