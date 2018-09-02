#include "SimpleLUT.hpp"

int ApplyLUT::pitchBitShift(int bitDepth) {
  return bitDepth == 8 ? 0 : bitDepth == 32 ? 2 : 1;
}

PVideoFrame __stdcall ApplyLUT::GetFrame(int n, IScriptEnvironment* env) {
  
  std::vector<PVideoFrame> src(num_src_clips);
  for (int sc = 0; sc < num_src_clips; ++sc)
    src[sc] = src_clips[sc]->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  
  (this->*wrapper_to_use)(src, dst);
  
  return dst;
  
}

int __stdcall ApplyLUT::SetCacheHints(int cachehints,int frame_range) {
  return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}
  
AVSValue __cdecl ApplyLUT::Create(AVSValue args, void*, IScriptEnvironment* env) {
  
  int num_src_clips = args[0].ArraySize() - 1;
  if (num_src_clips < 1 || 3 < num_src_clips)
    env->ThrowError("ApplyLUT: From 2 to 4 input clips must be provided, the last one being a LUT clip.");
  
  std::vector<PClip> src_clips(num_src_clips);
  for (int i = 0; i < num_src_clips; ++i)
    src_clips[i] = args[0][i].AsClip();
  
  PClip lut_clip = args[0][num_src_clips].AsClip();
  
  int mode = args[1].AsInt();
  if (mode < 1 || 6 < mode)
    env->ThrowError("ApplyLUT: \"mode\" must be an integer from 1 to 6.");
  
  return new ApplyLUT(src_clips[0], src_clips, lut_clip, mode, env);
}

void ApplyLUT::constructorTesting(IScriptEnvironment* env) {
  if (num_src_clips == 1 && vi_src[0].pixel_type == VideoInfo::CS_RGBP && vi_lut.pixel_type == VideoInfo::CS_RGBP && mode == 1
      && vi_src[0].width == 640 && vi_src[0].height == 480 && vi_lut.width == 256) {
    if (num_src_planes != 3)
      env->ThrowError("ApplyLUT.Testing: num_src_planes should be 3, is %d.", num_src_planes);
    if (src_planes[0] != planes_rgb)
      env->ThrowError("ApplyLUT.Testing: src_planes[0] does not equal planes_rgb.");
    if (src_width[0] != std::vector<int>(3, 640))
      env->ThrowError("ApplyLUT.Testing: src_width should be 640 for all planes, is {%d, %d, %d}.", src_width[0][0], src_width[0][1], src_width[0][2]);
    if (src_height[0] != std::vector<int>(3, 480))
      env->ThrowError("ApplyLUT.Testing: src_height should be 480 for all planes, is {%d, %d, %d}.", src_height[0][0], src_height[0][1], src_height[0][2]);
    if (srcBitDepth != 8)
      env->ThrowError("ApplyLUT.Testing: srcBitDepth should be 8, is %d.", srcBitDepth);
    if (dstBitDepth != 8)
      env->ThrowError("ApplyLUT.Testing: dstBitDepth should be 8, is %d.", dstBitDepth);
    if (src_pitch_bitshift != 0)
      env->ThrowError("ApplyLUT.Testing: src_pitch_bitshift should be 0, is %d.", src_pitch_bitshift);
    if (dst_pitch_bitshift != 0)
      env->ThrowError("ApplyLUT.Testing: dst_pitch_bitshift should be 0, is %d.", dst_pitch_bitshift);
    if (num_lut_planes != 3)
      env->ThrowError("ApplyLUT.Testing: num_lut_planes should be 3, is %d.", num_lut_planes);
    if (lut_dimensions != 1)
      env->ThrowError("ApplyLUT.Testing: lut_dimensions should be 1, is %d.", lut_dimensions);
    if (num_dst_planes != 3)
      env->ThrowError("ApplyLUT.Testing: num_dst_planes should be 3, is %d.", num_dst_planes);
    if (dst_planes != planes_rgb)
      env->ThrowError("ApplyLUT.Testing: dst_planes does not equal planes_rgb.");
    if (dst_width != std::vector<int>(3, 640))
      env->ThrowError("ApplyLUT.Testing: dst_width should be 640 for all planes, is {%d, %d, %d}.", dst_width[0], dst_width[1], dst_width[2]);
    if (dst_height != std::vector<int>(3,480))
      env->ThrowError("ApplyLUT.Testing: dst_height should be 480 for all planes, is {%d, %d, %d}.", dst_height[0], dst_height[1], dst_height[2]);
    env->ThrowError("ApplyLUT.Testing: Test passed.");
  } else
    env->ThrowError("ApplyLUT.Testing: No test case matched!");
}
