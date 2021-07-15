/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SVM_TYPES_H__
#define __SVM_TYPES_H__

CCL_NAMESPACE_BEGIN

/* Stack */

/* SVM stack has a fixed size */
#define SVM_STACK_SIZE 255
/* SVM stack offsets with this value indicate that it's not on the stack */
#define SVM_STACK_INVALID 255

#define SVM_BUMP_EVAL_STATE_SIZE 9

/* Nodes */

typedef enum ShaderNodeType {
  NODE_END = 0,
  NODE_SHADER_JUMP,
  NODE_CLOSURE_BSDF,
  NODE_CLOSURE_EMISSION,
  NODE_CLOSURE_BACKGROUND,
  NODE_CLOSURE_SET_WEIGHT,
  NODE_CLOSURE_WEIGHT,
  NODE_EMISSION_WEIGHT,
  NODE_MIX_CLOSURE,
  NODE_JUMP_IF_ZERO,
  NODE_JUMP_IF_ONE,
  NODE_GEOMETRY,
  NODE_CONVERT,
  NODE_TEX_COORD,
  NODE_VALUE_F,
  NODE_VALUE_V,
  NODE_ATTR,
  NODE_VERTEX_COLOR,
  NODE_GEOMETRY_BUMP_DX,
  NODE_GEOMETRY_BUMP_DY,
  NODE_SET_DISPLACEMENT,
  NODE_DISPLACEMENT,
  NODE_VECTOR_DISPLACEMENT,
  NODE_RGB_TO_SPECTRUM,
  NODE_TEX_IMAGE,
  NODE_TEX_IMAGE_BOX,
  NODE_TEX_NOISE,
  NODE_SET_BUMP,
  NODE_ATTR_BUMP_DX,
  NODE_ATTR_BUMP_DY,
  NODE_VERTEX_COLOR_BUMP_DX,
  NODE_VERTEX_COLOR_BUMP_DY,
  NODE_TEX_COORD_BUMP_DX,
  NODE_TEX_COORD_BUMP_DY,
  NODE_CLOSURE_SET_NORMAL,
  NODE_ENTER_BUMP_EVAL,
  NODE_LEAVE_BUMP_EVAL,
  NODE_HSV,
  NODE_CLOSURE_HOLDOUT,
  NODE_FRESNEL,
  NODE_LAYER_WEIGHT,
  NODE_CLOSURE_VOLUME,
  NODE_PRINCIPLED_VOLUME,
  NODE_MATH,
  NODE_VECTOR_MATH,
  NODE_RGB_RAMP,
  NODE_GAMMA,
  NODE_BRIGHTCONTRAST,
  NODE_LIGHT_PATH,
  NODE_OBJECT_INFO,
  NODE_PARTICLE_INFO,
  NODE_HAIR_INFO,
  NODE_TEXTURE_MAPPING,
  NODE_MAPPING,
  NODE_MIN_MAX,
  NODE_CAMERA,
  NODE_TEX_ENVIRONMENT,
  NODE_TEX_SKY,
  NODE_TEX_GRADIENT,
  NODE_TEX_VORONOI,
  NODE_TEX_MUSGRAVE,
  NODE_TEX_WAVE,
  NODE_TEX_MAGIC,
  NODE_TEX_CHECKER,
  NODE_TEX_BRICK,
  NODE_TEX_WHITE_NOISE,
  NODE_NORMAL,
  NODE_LIGHT_FALLOFF,
  NODE_IES,
  NODE_RGB_CURVES,
  NODE_VECTOR_CURVES,
  NODE_TANGENT,
  NODE_NORMAL_MAP,
  NODE_INVERT,
  NODE_MIX,
  NODE_SEPARATE_VECTOR,
  NODE_COMBINE_VECTOR,
  NODE_SEPARATE_HSV,
  NODE_COMBINE_HSV,
  NODE_VECTOR_ROTATE,
  NODE_VECTOR_TRANSFORM,
  NODE_WIREFRAME,
  NODE_WAVELENGTH,
  NODE_BLACKBODY,
  NODE_MAP_RANGE,
  NODE_CLAMP,
  NODE_BEVEL,
  NODE_AMBIENT_OCCLUSION,
  NODE_TEX_VOXEL,
  NODE_AOV_START,
  NODE_AOV_COLOR,
  NODE_AOV_VALUE,
  /* NOTE: for best OpenCL performance, item definition in the enum must
   * match the switch case order in svm.h. */
} ShaderNodeType;

