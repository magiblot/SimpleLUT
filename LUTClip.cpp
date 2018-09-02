#pragma warning (disable : 4100)

#include "windows.h"
#include "avisynth.h"
#include <vector>
#include <cmath>
#include <stdint.h>

#define PICK_TEMPLATE(srcBitDepth, dstBitDepth, template_function) \
  srcBitDepth == 8 ? \
    dstBitDepth == 8 ?  template_function <uint8_t,uint8_t> : \
    dstBitDepth == 32 ? template_function <uint8_t,uint32_t> : \
                        template_function <uint8_t,uint16_t> \
  : \
    dstBitDepth == 8 ?  template_function <uint16_t,uint8_t> : \
    dstBitDepth == 32 ? template_function <uint16_t,uint32_t> : \
                        template_function <uint16_t,uint16_t>

int bitDepthID(int bitDepth) {
  return bitDepth == 8 ? 0 : bitDepth == 32 ? 2 : 1;
}

int getPixelTypeAccordingToBitDepth(int generic_flag, int bitDepth) {
  switch (bitDepth) {
    case  8 : return generic_flag | VideoInfo::CS_Sample_Bits_8;
    case 10 : return generic_flag | VideoInfo::CS_Sample_Bits_10;
    case 12 : return generic_flag | VideoInfo::CS_Sample_Bits_12;
    case 14 : return generic_flag | VideoInfo::CS_Sample_Bits_14;
    case 16 : return generic_flag | VideoInfo::CS_Sample_Bits_16;
    case 32 : return generic_flag | VideoInfo::CS_Sample_Bits_32;
  }
  return 0;
}

std::vector<int> no_planes = {0},
                 planes_y = {PLANAR_Y},
                 planes_yuv = {PLANAR_Y, PLANAR_U, PLANAR_V},
                 planes_yuva = {PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A},
                 planes_rgb = {PLANAR_R, PLANAR_G, PLANAR_B},
                 planes_rgba = {PLANAR_R, PLANAR_G, PLANAR_B, PLANAR_A};

class LUTClip : public IClip {

private:
    
  VideoInfo vi;
  PVideoFrame frame;
  
  int width, height;
  int num_planes, num_values;
  std::vector<int> planes;
  int clip_id;
  
  template<typename pixel_t> void write_planar_lut() {
    int value_repetitions = (int) pow(num_values, clip_id);
    pixel_t mask = (pixel_t) (num_values - 1);
    for (int p = 0; p < num_planes; ++p) {
      pixel_t* ptr = (pixel_t*) frame->GetWritePtr(planes[p]);
      int x = 0;
      pixel_t value = 0;
      while (x < width) {
        for (int i = 0; i < value_repetitions; ++i)
          ptr[x++] = value;
        value = (value + 1)&mask;
      }
    }
  }
  
public:

    LUTClip(const char* plane_string, int dimensions, int bitDepth, int src_num, IScriptEnvironment* env) {
    
    if (dimensions == 2) {
      if(bitDepth > 12)
        env->ThrowError("LUTClip: Cannot prepare a 2D LUT for bit depths higher than 12");
      if(src_num < 1 || 2 < src_num)
        env->ThrowError("LUTClip: (2D) \"src_num\" must be either 1 or 2.");
    } else if (dimensions == 3) {
      if (bitDepth > 8)
        env->ThrowError("LUTClip: Cannot prepare a 3D LUT for bit depths higher than 8");
      if(src_num < 1 || 3 < src_num)
        env->ThrowError("LUTClip: (3D) \"src_num\" must be either 1, 2 or 3.");
    }
    
    if (bitDepth == 32)
      env->ThrowError("LUTClip: Only integer pixel types (with a bit depth of 8, 10, 12, 14 or 16)\nare supported.");
    if (bitDepth != 8 && bitDepth != 10 && bitDepth != 12 && bitDepth != 14 && bitDepth != 16)
      env->ThrowError("LUTClip: Invalid bit depth.");
    
    memset(&vi, 0, sizeof(VideoInfo));
    
    if (lstrcmpi(plane_string, "Y") == 0) {
      planes = planes_y;
      vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_Y, bitDepth);
    } else if (lstrcmpi(plane_string, "YUV") == 0) {
      planes = planes_yuv;
      vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_YUV444, bitDepth);
    } else if (lstrcmpi(plane_string, "YUVA") == 0) {
      planes = planes_yuva;
      vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_YUVA444, bitDepth);
    } else if (lstrcmpi(plane_string, "RGB") == 0) {
      planes = planes_rgb;
      vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_RGBP, bitDepth);
    } else if (lstrcmpi(plane_string, "RGBA") == 0) {
      planes = planes_rgba;
      vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_RGBAP, bitDepth);
    } else
      env->ThrowError("LUTClip: Invalid selection of planes.");
    
    num_planes = (int) planes.size();
    clip_id = dimensions == 1 ? 0 : src_num - 1;
    num_values = 1 << bitDepth;
    width = (int) pow(num_values, dimensions);
    height = 1;
    
    vi.width = width;
    vi.height = height;
    vi.fps_numerator = 24;
    vi.fps_denominator = 1;
    vi.num_frames = 1;
    
    frame = env->NewVideoFrame(vi);
    
    bitDepth == 8 ?  write_planar_lut<uint8_t>() :
    bitDepth == 32 ? write_planar_lut<uint32_t>() :
                     write_planar_lut<uint16_t>();
    
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  bool __stdcall GetParity(int n) { return false; }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  int __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
  
  static AVSValue __cdecl Create_LUTClip(AVSValue args, void*, IScriptEnvironment* env) {
    if (!args[0].Defined())
        env->ThrowError("LUTClip: Parameter \"planes\" was not given a value.");
    return new LUTClip(args[0].AsString(), args[1].AsInt(1), args[2].AsInt(8), args[3].AsInt(1), env);
  }
  
};

