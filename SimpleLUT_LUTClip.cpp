#include "SimpleLUT.hpp"

#include <string.h>

#ifndef _MSC_VER

#include <strings.h>

static int stricmp(const char *a, const char *b) noexcept
{
    return strcasecmp(a, b);
}

#endif // _MSC_VER
  
template<typename pixel_t> void LUTClip::write_planar_lut() const {
  pixel_t mask = (pixel_t) (num_values - 1);
  for (int p = 0; p < num_planes; ++p) {
    pixel_t* ptr = (pixel_t*) frame->GetWritePtr(planes[p]);
    int value_repetitions = (int) pow(num_values, src_num == -1 ? p : src_num);
    int x = 0;
    pixel_t value = 0;
    while (x < width) {
      for (int i = 0; i < value_repetitions; ++i)
        ptr[x++] = value;
      value = (value + 1)&mask;
    }
  }
}

LUTClip::LUTClip(const char* plane_string, int dimensions, int bitDepth, int _src_num, IScriptEnvironment* env) {
  
  memset(&vi, 0, sizeof(VideoInfo));
  
  if (stricmp(plane_string, "Y") == 0)
    vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_Y, bitDepth);
  else if (stricmp(plane_string, "YUV") == 0)
    vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_YUV444, bitDepth);
  else if (stricmp(plane_string, "YUVA") == 0)
    vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_YUVA444, bitDepth);
  else if (stricmp(plane_string, "RGB") == 0)
    vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_RGBP, bitDepth);
  else if (stricmp(plane_string, "RGBA") == 0)
    vi.pixel_type = getPixelTypeAccordingToBitDepth(VideoInfo::CS_GENERIC_RGBAP, bitDepth);
  else
    env->ThrowError("LUTClip: Invalid selection of planes.");
  
  planes = getPlanesVector(vi, 0, 0);
  num_planes = (int) planes.size();
  src_num = dimensions == 1 ? 0 : _src_num - 1;
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

PVideoFrame __stdcall LUTClip::GetFrame(int n, IScriptEnvironment* env) { return frame; }
bool __stdcall LUTClip::GetParity(int n) { return false; }
const VideoInfo& __stdcall LUTClip::GetVideoInfo() { return vi; }
int __stdcall LUTClip::SetCacheHints(int cachehints,int frame_range) {
  return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}
void __stdcall LUTClip::GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env) {}

AVSValue __cdecl LUTClip::Create_LUTClip(AVSValue args, void*, IScriptEnvironment* env) {
  
  if (!args[0].Defined())
    env->ThrowError("LUTClip: Parameter \"planes\" was not given a value.");
  
  int dimensions = args[1].AsInt(1);
  if (dimensions < 1 || 3 < dimensions)
    env->ThrowError("LUTClip: \"dimensions\" must be either 1, 2 or 3.");
  
  int bitDepth = args[2].AsInt(8);
  if (bitDepth != 8 && bitDepth != 10 && bitDepth != 12 && bitDepth != 14 && bitDepth != 16 && bitDepth != 32)
    env->ThrowError("LUTClip: Invalid bit depth.");
  if (bitDepth == 32)
    env->ThrowError("LUTClip: Only integer pixel types (with a bit depth of 8, 10, 12, 14 or 16)\nare supported.");
  
  int src_num = args[3].AsInt(1);
  if (dimensions == 2) {
    if(bitDepth > 12)
      env->ThrowError("LUTClip: Cannot prepare a 2D LUT for bit depths higher than 12");
    if(src_num < 1 || 2 < src_num)
      env->ThrowError("LUTClip: (2D) \"src_num\" must be either 1 or 2.");
  } else if (dimensions == 3) {
    if (bitDepth > 8)
      env->ThrowError("LUTClip: Cannot prepare a 3D LUT for bit depths higher than 8");
    if(src_num < 0 || 3 < src_num)
      env->ThrowError("LUTClip: (3D) \"src_num\" must be either 0, 1, 2 or 3.");
  }
  
  return new LUTClip(args[0].AsString(), dimensions, bitDepth, src_num, env);
}