typedef enum NodeAttributeOutputType {
  NODE_ATTR_OUTPUT_FLOAT3 = 0,
  NODE_ATTR_OUTPUT_FLOAT,
  NODE_ATTR_OUTPUT_FLOAT_ALPHA,
} NodeAttributeOutputType;

typedef enum NodeAttributeType {
  NODE_ATTR_FLOAT = 0,
  NODE_ATTR_FLOAT2,
  NODE_ATTR_FLOAT3,
  NODE_ATTR_FLOAT4,
  NODE_ATTR_RGBA,
  NODE_ATTR_MATRIX
} NodeAttributeType;

typedef enum NodeGeometry {
  NODE_GEOM_P = 0,
  NODE_GEOM_N,
  NODE_GEOM_T,
  NODE_GEOM_I,
  NODE_GEOM_Ng,
  NODE_GEOM_uv
} NodeGeometry;

typedef enum NodeObjectInfo {
  NODE_INFO_OB_LOCATION,
  NODE_INFO_OB_COLOR,
  NODE_INFO_OB_INDEX,
  NODE_INFO_MAT_INDEX,
  NODE_INFO_OB_RANDOM
} NodeObjectInfo;

typedef enum NodeParticleInfo {
  NODE_INFO_PAR_INDEX,
  NODE_INFO_PAR_RANDOM,
  NODE_INFO_PAR_AGE,
  NODE_INFO_PAR_LIFETIME,
  NODE_INFO_PAR_LOCATION,
  NODE_INFO_PAR_ROTATION,
  NODE_INFO_PAR_SIZE,
  NODE_INFO_PAR_VELOCITY,
  NODE_INFO_PAR_ANGULAR_VELOCITY
} NodeParticleInfo;

typedef enum NodeHairInfo {
  NODE_INFO_CURVE_IS_STRAND,
  NODE_INFO_CURVE_INTERCEPT,
  NODE_INFO_CURVE_THICKNESS,
  /* Fade for minimum hair width transiency. */
  // NODE_INFO_CURVE_FADE,
  NODE_INFO_CURVE_TANGENT_NORMAL,
  NODE_INFO_CURVE_RANDOM,
} NodeHairInfo;

typedef enum NodeLightPath {
  NODE_LP_camera = 0,
  NODE_LP_shadow,
  NODE_LP_diffuse,
  NODE_LP_glossy,
  NODE_LP_singular,
  NODE_LP_reflection,
  NODE_LP_transmission,
  NODE_LP_volume_scatter,
  NODE_LP_backfacing,
  NODE_LP_ray_length,
  NODE_LP_ray_depth,
  NODE_LP_ray_diffuse,
  NODE_LP_ray_glossy,
  NODE_LP_ray_transparent,
  NODE_LP_ray_transmission,
} NodeLightPath;

typedef enum NodeLightFalloff {
  NODE_LIGHT_FALLOFF_QUADRATIC,
  NODE_LIGHT_FALLOFF_LINEAR,
  NODE_LIGHT_FALLOFF_CONSTANT
} NodeLightFalloff;

typedef enum NodeTexCoord {
  NODE_TEXCO_NORMAL,
  NODE_TEXCO_OBJECT,
  NODE_TEXCO_CAMERA,
  NODE_TEXCO_WINDOW,
  NODE_TEXCO_REFLECTION,
  NODE_TEXCO_DUPLI_GENERATED,
  NODE_TEXCO_DUPLI_UV,
  NODE_TEXCO_VOLUME_GENERATED
} NodeTexCoord;

