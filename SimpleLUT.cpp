#include "SimpleLUT.hpp"

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

const AVS_Linkage *AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, AVS_Linkage* vectors)
{
  AVS_linkage = vectors;
  env->AddFunction("LUTClip", "[planes]s[dimensions]i[bit_depth]i[src_num]i", LUTClip::Create_LUTClip, 0);
  env->AddFunction("ApplyLUT", "c*[mode]i", ApplyLUT::Create, 0);
  return 0;
}
