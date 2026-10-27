// Execute the donor's unchanged Camera_Demo4/5 bodies. Only MM's camera ABI,
// setting lookup, geometric helpers and distortion service are adapted here.
// The expected shot must never be reconstructed from the candidate controller.
namespace MmCameraReference {
using VecGeo = VecSph;
using CameraModeValue =
    s16; // Setting values only supply the out-of-scope HUD flags.
/* DONOR_CAMERA_TYPES */
struct Camera {
  Vec3f at, eye, eyeNext;
  f32 fov;
  s16 roll, animState, setting, mode;
  Actor *focalActor;
  PosRot focalActorPosRot;
  struct {
    Demo4 demo4;
    Demo5 demo5;
  } paramData;
};
static struct {
  struct {
    CameraModeValue *values;
  } cameraModes[1];
} sCameraSettings[1];
static s16 sCameraInterfaceFlags;
enum { DISTORTION_TYPE_MASK_TRANSFORM_1, DISTORTION_TYPE_MASK_TRANSFORM_2 };
static void Distortion_Request(int) {}
static void Distortion_SetDuration(int) {}
static void Distortion_RemoveRequest(int) {}
static PosRot Actor_GetFocus(Actor *actor) { return actor->focus; }
static f32 Camera_ScaledStepToCeilF(f32 target, f32 value, f32 step,
                                    f32 epsilon) {
  return Camera_LERPCeilF(target, value, step, epsilon);
}
static s16 Camera_ScaledStepToCeilS(s16 target, s16 value, f32 step,
                                    s16 epsilon) {
  return Camera_LERPCeilS(target, value, step, epsilon);
}
static void Camera_ScaledStepToCeilVec3f(Vec3f *target, Vec3f *value, f32 y,
                                         f32 xz, f32 epsilon) {
  Camera_LERPCeilVec3f(target, value, y, xz, epsilon);
}
static VecGeo OLib_Vec3fDiffToVecGeo(Vec3f *at, Vec3f *eye) {
  VecGeo result;
  OLib_Vec3fDiffToVecSphGeo(&result, at, eye);
  return result;
}
static Vec3f OLib_AddVecGeoToVec3f(Vec3f *at, VecGeo *offset) {
  Vec3f result;
  OLib_VecSphGeoToVec3f(&result, offset);
  result.x += at->x;
  result.y += at->y;
  result.z += at->z;
  return result;
}
static f32 Math_SinF(f32 value) { return std::sin(value); }
#define RELOAD_PARAMS(camera) ((camera)->animState == 0)
#define GET_NEXT_RO_DATA(values) 0
#define CAM_DEG_TO_BINANG(degrees) (s16)((degrees) * ((f32)0xFFFF / 360) + .5f)
/* DONOR_CAMERA_FUNCTIONS */
#undef RELOAD_PARAMS
#undef GET_NEXT_RO_DATA
#undef CAM_DEG_TO_BINANG
} // namespace MmCameraReference