typedef enum NodeMix {
  NODE_MIX_BLEND = 0,
  NODE_MIX_ADD,
  NODE_MIX_MUL,
  NODE_MIX_SUB,
  NODE_MIX_SCREEN,
  NODE_MIX_DIV,
  NODE_MIX_DIFF,
  NODE_MIX_DARK,
  NODE_MIX_LIGHT,
  NODE_MIX_OVERLAY,
  NODE_MIX_DODGE,
  NODE_MIX_BURN,
  NODE_MIX_HUE,
  NODE_MIX_SAT,
  NODE_MIX_VAL,
  NODE_MIX_COLOR,
  NODE_MIX_SOFT,
  NODE_MIX_LINEAR,
  NODE_MIX_CLAMP /* used for the clamp UI option */
} NodeMix;

typedef enum NodeMathType {
  NODE_MATH_ADD,
  NODE_MATH_SUBTRACT,
  NODE_MATH_MULTIPLY,
  NODE_MATH_DIVIDE,
  NODE_MATH_SINE,
  NODE_MATH_COSINE,
  NODE_MATH_TANGENT,
  NODE_MATH_ARCSINE,
  NODE_MATH_ARCCOSINE,
  NODE_MATH_ARCTANGENT,
  NODE_MATH_POWER,
  NODE_MATH_LOGARITHM,
  NODE_MATH_MINIMUM,
  NODE_MATH_MAXIMUM,
  NODE_MATH_ROUND,
  NODE_MATH_LESS_THAN,
  NODE_MATH_GREATER_THAN,
  NODE_MATH_MODULO,
  NODE_MATH_ABSOLUTE,
  NODE_MATH_ARCTAN2,
  NODE_MATH_FLOOR,
  NODE_MATH_CEIL,
  NODE_MATH_FRACTION,
  NODE_MATH_SQRT,
  NODE_MATH_INV_SQRT,
  NODE_MATH_SIGN,
  NODE_MATH_EXPONENT,
  NODE_MATH_RADIANS,
  NODE_MATH_DEGREES,
  NODE_MATH_SINH,
  NODE_MATH_COSH,
  NODE_MATH_TANH,
  NODE_MATH_TRUNC,
  NODE_MATH_SNAP,
  NODE_MATH_WRAP,
  NODE_MATH_COMPARE,
  NODE_MATH_MULTIPLY_ADD,
  NODE_MATH_PINGPONG,
  NODE_MATH_SMOOTH_MIN,
  NODE_MATH_SMOOTH_MAX,
} NodeMathType;

typedef enum NodeVectorMathType {
  NODE_VECTOR_MATH_ADD,
  NODE_VECTOR_MATH_SUBTRACT,
  NODE_VECTOR_MATH_MULTIPLY,
  NODE_VECTOR_MATH_DIVIDE,

  NODE_VECTOR_MATH_CROSS_PRODUCT,
  NODE_VECTOR_MATH_PROJECT,
  NODE_VECTOR_MATH_REFLECT,
  NODE_VECTOR_MATH_DOT_PRODUCT,

  NODE_VECTOR_MATH_DISTANCE,
  NODE_VECTOR_MATH_LENGTH,
  NODE_VECTOR_MATH_SCALE,
  NODE_VECTOR_MATH_NORMALIZE,

  NODE_VECTOR_MATH_SNAP,
  NODE_VECTOR_MATH_FLOOR,
  NODE_VECTOR_MATH_CEIL,
  NODE_VECTOR_MATH_MODULO,
  NODE_VECTOR_MATH_FRACTION,
  NODE_VECTOR_MATH_ABSOLUTE,
  NODE_VECTOR_MATH_MINIMUM,
  NODE_VECTOR_MATH_MAXIMUM,
  NODE_VECTOR_MATH_WRAP,
  NODE_VECTOR_MATH_SINE,
  NODE_VECTOR_MATH_COSINE,
  NODE_VECTOR_MATH_TANGENT,
  NODE_VECTOR_MATH_REFRACT,
  NODE_VECTOR_MATH_FACEFORWARD,
  NODE_VECTOR_MATH_MULTIPLY_ADD,
} NodeVectorMathType;

