#include "SimpleLUT.hpp"

ApplyLUT::ApplyLUT(PClip _child, std::vector<PClip> _src_clips, PClip _lut_clip, int _mode, IScriptEnvironment* env) : GenericVideoFilter(_child), src_clips(_src_clips), lut_clip(_lut_clip), mode(_mode) {
  
  fillSrcAndLutInfo(env);
  setDstFormatAndWrapperFunction(env);
  fillDstInfo();
#ifdef ENABLE_CONSTRUCTOR_TESTING
  constructorTesting(env);
#endif
  
}
  
void ApplyLUT::fillSrcAndLutInfo(IScriptEnvironment* env) {
  
  num_src_clips = (int) src_clips.size();
  vi_src = std::vector<VideoInfo> (num_src_clips);
  src_planes = std::vector<std::vector<int>> (num_src_clips);
  int min_num_planes = 3;
  for (int sc = 0; sc < num_src_clips; ++sc) {
    vi_src[sc] = src_clips[sc]->GetVideoInfo();
    src_planes[sc] = vi_src[sc].pixel_type&VideoInfo::CS_INTERLEAVED ? no_planes
                    : vi_src[sc].IsY() ? planes_y
                    : vi_src[sc].IsYUV() ? planes_yuv
                    : vi_src[sc].IsYUVA() ? planes_yuva
                    : vi_src[sc].IsPlanarRGB() ? planes_rgb
                    : planes_rgba;
    min_num_planes = min((int) src_planes[sc].size(), min_num_planes);
  }
  num_src_planes = min_num_planes;
  
  src_width = std::vector<std::vector<int>> (num_src_clips, std::vector<int>(num_src_planes));
  src_height = std::vector<std::vector<int>> (num_src_clips, std::vector<int>(num_src_planes));
  for (int sc = 0; sc < num_src_clips; ++sc) {
    src_width[sc][0] = vi_src[sc].width;
    src_height[sc][0] = vi_src[sc].height;
    for (int sp = 1; sp < num_src_planes; ++sp) {
      src_width[sc][sp] = src_width[sc][0] >> vi_src[sc].VideoInfo::GetPlaneWidthSubsampling(src_planes[sc][sp]);
      src_height[sc][sp] = src_height[sc][0] >> vi_src[sc].VideoInfo::GetPlaneHeightSubsampling(src_planes[sc][sp]);
    }
  }
  
  srcBitDepth = vi_src[0].BitsPerComponent();
  for (int sc = 1; sc < num_src_clips; ++sc)
    if (srcBitDepth != vi_src[sc].BitsPerComponent())
      env->ThrowError("ApplyLUT: All source clips must have the same bit depth.");
  
  vi_lut = lut_clip->GetVideoInfo();
  if (!(vi_lut.Is444() || vi_lut.IsRGB() || vi_lut.IsY()))
  env->ThrowError("ApplyLUT: The LUT clip can not be subsampled.");
  
  int src_num_values = 1 << srcBitDepth;
  double lut_dimensions_d = log(vi_lut.width)/log(src_num_values);
  lut_dimensions = (int) lut_dimensions_d;
  if (lut_dimensions_d - lut_dimensions != 0.0)
    env->ThrowError("ApplyLUT: The provided LUT clip was expected to have a width of %d pixels,\ndue to the source bit depth being %d, but got %d pixels instead.", src_num_values, srcBitDepth, vi_lut.width);
  num_lut_planes = (int) (vi_lut.pixel_type&VideoInfo::CS_INTERLEAVED ? no_planes.size()
                        : vi_lut.IsY() ? planes_y.size()
                        : vi_lut.IsYUV()  ? planes_yuv.size()
                        : vi_lut.IsYUVA()  ? planes_yuva.size()
                        : vi_lut.IsPlanarRGB() ? planes_rgb.size()
                        : planes_rgba.size());
  
  dstBitDepth = vi_lut.BitsPerComponent();    
  src_pitch_bitshift = pitchBitShift(srcBitDepth);
  dst_pitch_bitshift = pitchBitShift(dstBitDepth);
  
  lut = lut_clip->GetFrame(0, env);
}

