#include "SimpleLUT.hpp"

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

const std::vector<int> getPlanesVector(const VideoInfo& vi, const char* description, IScriptEnvironment* env) {
  if (!vi.IsPlanar()) return no_planes;
  if (vi.IsY()) return planes_y;
  if (vi.IsYUV()) return planes_yuv;
  if (vi.IsYUVA()) return planes_yuva;
  if (vi.IsPlanarRGB()) return planes_rgb;
  if (vi.IsPlanarRGBA()) return planes_rgba;
  env->ThrowError("ApplyLUT: Unsupported colorspace of %s.", description);
  return no_planes;
}

const AVS_Linkage *AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, AVS_Linkage* vectors)
{
  AVS_linkage = vectors;
  env->AddFunction("LUTClip", "[planes]s[dimensions]i[bit_depth]i[src_num]i", LUTClip::Create_LUTClip, 0);
  env->AddFunction("ApplyLUT", "c*[mode]i", ApplyLUT::Create, 0);
  return 0;
}