typedef enum NodeClampType {
  NODE_CLAMP_MINMAX,
  NODE_CLAMP_RANGE,
} NodeClampType;

typedef enum NodeMapRangeType {
  NODE_MAP_RANGE_LINEAR,
  NODE_MAP_RANGE_STEPPED,
  NODE_MAP_RANGE_SMOOTHSTEP,
  NODE_MAP_RANGE_SMOOTHERSTEP,
} NodeMapRangeType;

typedef enum NodeMappingType {
  NODE_MAPPING_TYPE_POINT,
  NODE_MAPPING_TYPE_TEXTURE,
  NODE_MAPPING_TYPE_VECTOR,
  NODE_MAPPING_TYPE_NORMAL
} NodeMappingType;

typedef enum NodeVectorRotateType {
  NODE_VECTOR_ROTATE_TYPE_AXIS,
  NODE_VECTOR_ROTATE_TYPE_AXIS_X,
  NODE_VECTOR_ROTATE_TYPE_AXIS_Y,
  NODE_VECTOR_ROTATE_TYPE_AXIS_Z,
  NODE_VECTOR_ROTATE_TYPE_EULER_XYZ,
} NodeVectorRotateType;

typedef enum NodeVectorTransformType {
  NODE_VECTOR_TRANSFORM_TYPE_VECTOR,
  NODE_VECTOR_TRANSFORM_TYPE_POINT,
  NODE_VECTOR_TRANSFORM_TYPE_NORMAL
} NodeVectorTransformType;

typedef enum NodeVectorTransformConvertSpace {
  NODE_VECTOR_TRANSFORM_CONVERT_SPACE_WORLD,
  NODE_VECTOR_TRANSFORM_CONVERT_SPACE_OBJECT,
  NODE_VECTOR_TRANSFORM_CONVERT_SPACE_CAMERA
} NodeVectorTransformConvertSpace;

typedef enum NodeConvert {
  NODE_CONVERT_FV,
  NODE_CONVERT_FI,
  NODE_CONVERT_CF,
  NODE_CONVERT_CI,
  NODE_CONVERT_VF,
  NODE_CONVERT_VI,
  NODE_CONVERT_IF,
  NODE_CONVERT_IV
} NodeConvert;

typedef enum NodeMusgraveType {
  NODE_MUSGRAVE_MULTIFRACTAL,
  NODE_MUSGRAVE_FBM,
  NODE_MUSGRAVE_HYBRID_MULTIFRACTAL,
  NODE_MUSGRAVE_RIDGED_MULTIFRACTAL,
  NODE_MUSGRAVE_HETERO_TERRAIN
} NodeMusgraveType;

typedef enum NodeWaveType { NODE_WAVE_BANDS, NODE_WAVE_RINGS } NodeWaveType;

typedef enum NodeWaveBandsDirection {
  NODE_WAVE_BANDS_DIRECTION_X,
  NODE_WAVE_BANDS_DIRECTION_Y,
  NODE_WAVE_BANDS_DIRECTION_Z,
  NODE_WAVE_BANDS_DIRECTION_DIAGONAL
} NodeWaveBandsDirection;

typedef enum NodeWaveRingsDirection {
  NODE_WAVE_RINGS_DIRECTION_X,
  NODE_WAVE_RINGS_DIRECTION_Y,
  NODE_WAVE_RINGS_DIRECTION_Z,
  NODE_WAVE_RINGS_DIRECTION_SPHERICAL
} NodeWaveRingsDirection;

typedef enum NodeWaveProfile {
  NODE_WAVE_PROFILE_SIN,
  NODE_WAVE_PROFILE_SAW,
  NODE_WAVE_PROFILE_TRI,
} NodeWaveProfile;

typedef enum NodeSkyType { NODE_SKY_PREETHAM, NODE_SKY_HOSEK, NODE_SKY_NISHITA } NodeSkyType;

