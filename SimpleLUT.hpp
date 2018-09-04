#pragma warning (disable : 4100)

#include "windows.h"
#include "avisynth.h"
#include <vector>
#include <cmath>
#include <stdint.h>

#define PICK_TEMPLATE(srcBitDepth, dstBitDepth, template_function) \
 (srcBitDepth == 8 ? \
    dstBitDepth == 8 ?  template_function <uint8_t,uint8_t> : \
    dstBitDepth == 32 ? template_function <uint8_t,uint32_t> : \
                        template_function <uint8_t,uint16_t> \
  : \
    dstBitDepth == 8 ?  template_function <uint16_t,uint8_t> : \
    dstBitDepth == 32 ? template_function <uint16_t,uint32_t> : \
                        template_function <uint16_t,uint16_t>)

#define no_planes std::vector<int>({0})
#define planes_y std::vector<int>({PLANAR_Y})
#define planes_yuv std::vector<int>({PLANAR_Y, PLANAR_U, PLANAR_V})
#define planes_yuva std::vector<int>({PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A})
#define planes_rgb std::vector<int>({PLANAR_R, PLANAR_G, PLANAR_B})
#define planes_rgba std::vector<int>({PLANAR_R, PLANAR_G, PLANAR_B, PLANAR_A})
  
int getPixelTypeAccordingToBitDepth(int generic_flag, int bitDepth);
const std::vector<int> getPlanesVector(const VideoInfo& vi, const char* description, IScriptEnvironment* env);

class LUTClip : public IClip {

private:
    
  VideoInfo vi;
  PVideoFrame frame;
  
  int width, height;
  int num_planes, num_values;
  std::vector<int> planes;
  int src_num;
  
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
  bool optMakeWritable;
  
  int num_src_clips, num_src_planes;
  std::vector<VideoInfo> vi_src;
  std::vector<std::vector<int>> src_planes, src_width, src_height;
  int srcBitDepth, dstBitDepth, src_pitch_bitshift, dst_pitch_bitshift;
  VideoInfo vi_lut;
  int num_lut_planes, lut_dimensions;
  PVideoFrame lut;
  
  int num_dst_planes;
  std::vector<int> dst_planes, dst_width, dst_height;
  
  int num_writable_candidates;
  std::vector<int> writable_candidates;
  
  // Pointer to wrapper function
  void(ApplyLUT::*wrapper_to_use) (std::vector<PVideoFrame>& src, PVideoFrame& dst) const;
  
  const int MAX_NUM_PLANES = 3;
  
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
  void findWritableCandidates();
  
#ifdef ENABLE_CONSTRUCTOR_TESTING
  void constructorTesting(IScriptEnvironment* env);
#endif
  
#include "SimpleLUT_ApplyLUT_WriteFunctions.tpp"
  
public:
  
  ApplyLUT(PClip _child, std::vector<PClip> _src_clips, PClip _lut_clip, int _mode, bool _optMakeWritable, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints,int frame_range);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  
};

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, AVS_Linkage* vectors);