class ApplyLUT : public GenericVideoFilter {
  
private:
  
  std::vector<PClip> src_clips;
  PClip lut_clip;
  int mode;
  
  int num_src_clips, num_src_planes;
  std::vector<VideoInfo> vi_src;
  std::vector<std::vector<int>> src_planes, src_width, src_height;
  int srcBitDepth, dstBitDepth, src_pitch_bitshift, dst_pitch_bitshift;
  VideoInfo vi_lut;
  int num_lut_planes, lut_dimensions;
  PVideoFrame lut;
  
  int num_dst_planes;
  std::vector<int> dst_planes, dst_width, dst_height;
  
  // Pointer to wrapper function
  void(ApplyLUT::*wrapper_to_use) (std::vector<PVideoFrame>& src, PVideoFrame& dst);

  enum Condition {
    SRC_SAME_RES,
    SRC_NO_SUBSAMPLING,
    SRC_NOT_INTERLEAVED,
    DST_NOT_INTERLEAVED,
    DST_NOT_YUV_INTERLEAVED
  };
  
  static int pitchBitShift(int bitDepth) {
    return bitDepth == 8 ? 0 : bitDepth == 32 ? 2 : 1;
  }
  
  void fillSrcAndLutInfo(IScriptEnvironment* env) {
    
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
  
  bool conditionNotFulfilled(Condition cond) const {
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
  
  void takeFirstPlaneFromEachSource() {
    num_src_planes = 1;
  }
  
  int generateSubsampledPixelType(IScriptEnvironment* env) {
    
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
  
  void setDstFormatAndWrapperFunction(IScriptEnvironment* env) {
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
  
  void fillDstInfo() {
    
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
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_1plane_to_1plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    for (int dp = 0; dp < num_dst_planes; ++dp) {
      int sp = min(dp, num_src_planes),
          sc = min(dp, num_src_clips);
        
      int src_pitch = src[sc]->GetPitch(src_planes[sc][sp]) >> src_pitch_bitshift;
      const src_pixel_t* srcp = (const src_pixel_t*) src[sc]->GetReadPtr(src_planes[sc][sp]);
      const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
      int dst_pitch = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
      dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[dp]);
      
      write_1plane_to_1plane <src_pixel_t, dst_pixel_t> \
      (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[dp], dst_height[dp]);
    }
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_1plane_to_1plane \
  (int src_pitch, const src_pixel_t* srcp,
   int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
   int width, int height) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x)
        dstp[x] = lutp[srcp[x]];
        srcp += src_pitch;
        dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_1plane_to_3plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    
    int src_pitch = src[0]->GetPitch(src_planes[0][0]) >> src_pitch_bitshift;
    const src_pixel_t* srcp = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[0][0]);
    
    const dst_pixel_t* lutp[3];
    int dst_pitch[3];
    dst_pixel_t* dstp[3];
    
    for (int dp = 0; dp < num_dst_planes; ++dp) {
      lutp[dp] = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
      dst_pitch[dp] = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
      dstp[dp] = (dst_pixel_t*) dst->GetWritePtr(dst_planes[dp]);
    }
    
    write_1plane_to_3plane <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_1plane_to_3plane \
  (int src_pitch, const src_pixel_t* srcp,
   int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
   int width, int height) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int s = srcp[x];
        dstp[0][x] = lutp[0][s];
        dstp[1][x] = lutp[1][s];
        dstp[2][x] = lutp[2][s];
      }
      srcp += src_pitch;
      dstp[0] += dst_pitch[0];
      dstp[1] += dst_pitch[1];
      dstp[2] += dst_pitch[2];
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_1plane_to_3plane_packed_rgb_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    
    int src_pitch = src[0]->GetPitch(src_planes[0][0]) >> src_pitch_bitshift;
    const src_pixel_t* srcp = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[0][0]);
    