typedef enum NodeGradientType {
  NODE_BLEND_LINEAR,
  NODE_BLEND_QUADRATIC,
  NODE_BLEND_EASING,
  NODE_BLEND_DIAGONAL,
  NODE_BLEND_RADIAL,
  NODE_BLEND_QUADRATIC_SPHERE,
  NODE_BLEND_SPHERICAL
} NodeGradientType;

typedef enum NodeVoronoiDistanceMetric {
  NODE_VORONOI_EUCLIDEAN,
  NODE_VORONOI_MANHATTAN,
  NODE_VORONOI_CHEBYCHEV,
  NODE_VORONOI_MINKOWSKI,
} NodeVoronoiDistanceMetric;

typedef enum NodeVoronoiFeature {
  NODE_VORONOI_F1,
  NODE_VORONOI_F2,
  NODE_VORONOI_SMOOTH_F1,
  NODE_VORONOI_DISTANCE_TO_EDGE,
  NODE_VORONOI_N_SPHERE_RADIUS,
} NodeVoronoiFeature;

typedef enum NodeBlendWeightType {
  NODE_LAYER_WEIGHT_FRESNEL,
  NODE_LAYER_WEIGHT_FACING
} NodeBlendWeightType;

typedef enum NodeTangentDirectionType {
  NODE_TANGENT_RADIAL,
  NODE_TANGENT_UVMAP
} NodeTangentDirectionType;

typedef enum NodeTangentAxis {
  NODE_TANGENT_AXIS_X,
  NODE_TANGENT_AXIS_Y,
  NODE_TANGENT_AXIS_Z
} NodeTangentAxis;

typedef enum NodeNormalMapSpace {
  NODE_NORMAL_MAP_TANGENT,
  NODE_NORMAL_MAP_OBJECT,
  NODE_NORMAL_MAP_WORLD,
  NODE_NORMAL_MAP_BLENDER_OBJECT,
  NODE_NORMAL_MAP_BLENDER_WORLD,
} NodeNormalMapSpace;

typedef enum NodeImageProjection {
  NODE_IMAGE_PROJ_FLAT = 0,
  NODE_IMAGE_PROJ_BOX = 1,
  NODE_IMAGE_PROJ_SPHERE = 2,
  NODE_IMAGE_PROJ_TUBE = 3,
} NodeImageProjection;

typedef enum NodeImageFlags {
  NODE_IMAGE_COMPRESS_AS_SRGB = 1,
  NODE_IMAGE_ALPHA_UNASSOCIATE = 2,
} NodeImageFlags;

typedef enum NodeEnvironmentProjection {
  NODE_ENVIRONMENT_EQUIRECTANGULAR = 0,
  NODE_ENVIRONMENT_MIRROR_BALL = 1,
} NodeEnvironmentProjection;

typedef enum NodeBumpOffset {
  NODE_BUMP_OFFSET_CENTER,
  NODE_BUMP_OFFSET_DX,
  NODE_BUMP_OFFSET_DY,
} NodeBumpOffset;

typedef enum NodeTexVoxelSpace {
  NODE_TEX_VOXEL_SPACE_OBJECT = 0,
  NODE_TEX_VOXEL_SPACE_WORLD = 1,
} NodeTexVoxelSpace;

typedef enum NodeAO {
  NODE_AO_ONLY_LOCAL = (1 << 0),
  NODE_AO_INSIDE = (1 << 1),
  NODE_AO_GLOBAL_RADIUS = (1 << 2),
} NodeAO;

typedef enum ShaderType {
  SHADER_TYPE_SURFACE,
  SHADER_TYPE_VOLUME,
  SHADER_TYPE_DISPLACEMENT,
  SHADER_TYPE_BUMP,
} ShaderType;

typedef enum NodePrincipledHairParametrization {
  NODE_PRINCIPLED_HAIR_REFLECTANCE = 0,
  NODE_PRINCIPLED_HAIR_PIGMENT_CONCENTRATION = 1,
  NODE_PRINCIPLED_HAIR_DIRECT_ABSORPTION = 2,
  NODE_PRINCIPLED_HAIR_NUM,
} NodePrincipledHairParametrization;

