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

int bitDepthID(int bitDepth);

int getPixelTypeAccordingToBitDepth(int generic_flag, int bitDepth);

class LUTClip : public IClip {

private:
    
  VideoInfo vi;
  PVideoFrame frame;
  
  int width, height;
  int num_planes, num_values;
  std::vector<int> planes;
  int clip_id;
  
  const std::vector<int>
  planes_y = {PLANAR_Y},
  planes_yuv = {PLANAR_Y, PLANAR_U, PLANAR_V},
  planes_yuva = {PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A},
  planes_rgb = {PLANAR_R, PLANAR_G, PLANAR_B},
  planes_rgba = {PLANAR_R, PLANAR_G, PLANAR_B, PLANAR_A};
  
  template<typename pixel_t> void write_planar_lut() const;
  
public:

    LUTClip(const char* plane_string, int dimensions, int bitDepth, int src_num, IScriptEnvironment* env);

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);
  const VideoInfo& __stdcall GetVideoInfo();
  int __stdcall SetCacheHints(int cachehints,int frame_range);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  
  static AVSValue __cdecl Create_LUTClip(AVSValue args, void*, IScriptEnvironment* env);
  
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
  void(ApplyLUT::*wrapper_to_use) (std::vector<PVideoFrame>& src, PVideoFrame& dst) const;

  const std::vector<int>
  no_planes = {0},
  planes_y = {PLANAR_Y},
  planes_yuv = {PLANAR_Y, PLANAR_U, PLANAR_V},
  planes_yuva = {PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A},
  planes_rgb = {PLANAR_R, PLANAR_G, PLANAR_B},
  planes_rgba = {PLANAR_R, PLANAR_G, PLANAR_B, PLANAR_A};
  
  enum Condition {
    SRC_SAME_RES,
    SRC_NO_SUBSAMPLING,
    SRC_NOT_INTERLEAVED,
    DST_NOT_INTERLEAVED,
    DST_NOT_YUV_INTERLEAVED
  };
  
  static int pitchBitShift(int bitDepth);
  
  void fillSrcAndLutInfo(IScriptEnvironment* env);
  bool conditionNotFulfilled(Condition cond) const;
  void takeFirstPlaneFromEachSource();
  int generateSubsampledPixelType(IScriptEnvironment* env);
  void setDstFormatAndWrapperFunction(IScriptEnvironment* env);
  void fillDstInfo();
  void constructorTesting(IScriptEnvironment* env);
  
#include "SimpleLUT_ApplyLUT_WriteFunctions.tpp"
  
public:
  
  ApplyLUT(PClip _child, std::vector<PClip> _src_clips, PClip _lut_clip, int _mode, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints,int frame_range);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  
};

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, AVS_Linkage* vectors);