bool ApplyLUT::conditionNotFulfilled(Condition cond) const {
  switch(cond) {
    case SRC_SAME_RES:
      for (int c = 1; c < num_src_clips; ++c)
        if (src_width[0][0] != src_width[c][0] || src_height[0][0] != src_height[c][0])
          return true;
      break;
    case SRC_NO_SUBSAMPLING:
      for (int c = 0; c < num_src_clips; ++c)
          if (!(vi_src[c].Is444() || vi_src[c].IsRGB() || vi_src[c].IsY()))
          return true;
      break;
    case SRC_NOT_INTERLEAVED:
      for (int c = 0; c < num_src_clips; ++c)
        if (vi_src[c].pixel_type&VideoInfo::CS_INTERLEAVED && !vi_src[c].IsY())
          return true;
      break;
    case DST_NOT_INTERLEAVED:
      if (vi_lut.pixel_type&VideoInfo::CS_INTERLEAVED && !vi_lut.IsY())
        return true;
      break;
    case DST_NOT_YUV_INTERLEAVED:
      if (vi_lut.pixel_type&(VideoInfo::CS_INTERLEAVED|VideoInfo::CS_YUV|VideoInfo::CS_YUVA) && !vi_lut.IsY())
        return true;
      break;
  }
  return false;
}

void ApplyLUT::takeFirstPlaneFromEachSource() {
  num_src_planes = 1;
}

int ApplyLUT::generateSubsampledPixelType(IScriptEnvironment* env) {
  
  if (num_src_clips == 1) {
    if (num_src_planes == 1 || vi_src[0].Is444() || vi_src[0].IsRGB())
      return vi_lut.pixel_type;
    else if (vi_lut.IsRGB())
      env->ThrowError("ApplyLUT: Can't use all the planes of a subsampled YUV source\nwhen the LUT clip is RGB.");
    return vi_src[0].pixel_type;
  } else if (num_src_clips == 3 && num_src_planes == 1) {
    if (src_width[0][0] == src_width[1][0] && src_width[1][0] == src_width[2][0]
      && src_height[0][0] == src_height[1][0] && src_height[1][0] == src_height[2][0])
      return vi_lut.pixel_type;
    if (vi_lut.IsRGB())
      env->ThrowError("ApplyLUT: The LUT clip is RGB, so all source clips must have the same resolution.");
    if (src_width[1][0] != src_width[2][0] || src_height[1][0] != src_height[2][0])
      env->ThrowError("ApplyLUT: The 2nd and 3rd source clips, which will be taken as U and V planes,\nmust have the same resolution.");
    
    // CS_Sub_Width_2 and CS_Sub_Height_2 are 0, pixel_type bitfield can be or'd if change needed
    int generic_flag = vi_lut.IsYUVA() ? VideoInfo::CS_GENERIC_YUVA420 : VideoInfo::CS_GENERIC_YUV420;
    int pixel_type = getPixelTypeAccordingToBitDepth(generic_flag, dstBitDepth);

    if (src_width[0][0] == src_width[1][0])
      pixel_type |= VideoInfo::CS_Sub_Width_1;
    else if (src_width[0][0] == 2*src_width[1][0])
      pixel_type |= VideoInfo::CS_Sub_Width_2;
    else if (src_width[0][0] == 4*src_width[1][0])
      pixel_type |= VideoInfo::CS_Sub_Width_4;
    else
      env->ThrowError("ApplyLUT: Could not produce a subsampled color format\nfrom the width of the 2nd and 3rd source clips.");

    if (src_height[0][0] == src_height[1][0])
      pixel_type |= VideoInfo::CS_Sub_Height_1;
    else if (src_height[0][0] == 2*src_height[1][0])
      pixel_type |= VideoInfo::CS_Sub_Height_2;
    else if (src_height[0][0] == 4*src_height[1][0])
      pixel_type |= VideoInfo::CS_Sub_Height_4;
    else
      env->ThrowError("ApplyLUT: Could not produce a subsampled color format\nfrom the height of the 2nd and 3rd source clips.");
    
    return pixel_type;
  } else {
    for (int sp = 0; sp < num_src_planes; ++sp) {
      for (int sc = 1; sc < num_src_clips; ++sc)
        if (src_width[sc][sp] != src_width[0][sp])
          env->ThrowError("ApplyLUT: In order to produce a subsampled output,\nall source clips must have the same subsampling ratio.");
    }
    return vi_src[0].pixel_type;
  }
}