/* Closure */

typedef enum ClosureType {
  /* Special type, flags generic node as a non-BSDF. */
  CLOSURE_NONE_ID,

  CLOSURE_BSDF_ID,

  /* Diffuse */
  CLOSURE_BSDF_DIFFUSE_ID,
  CLOSURE_BSDF_OREN_NAYAR_ID,
  CLOSURE_BSDF_DIFFUSE_RAMP_ID,
  CLOSURE_BSDF_PRINCIPLED_DIFFUSE_ID,
  CLOSURE_BSDF_PRINCIPLED_SHEEN_ID,
  CLOSURE_BSDF_DIFFUSE_TOON_ID,
  CLOSURE_BSDF_TRANSLUCENT_ID,

  /* Glossy */
  CLOSURE_BSDF_REFLECTION_ID,
  CLOSURE_BSDF_MICROFACET_GGX_ID,
  CLOSURE_BSDF_MICROFACET_GGX_FRESNEL_ID,
  CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID,
  CLOSURE_BSDF_MICROFACET_BECKMANN_ID,
  CLOSURE_BSDF_MICROFACET_MULTI_GGX_ID,
  CLOSURE_BSDF_MICROFACET_MULTI_GGX_FRESNEL_ID,
  CLOSURE_BSDF_ASHIKHMIN_SHIRLEY_ID,
  CLOSURE_BSDF_ASHIKHMIN_VELVET_ID,
  CLOSURE_BSDF_PHONG_RAMP_ID,
  CLOSURE_BSDF_GLOSSY_TOON_ID,
  CLOSURE_BSDF_HAIR_REFLECTION_ID,

  /* Transmission */
  CLOSURE_BSDF_REFRACTION_ID,
  CLOSURE_BSDF_MICROFACET_BECKMANN_REFRACTION_ID,
  CLOSURE_BSDF_MICROFACET_GGX_REFRACTION_ID,
  CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_ID,
  CLOSURE_BSDF_MICROFACET_BECKMANN_GLASS_ID,
  CLOSURE_BSDF_MICROFACET_GGX_GLASS_ID,
  CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_FRESNEL_ID,
  CLOSURE_BSDF_SHARP_GLASS_ID,
  CLOSURE_BSDF_HAIR_PRINCIPLED_ID,
  CLOSURE_BSDF_HAIR_TRANSMISSION_ID,

  /* Special cases */
  CLOSURE_BSDF_BSSRDF_ID,
  CLOSURE_BSDF_BSSRDF_PRINCIPLED_ID,
  CLOSURE_BSDF_TRANSPARENT_ID,

  /* BSSRDF */
  CLOSURE_BSSRDF_CUBIC_ID,
  CLOSURE_BSSRDF_GAUSSIAN_ID,
  CLOSURE_BSSRDF_PRINCIPLED_ID,
  CLOSURE_BSSRDF_BURLEY_ID,
  CLOSURE_BSSRDF_RANDOM_WALK_ID,
  CLOSURE_BSSRDF_PRINCIPLED_RANDOM_WALK_ID,

  /* Other */
  CLOSURE_HOLDOUT_ID,

  /* Volume */
  CLOSURE_VOLUME_ID,
  CLOSURE_VOLUME_ABSORPTION_ID,
  CLOSURE_VOLUME_HENYEY_GREENSTEIN_ID,

  CLOSURE_BSDF_PRINCIPLED_ID,

  NBUILTIN_CLOSURES
} ClosureType;

/* watch this, being lazy with memory usage */
#define CLOSURE_IS_BSDF(type) (type <= CLOSURE_BSDF_TRANSPARENT_ID)
#define CLOSURE_IS_BSDF_DIFFUSE(type) \
  (type >= CLOSURE_BSDF_DIFFUSE_ID && type <= CLOSURE_BSDF_TRANSLUCENT_ID)