    int dst_pitch = dst->GetPitch() >> dst_pitch_bitshift;
    const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr();
    dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr();
    
    write_1plane_to_3plane_packed_rgb <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_1plane_to_3plane_packed_rgb \
  (int src_pitch, const src_pixel_t* srcp,
   int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
   int width, int height) {
    
    srcp += height * src_pitch;
    for (int y = 0; y < height; ++y) {
      srcp -= src_pitch;
      for (int x = 0; x < width; ++x) {
        int s = srcp[x];
        int strideBGRdst = x*3, strideBGRlut = s*3;
        dstp[strideBGRdst    ] = lutp[strideBGRlut    ];
        dstp[strideBGRdst + 1] = lutp[strideBGRlut + 1];
        dstp[strideBGRdst + 2] = lutp[strideBGRlut + 2];
      }
      dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_1plane_to_3plane_packed_rgba_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    
    int src_pitch = src[0]->GetPitch(src_planes[0][0]) >> src_pitch_bitshift;
    const src_pixel_t* srcp = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[0][0]);
    
    int dst_pitch = dst->GetPitch() >> dst_pitch_bitshift;
    const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr();
    dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr();
    
    write_1plane_to_3plane_packed_rgba <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_1plane_to_3plane_packed_rgba \
  (int src_pitch, const src_pixel_t* srcp,
   int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
   int width, int height) {
    
    srcp += height * src_pitch;
    for (int y = 0; y < height; ++y) {
      srcp -= src_pitch;
      for (int x = 0; x < width; ++x) {
        int s = srcp[x];
        int strideBGRdst = x << 2, strideBGRlut = s << 2;
        dstp[strideBGRdst    ] = lutp[strideBGRlut    ];
        dstp[strideBGRdst + 1] = lutp[strideBGRlut + 1];
        dstp[strideBGRdst + 2] = lutp[strideBGRlut + 2];
      }
      dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_2plane_to_1plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    for (int dp = 0; dp < num_dst_planes; ++dp) {
      
      const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
      int dst_pitch = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
      dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[dp]);
      
      int src_pitch[2];
      const src_pixel_t* srcp[2];
      
      for (int i = 0; i < 2; ++i) {
        int sc = min(i, num_src_clips),
            sp = min(dp, num_src_planes);
        src_pitch[i] = src[sc]->GetPitch(src_planes[sc][sp]) >> src_pitch_bitshift;
        srcp[i] = (const src_pixel_t*) src[sc]->GetReadPtr(src_planes[sc][sp]);
      }

      write_2plane_to_1plane <src_pixel_t, dst_pixel_t>
      (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[dp], dst_height[dp], srcBitDepth);
      
    }
  }
  
  template <typename src_pixel_t, typename dst_pixel_t>
  static void write_2plane_to_1plane
  (int src_pitch[2], const src_pixel_t* srcp[2],
   int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
   int width, int height, int srcBitDepth) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x];
        dstp[x] = lutp[(b << srcBitDepth) + a];
      }
      srcp[0] += src_pitch[0];
      srcp[1] += src_pitch[1];
      dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_2plane_to_3plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    
    int src_pitch[2];
    const src_pixel_t* srcp[2];
    
    for (int i = 0; i < 2; ++i) {
      int sc = min(i, num_src_clips);
      src_pitch[i] = src[sc]->GetPitch(src_planes[sc][0]) >> src_pitch_bitshift;
      srcp[i] = (const src_pixel_t*)src[sc]->GetReadPtr(src_planes[sc][0]);
    }

    const dst_pixel_t* lutp[3];
    int dst_pitch[3];
    dst_pixel_t* dstp[3];
    
    for (int dp = 0; dp < num_dst_planes; ++dp) {
      lutp[dp] = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
      dst_pitch[dp] = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
      dstp[dp] = (dst_pixel_t*) dst->GetWritePtr(dst_planes[dp]);
    }
    
    write_2plane_to_3plane <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0], srcBitDepth);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_2plane_to_3plane \
  (int src_pitch[2], const src_pixel_t* srcp[2],
   int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
   int width, int height, int srcBitDepth) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x];
        int ab = (b << srcBitDepth) + a;
        dstp[0][x] = lutp[0][ab];
        dstp[1][x] = lutp[1][ab];
        dstp[2][x] = lutp[2][ab];
      }
      srcp[0] += src_pitch[0];
      srcp[1] += src_pitch[1];
      dstp[0] += dst_pitch[0];
      dstp[1] += dst_pitch[1];
      dstp[2] += dst_pitch[2];
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_3plane_to_1plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    for (int dp = 0; dp < num_dst_planes; ++dp) {
      
      int src_pitch[3];
      const src_pixel_t* srcp[3];
      
      for (int i = 0; i < 3; ++i) {
        int sc = min(i, num_src_clips),
            sp = min(num_src_clips == 1 ? i : dp, num_src_planes);
        src_pitch[i] = src[sc]->GetPitch(src_planes[sc][sp]) >> src_pitch_bitshift;
        srcp[i] = (const src_pixel_t*) src[sc]->GetReadPtr(src_planes[sc][sp]);
      }
      
      const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
      int dst_pitch = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
      dst_pixel_t* dstp = (dst_pixel_t*)dst->GetWritePtr(dst_planes[dp]);
      
      write_3plane_to_1plane <src_pixel_t, dst_pixel_t> \
      (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[dp], dst_height[dp]);
      
    }
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_3plane_to_1plane \
  (int src_pitch[3], const src_pixel_t* srcp[3],
   int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
   int width, int height) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x], c = srcp[2][x];
        dstp[x] = lutp[(((c << 8) + b) << 8) + a];
      }
      srcp[0] += src_pitch[0];
      srcp[1] += src_pitch[1];
      srcp[2] += src_pitch[2];
      dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_3plane_to_3plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) {
    
    int src_pitch[3];
    const src_pixel_t* srcp[3];
    
    for (int i = 0; i < 3; ++i) {
      int sc = min(i, num_src_clips),
          sp = min(num_src_clips == 1 ? i : 0, num_src_planes);
      src_pitch[i] = src[sc]->GetPitch(src_planes[sc][sp]) >> src_pitch_bitshift;
      srcp[i] = (const src_pixel_t*)src[sc]->GetReadPtr(src_planes[sc][sp]);
    }
    
    const dst_pixel_t* lutp[3];
    int dst_pitch[3];
    dst_pixel_t* dstp[3];
    
    for (int p = 0; p < num_dst_planes; ++p) {
      lutp[p] = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
      dst_pitch[p] = dst->GetPitch(dst_planes[p]) >> dst_pitch_bitshift;
      dstp[p] = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p]);
    }
    
    write_3plane_to_3plane <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_3plane_to_3plane \
  (int src_pitch[3], const src_pixel_t* srcp[3],
   int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
   int width, int height) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x], c = srcp[2][x];
        int abc = (((c << 8) + b) << 8) + a;
        dstp[0][x] = lutp[0][abc];
        dstp[1][x] = lutp[1][abc];
        dstp[2][x] = lutp[2][abc];
      }
      srcp[0] += src_pitch[0];
      srcp[1] += src_pitch[1];
      srcp[2] += src_pitch[2];
      dstp[0] += dst_pitch[0];
      dstp[1] += dst_pitch[1];
      dstp[2] += dst_pitch[2];
    }
    
  }
  
  void Testing(IScriptEnvironment* env) {
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
  
public:
  
  ApplyLUT(PClip _child, std::vector<PClip> _src_clips, PClip _lut_clip, int _mode, IScriptEnvironment* env) : GenericVideoFilter(_child), src_clips(_src_clips), lut_clip(_lut_clip), mode(_mode) {
    
    fillSrcAndLutInfo(env);
    setDstFormatAndWrapperFunction(env);
    fillDstInfo();
    Testing(env);
  
  }
  
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
    
    std::vector<PVideoFrame> src(num_src_clips);
    for (int sc = 0; sc < num_src_clips; ++sc)
      src[sc] = src_clips[sc]->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);
    
    (this->*wrapper_to_use)(src, dst);
    
    return dst;
    
  }
  
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    
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
  
};

const AVS_Linkage *AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, AVS_Linkage* vectors)
{
  AVS_linkage = vectors;
  env->AddFunction("LUTClip", "[planes]s[dimensions]i[bit_depth]i[src_num]i", LUTClip::Create_LUTClip, 0);
  env->AddFunction("ApplyLUT", "c*[mode]i", ApplyLUT::Create, 0);
  return 0;
}