void ApplyLUT::setDstFormatAndWrapperFunction(IScriptEnvironment* env) {
  switch(mode) {
    case 1:
      if (lut_dimensions != 1)
        env->ThrowError("ApplyLUT: Mode 1 requires a 1D LUT clip, but got a %dD one instead.", lut_dimensions);
      if (num_src_clips != 1 && num_src_clips != 3)
        env->ThrowError("ApplyLUT: Mode 1 requires either 1 or 3 source clips, but got %d instead.", num_src_clips);
      if (conditionNotFulfilled(SRC_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 1 doesn't support source clips with interleaved format.");
      if (conditionNotFulfilled(DST_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 1 doesn't support an interleaved destination format.");
      if (num_lut_planes == 1) {
        if (num_src_clips != 1)
          env->ThrowError("ApplyLUT: In mode 1, when the LUT clip is Y, there can only be 1 source clip, but got %d instead.", num_src_clips);
        takeFirstPlaneFromEachSource();
        vi.pixel_type = vi_lut.pixel_type;
      } else
        vi.pixel_type = generateSubsampledPixelType(env);
      wrapper_to_use = PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_1plane_to_1plane_wrapper);
      break;
    case 2:
      if (lut_dimensions != 1)
        env->ThrowError("ApplyLUT: Mode 2 requires a 1D LUT clip, but got a %dD one instead.", lut_dimensions);
      if (num_lut_planes < 3)
        env->ThrowError("ApplyLUT: In mode 2, the LUT clip cannot have only 1 plane (Y).");
      if (num_src_clips != 1)
        env->ThrowError("ApplyLUT: Mode 2 can only accept 1 source clip, but got %d instead.", num_src_clips);
      if (conditionNotFulfilled(SRC_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 2 doesn't support source clips with interleaved format.");
      if (conditionNotFulfilled(DST_NOT_YUV_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 2 doesn't support YUV interleaved destination formats.");
      takeFirstPlaneFromEachSource();
      vi.pixel_type = vi_lut.pixel_type;
      wrapper_to_use = vi.pixel_type&(VideoInfo::CS_BGR24|VideoInfo::CS_BGR48) ?
                        PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_1plane_to_3plane_packed_rgb_wrapper)
                      : vi.pixel_type&(VideoInfo::CS_BGR32|VideoInfo::CS_BGR64) ?
                        PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_1plane_to_3plane_packed_rgba_wrapper)
                      : PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_1plane_to_3plane_wrapper);
      break;
    case 3:
      if (lut_dimensions != 2)
        env->ThrowError("ApplyLUT: Mode 3 requires a 2D LUT clip, but got a %dD one instead.", lut_dimensions);
      if (num_src_clips != 2)
          env->ThrowError("ApplyLUT: Mode 3 needs exactly 2 source clips, but got %d instead.", num_src_clips);
      if (conditionNotFulfilled(SRC_SAME_RES))
          env->ThrowError("ApplyLUT: Mode 3 requires all source clips to have the same resolution.");
      if (conditionNotFulfilled(SRC_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 3 doesn't support source clips with interleaved format.");              
      if (conditionNotFulfilled(DST_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 3 doesn't support an interleaved destination format.");
      if (num_lut_planes == 1) {
        takeFirstPlaneFromEachSource();
        vi.pixel_type = vi_lut.pixel_type;
      } else
        vi.pixel_type = generateSubsampledPixelType(env);
      wrapper_to_use = PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_2plane_to_1plane_wrapper);
      break;
    case 4:
      if (lut_dimensions != 2)
        env->ThrowError("ApplyLUT: Mode 4 requires a 2D LUT clip, but got a %dD one instead.", lut_dimensions);
      if (num_lut_planes < 3)
        env->ThrowError("ApplyLUT: In mode 4, the LUT clip cannot have only 1 plane (Y).");
      if (num_src_clips != 2)
        env->ThrowError("ApplyLUT: Mode 4 needs exactly 2 source clips, but got %d instead.", num_src_clips);
      if (conditionNotFulfilled(SRC_SAME_RES))
        env->ThrowError("ApplyLUT: Mode 4 requires all source clips to have the same resolution.");
      if (conditionNotFulfilled(SRC_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 4 doesn't support source clips with interleaved format.");
      if (conditionNotFulfilled(DST_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 4 doesn't support an interleaved destination format.");
      takeFirstPlaneFromEachSource();
      vi.pixel_type = vi_lut.pixel_type;
      wrapper_to_use = PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_2plane_to_3plane_wrapper);
      break;
    case 5: 
      if (lut_dimensions != 3)
        env->ThrowError("ApplyLUT: Mode 5 requires a 3D LUT clip, but got a %dD one instead.", lut_dimensions);
      if (num_src_clips != 1 && num_src_clips != 3)
        env->ThrowError("ApplyLUT: Mode 5 requires either 1 or 3 source clips, but got %d instead.", num_src_clips);
      if (num_src_clips == 1 && conditionNotFulfilled(SRC_NO_SUBSAMPLING))
          env->ThrowError("ApplyLUT: In mode 5, when there is only one source clip, it cannot be subsampled.");
      if (conditionNotFulfilled(SRC_SAME_RES))
        env->ThrowError("ApplyLUT: Mode 5 requires all source clips to have the same resolution.");
      if (conditionNotFulfilled(SRC_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 5 doesn't support source clips with interleaved format.");
      if (conditionNotFulfilled(DST_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 5 doesn't support an interleaved destination format.");
      if (num_lut_planes == 1) {
        if (num_src_clips == 3)
          takeFirstPlaneFromEachSource();
        vi.pixel_type = vi_lut.pixel_type;
      } else if (num_src_clips == 1 || num_src_planes == 1)
        vi.pixel_type = vi_lut.pixel_type;
      else // num_src_clips == 3 && num_src_planes == 3
        vi.pixel_type = generateSubsampledPixelType(env);
      wrapper_to_use = PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_3plane_to_1plane_wrapper);
      break;
    case 6:
      if (lut_dimensions != 3)
        env->ThrowError("ApplyLUT: Mode 6 requires a 3D LUT clip, but got a %dD one instead.", lut_dimensions);
      if (num_lut_planes < 3)
        env->ThrowError("ApplyLUT: In mode 6, the LUT clip cannot have only 1 plane (Y).");
      if (num_src_clips != 1 && num_src_clips != 3)
        env->ThrowError("ApplyLUT: Mode 6 requires either 1 or 3 source clips, but got %d instead.", num_src_clips);
      if (conditionNotFulfilled(SRC_SAME_RES))
          env->ThrowError("ApplyLUT: Mode 6 requires all source clips to have the same resolution.");
      if (conditionNotFulfilled(SRC_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 6 doesn't support source clips with interleaved format.");
      if (conditionNotFulfilled(DST_NOT_INTERLEAVED))
        env->ThrowError("ApplyLUT: Mode 6 doesn't support an interleaved destination format.");
      if (num_src_clips == 1) {
        if (num_src_planes < 3)
          env->ThrowError("ApplyLUT: In mode 6, when there is only one source clip,\nit cannot have only one plane (Y).");
        if (conditionNotFulfilled(SRC_NO_SUBSAMPLING))
          env->ThrowError("ApplyLUT: In mode 6, when there is only one source clip,\nit cannot be subsampled.");
      } else // num_src_clips == 3
        takeFirstPlaneFromEachSource();
      vi.pixel_type = vi_lut.pixel_type;
      wrapper_to_use = PICK_TEMPLATE(srcBitDepth, dstBitDepth, &ApplyLUT::write_3plane_to_3plane_wrapper);
      break;
  }
}

void ApplyLUT::fillDstInfo() {
    
  dst_planes = vi.pixel_type&VideoInfo::CS_INTERLEAVED ? no_planes
              : vi.IsY() ? planes_y
              : vi.IsYUV()  ? planes_yuv
              : vi.IsYUVA()  ? planes_yuva
              : vi.IsPlanarRGB() ? planes_rgb
              : planes_rgba;
  num_dst_planes = (int) dst_planes.size();
  
  dst_width = std::vector<int>(num_dst_planes);
  dst_height = std::vector<int>(num_dst_planes);
  dst_width[0] = vi.width;
  dst_height[0] = vi.height;
  for (int dp = 1; dp < num_dst_planes; ++dp) {
    dst_width[dp] = dst_width[0] >> vi.VideoInfo::GetPlaneWidthSubsampling(dst_planes[dp]);
    dst_height[dp] = dst_height[0] >> vi.VideoInfo::GetPlaneHeightSubsampling(dst_planes[dp]);
  }
  
}
  