#define CLOSURE_IS_BSDF_GLOSSY(type) \
  ((type >= CLOSURE_BSDF_REFLECTION_ID && type <= CLOSURE_BSDF_HAIR_REFLECTION_ID) || \
   (type == CLOSURE_BSDF_HAIR_PRINCIPLED_ID))
#define CLOSURE_IS_BSDF_TRANSMISSION(type) \
  (type >= CLOSURE_BSDF_REFRACTION_ID && type <= CLOSURE_BSDF_HAIR_TRANSMISSION_ID)
#define CLOSURE_IS_BSDF_BSSRDF(type) \
  (type == CLOSURE_BSDF_BSSRDF_ID || type == CLOSURE_BSDF_BSSRDF_PRINCIPLED_ID)
#define CLOSURE_IS_BSDF_SINGULAR(type) \
  (type == CLOSURE_BSDF_REFLECTION_ID || type == CLOSURE_BSDF_REFRACTION_ID || \
   type == CLOSURE_BSDF_TRANSPARENT_ID)
#define CLOSURE_IS_BSDF_TRANSPARENT(type) (type == CLOSURE_BSDF_TRANSPARENT_ID)
#define CLOSURE_IS_BSDF_MULTISCATTER(type) \
  (type == CLOSURE_BSDF_MICROFACET_MULTI_GGX_ID || \
   type == CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_ID)
#define CLOSURE_IS_BSDF_MICROFACET(type) \
  ((type >= CLOSURE_BSDF_MICROFACET_GGX_ID && type <= CLOSURE_BSDF_ASHIKHMIN_SHIRLEY_ID) || \
   (type >= CLOSURE_BSDF_MICROFACET_BECKMANN_REFRACTION_ID && \
    type <= CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_ID) || \
   (type == CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_FRESNEL_ID))
#define CLOSURE_IS_BSDF_MICROFACET_FRESNEL(type) \
  (type == CLOSURE_BSDF_MICROFACET_MULTI_GGX_FRESNEL_ID || \
   type == CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_FRESNEL_ID || \
   type == CLOSURE_BSDF_MICROFACET_GGX_FRESNEL_ID || \
   type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID)
#define CLOSURE_IS_BSDF_OR_BSSRDF(type) (type <= CLOSURE_BSSRDF_PRINCIPLED_RANDOM_WALK_ID)
#define CLOSURE_IS_BSSRDF(type) \
  (type >= CLOSURE_BSSRDF_CUBIC_ID && type <= CLOSURE_BSSRDF_PRINCIPLED_RANDOM_WALK_ID)
#define CLOSURE_IS_DISK_BSSRDF(type) \
  (type >= CLOSURE_BSSRDF_CUBIC_ID && type <= CLOSURE_BSSRDF_BURLEY_ID)
#define CLOSURE_IS_VOLUME(type) \
  (type >= CLOSURE_VOLUME_ID && type <= CLOSURE_VOLUME_HENYEY_GREENSTEIN_ID)
#define CLOSURE_IS_VOLUME_SCATTER(type) (type == CLOSURE_VOLUME_HENYEY_GREENSTEIN_ID)
#define CLOSURE_IS_VOLUME_ABSORPTION(type) (type == CLOSURE_VOLUME_ABSORPTION_ID)
#define CLOSURE_IS_HOLDOUT(type) (type == CLOSURE_HOLDOUT_ID)
#define CLOSURE_IS_PHASE(type) (type == CLOSURE_VOLUME_HENYEY_GREENSTEIN_ID)
#define CLOSURE_IS_GLASS(type) \
  (type >= CLOSURE_BSDF_MICROFACET_MULTI_GGX_GLASS_ID && type <= CLOSURE_BSDF_SHARP_GLASS_ID)
#define CLOSURE_IS_PRINCIPLED(type) (type == CLOSURE_BSDF_PRINCIPLED_ID)

#define CLOSURE_WEIGHT_CUTOFF 1e-5f

CCL_NAMESPACE_END

#endif /*  __SVM_TYPES_H__ */
