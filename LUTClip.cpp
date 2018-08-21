#pragma warning (disable : 4100)

#include "windows.h"
#include "avisynth.h"
#include <cmath>
#include <vector>
#include <stdint.h>

#define pick_types(srcBitDepth, dstBitDepth, template_function) \
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

int pitchDivider(int bitDepth) {
  return bitDepth == 8 ? 1 : bitDepth == 32 ? 4 : 2;
}

class LUTClip : public IClip {

private:
    
  VideoInfo vi;
  PVideoFrame frame;
  
  int width, height;
  int num_planes, num_values;
  int planes[4];
  int clip_id;
  
  void write_8bit() {
    int value_repetitions = (int) pow(num_values, clip_id);
    for (int p = 0; p < num_planes; ++p) {
      uint8_t* ptr = (uint8_t*) frame->GetWritePtr(planes[p]);
      int x = 0;
      uint8_t value = 0;
      while (x < width) {
        for (int i = 0; i < value_repetitions; ++i)
          ptr[x++] = value;
        ++value;
      }
    }
  }
  
  void write_16bit() {
    int value_repetitions = (int) pow(num_values, clip_id);
    uint16_t mask = (uint16_t) (num_values - 1);
    for (int p = 0; p < num_planes; ++p) {
      uint16_t* ptr = (uint16_t*) frame->GetWritePtr(planes[p]);
      int x = 0;
      uint16_t value = 0;
      while (x < width) {
        for (int i = 0; i < value_repetitions; ++i)
          ptr[x++] = value;
        value = (value + 1) & mask;
      }
    }
  }
    
  void pixel_type_switch(int a, int b, int c, int d, int e, int bitDepth) {
    switch (bitDepth) { \
      case  8 : vi.pixel_type = a; \
                break; \
      case 10 : vi.pixel_type = b; \
                break; \
      case 12 : vi.pixel_type = c; \
                break; \
      case 14 : vi.pixel_type = d; \
                break; \
      case 16 : vi.pixel_type = e; \
                break; \
    }
  }
  
public:

  LUTClip(const char* plane_string, int bitDepth, int dimensions, const char* id, IScriptEnvironment* env) {
    
    if (dimensions == 2) {
      if(bitDepth > 12)
        env->ThrowError("LUTClip2D: Cannot prepare a 2D LUT for bit depths higher than 12");
      if(lstrcmpi(id, "x") != 0 && lstrcmpi(id, "y") != 0)
        env->ThrowError("LUTClip2D: \"id\" can only be \"x\" or \"y\".");
    } else if (dimensions == 3) {
      if (bitDepth > 8)
        env->ThrowError("LUTClip3D: Cannot prepare a 3D LUT for bit depths higher than 8");
      if(lstrcmpi(id, "x") != 0 && lstrcmpi(id, "y") != 0 && lstrcmpi(id, "z") != 0)
        env->ThrowError("LUTClip3D: \"id\" can only be \"x\", \"y\" or \"z\".");
    }
    
    if (bitDepth == 32)
      env->ThrowError("LUTClip: Only integer pixel types (with a bit depth of 8, 10, 12, 14 or 16) are supported.");
    if (bitDepth != 8 && bitDepth != 10 && bitDepth != 12 && bitDepth != 14 && bitDepth != 16)
      env->ThrowError("LUTClip: Invalid bit depth.");
    
    memset(&vi, 0, sizeof(VideoInfo));
    
    { // Set pixel type and plane identifiers array
      if (lstrcmpi(plane_string, "Y") == 0) {
        
        planes[0] = PLANAR_Y;
        num_planes = 1;
        pixel_type_switch(VideoInfo::CS_Y8, VideoInfo::CS_Y10, VideoInfo::CS_Y12,
                          VideoInfo::CS_Y14, VideoInfo::CS_Y16, bitDepth);
        
      } else if (lstrcmpi(plane_string, "YUV") == 0) {
        
        planes[0] = PLANAR_Y; planes[1] = PLANAR_U; planes[2] = PLANAR_V;
        num_planes = 3;
        pixel_type_switch(VideoInfo::CS_YV24, VideoInfo::CS_YUV444P10, VideoInfo::CS_YUV444P12,
                          VideoInfo::CS_YUV444P14, VideoInfo::CS_YUV444P16, bitDepth);
        
      } else if (lstrcmpi(plane_string, "YUVA") == 0) {
        
        planes[0] = PLANAR_Y; planes[1] = PLANAR_U; planes[2] = PLANAR_V; planes[3] = PLANAR_A;
        num_planes = 4;
        pixel_type_switch(VideoInfo::CS_YUVA444, VideoInfo::CS_YUVA444P10, VideoInfo::CS_YUVA444P12,
                          VideoInfo::CS_YUVA444P14, VideoInfo::CS_YUVA444P16, bitDepth);
        
      } else if (lstrcmpi(plane_string, "RGB") == 0) {
        
        planes[0] = PLANAR_R; planes[1] = PLANAR_G; planes[2] = PLANAR_B;
        num_planes = 3;
        pixel_type_switch(VideoInfo::CS_RGBP, VideoInfo::CS_RGBP10, VideoInfo::CS_RGBP12,
                          VideoInfo::CS_RGBP14, VideoInfo::CS_RGBP16, bitDepth);
        
      } else if (lstrcmpi(plane_string, "RGBA") == 0) {
        
        planes[0] = PLANAR_R; planes[1] = PLANAR_G; planes[2] = PLANAR_B; planes[3] = PLANAR_A;
        num_planes = 4;
        pixel_type_switch(VideoInfo::CS_RGBAP, VideoInfo::CS_RGBAP10, VideoInfo::CS_RGBAP12,
                          VideoInfo::CS_RGBAP14, VideoInfo::CS_RGBAP16, bitDepth);
      } else
        env->ThrowError("LUTClip: Invalid selection of planes.");
    }
      
    clip_id = id[0] - 'x';
    num_values = 1 << bitDepth;
    width = (int) pow(num_values, dimensions);
    height = 1;
    
    vi.width = width;
    vi.height = height;
    vi.fps_numerator = 24;
    vi.fps_denominator = 1;
    vi.num_frames = 1;
    
    frame = env->NewVideoFrame(vi);
    
    switch (bitDepth) {
      case  8 : write_8bit();
                break;
      default : write_16bit();
                break;
    }
    
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  bool __stdcall GetParity(int n) { return false; }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  int __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
  
  static AVSValue __cdecl Create_LUTClip(AVSValue args, void*, IScriptEnvironment* env) {
    return new LUTClip(args[0].AsString("Y"), args[1].AsInt(8), 1, "x", env);
  }
  
  static AVSValue __cdecl Create_LUTClip2D(AVSValue args, void*, IScriptEnvironment* env) {
    return new LUTClip(args[1].AsString("Y"), args[2].AsInt(8), 2, args[0].AsString("x"), env);
  }
  
  static AVSValue __cdecl Create_LUTClip3D(AVSValue args, void*, IScriptEnvironment* env) {
    return new LUTClip(args[1].AsString("Y"), args[2].AsInt(8), 3, args[0].AsString("x"), env);
  }
  
};

class ApplyLUT : public GenericVideoFilter {
  
private:
  
  PClip clip;
  PVideoFrame lut;
  
  int srcBitDepthID, dstBitDepthID;
  int src_pitch_divider, dst_pitch_divider;
  
  int num_src_planes, num_dst_planes;
  std::vector<int> src_planes, dst_planes, width, height;
  
  int src_num_values;
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_1plane_to_1plane_wrapper (std::vector<PVideoFrame> src, PVideoFrame dst) {
    for (int c = 0; c < num_src_clips; ++c) {
      for (int p = 0; p < num_src_planes; ++i) {
        
        int src_pitch = src[c]->GetPitch(src_planes[p])/src_pitch_divider;
        const src_pixel_t* srcp = (const src_pixel_t*) src[c]->GetReadPtr(src_planes[p]);
        const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
        int dst_pitch = dst->GetPitch(dst_planes[p])/dst_pitch_divider;
        dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p]);
        
        write_1plane_to_1plane <src_pixel_t, dst_pixel_t> \
        (src_pitch, srcp, dst_pitch, lutp, dstp, vi.width[p], vi.height[p]);
        
      }
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
  void write_1plane_to_3plane_wrapper (std::vector<PVideoFrame> src, PVideoFrame dst) {
    
    int src_pitch = src[0]->GetPitch(src_planes[0])/src_pitch_divider;
    const src_pixel_t* srcp = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[0]);
    
    if (vi.IsPlanar()) {
      
      const dst_pixel_t* lutp[3];
      int dst_pitch[3];
      dst_pixel_t* dstp[3];
      
      for (int p = 0; p < num_dst_planes; ++p) {
        lutp[p] = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
        dst_pitch[p] = dst->GetPitch(dst_planes[p])/dst_pitch_divider;
        dstp[p] = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p]);
      }
      
      write_1plane_to_3plane <src_pixel_t, dst_pixel_t> \
      (src_pitch, srcp, dst_pitch, lutp, dstp, width[0], height[0]);
      
    } else {
      
      int dst_pitch = dst->GetPitch()/dst_pitch_divider;
      const dst_pixel_t* lutp = lut->GetReadPtr();
      dst_pixel_t* dstp = dst->GetWritePtr();
      
      if (vi.IsRGB24() || vi.IsRGB48())
        write_1plane_to_3plane_packed_rgb <src_pixel_t, dst_pixel_t> \
        (src_pitch, srcp, dst_pitch, lutp, dstp, width[0], height[0]);
      else
        write_1plane_to_3plane_packed_rgba <src_pixel_t, dst_pixel_t> \
        (src_pitch, srcp, dst_pitch, lutp, dstp, width[0], height[0]);
    }
    
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
  void write_2plane_to_1plane_wrapper (std::vector<PVideoFrame> src, PVideoFrame dst) {
    for (int p = 0; p < num_dst_planes; ++i) {
      
      const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
      int dst_pitch = dst->GetPitch(dst_planes[p])/dst_pitch_divider;
      dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p]);
      
      int src_pitch[2];
      const src_pixel_t* srcp[2];
      
      for (int c = 0; c < num_src_clips; ++c) {
        src_pitch[c] = src[c]->GetPitch(src_planes[p])/src_pitch_divider;
        srcp[c] = (const src_pixel_t*) src[c]->GetReadPtr(src_planes[p]);
      }
      
      write_2plane_to_1plane <src_pixel_t, dst_pixel_t> \
      (src_pitch, srcp, dst_pitch, lutp, dstp, vi.width[p], vi.height[p]);
      
    }
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_2plane_to_1plane \
  (int src_pitch[2], const src_pixel_t* srcp[2],
   int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
   int width, int height, int srcBitDepth) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x];
        dstp[x] = lutp[b << srcBitDepth + a];
      }
      srcp[0] += src_pitch[0];
      srcp[1] += src_pitch[1];
      dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_2plane_to_3plane_wrapper (std::vector<PVideoFrame> src, PVideoFrame dst) {
    
    int src_pitch[2];
    const src_pixel_t* srcp[2];
    
    for (int c = 0; c < num_src_clips; ++c) {
      src_pitch[c] = src[c]->GetPitch(src_planes[0])/src_pitch_divider;
      srcp[c] = (const src_pixel_t*) src[c]->GetReadPtr(src_planes[0]);
    }
    
    const dst_pixel_t* lutp[3];
    int dst_pitch[3];
    dst_pixel_t* dstp[3];
    
    for (int p = 0; p < num_dst_planes; ++p) {
      lutp[p] = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
      dst_pitch[p] = dst->GetPitch(dst_planes[p])/dst_pitch_divider;
      dstp[p] = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p]);
    }
    
    write_2plane_to_3plane <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, width[0], height[0]);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_2plane_to_3plane \
  (int src_pitch[2], const src_pixel_t* srcp[2],
   int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
   int width, int height, int srcBitDepth) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x];
        int ab = b << srcBitDepth + a;
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
  void write_3plane_to_1plane_wrapper (std::vector<PVideoFrame> src, PVideoFrame dst) {
    for (int p = 0; p < num_dst_planes; ++p) {
      
      int src_pitch[3];
      const src_pixel_t* srcp[3];
      
      if (num_src_clips == 1) {
        for (int i = 0; i < 3; ++i) {
          src_pitch[i] = src[0]->GetPitch(src_planes[i])/src_pitch_divider;
          srcp[i] = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[i]);
        }
      } else {
        for (int c = 0; c < num_src_clips; ++c) {
          src_pitch[c] = src[c]->GetPitch(src_planes[p])/src_pitch_divider;
          srcp[c] = (const src_pixel_t*) src[c]->GetReadPtr(src_planes[p]);
        }
      }
      
      const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
      int dst_pitch = dst->GetPitch(dst_planes[p])/dst_pitch_divider;
      dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p])
      
      write_3plane_to_1plane <src_pixel_t, dst_pixel_t> \
      (src_pitch, srcp, dst_pitch, lutp, dstp, width[p], height[p]);
      
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
        dstp[x] = lutp[(c << 8 + b) << 8 + a];
      }
      srcp[0] += src_pitch[0];
      srcp[1] += src_pitch[1];
      srcp[2] += src_pitch[2];
      dstp += dst_pitch;
    }
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  void write_3plane_to_3plane_wrapper (std::vector<PVideoFrame> src, PVideoFrame dst) {
    
    int src_pitch[3];
    const src_pixel_t* srcp[3];
    
    if (num_src_clips == 1) {
      for (int p = 0; p < num_src_planes; ++p) {
        src_pitch[p] = src[0]->GetPitch(src_planes[p])/src_pitch_divider;
        srcp[p] = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[p]);
      }
    } else {
      for (int c = 0; c < num_src_clips; ++c) {
        src_pitch[c] = src[c]->GetPitch(src_planes[0])/src_pitch_divider;
        srcp[c] = (const src_pixel_t*) src[c]->GetReadPtr(src_planes[0]);
      }
    }
    
    const dst_pixel_t* lutp[3];
    int dst_pitch[3];
    dst_pixel_t* dstp[3];
    
    for (int p = 0; p < num_dst_planes; ++p) {
      lutp[p] = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[p]);
      dst_pitch[p] = dst->GetPitch(dst_planes[p])/dst_pitch_divider;
      dstp[p] = (dst_pixel_t*) dst->GetWritePtr(dst_planes[p]);
    }
    
    write_2plane_to_3plane <src_pixel_t, dst_pixel_t> \
    (src_pitch, srcp, dst_pitch, lutp, dstp, width[0], height[0]);
    
  }
  
  template <typename src_pixel_t, typename dst_pixel_t> \
  static void write_3plane_to_3plane \
  (int src_pitch[3], const src_pixel_t* srcp[3],
   int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
   int width, int height) {
    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int a = srcp[0][x], b = srcp[1][x], c = srcp[2][x];
        int abc = (c << 8 + b) << 8 + a;
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
  
  // Pointer to wrapper function
  void (*write_function_to_use) (std::vector<PVideoFrame> src, PVideoFrame dst);
  
public:
  
  ApplyLUT(PClip _child, PClip _clip, IScriptEnvironment* env) : GenericVideoFilter(_child), clip(_clip) {
  
    const VideoInfo vi_lut = clip->GetVideoInfo(),
                    vi_src = vi;
    
    int srcBitDepth = vi.BitsPerComponent();
    int dstBitDepth = vi_lut.BitsPerComponent();
    
    int src_num_values = 1 << srcBitDepth;
    
    double _dimensions = log(vi_lut.width)/log(src_num_values);
    int dimensions = (int) _dimensions;
    
    if (_dimensions != dimensions)
      env->ThrowError("ApplyLUT: The provided LUT clip is invalid or is not meant for the source clip's bit depth.");
    
    vi.pixel_type = vi_lut.pixel_type;
    
    srcBitDepthID = bitDepthID(srcBitDepth);
    dstBitDepthID = bitDepthID(dstBitDepth);
    src_pitch_divider = pitchDivider(srcBitDepth);
    dst_pitch_divider = pitchDivider(dstBitDepth);
    
    lut = clip->GetFrame(0, env);
    
    
    if (vi_src.IsY() && vi_lut.IsY()) {
      write_function_to_use = all_write_functions[srcBitDepthID][dstBitDepthID];
    } else if (srcBitDepth == 8 && !vi_src.IsRGB() && vi_lut.pixel_type == VideoInfo::CS_RGBPS) {
      src_planes[0] = PLANAR_Y;
      dst_planes[0] = PLANAR_R; dst_planes[1] = PLANAR_G; dst_planes[2] = PLANAR_B;
      write_function_to_use = &ApplyLUT::write_1plane8bit_to_3plane32bit;
    } else if (srcBitDepth == 8 && !vi_src.IsRGB() && vi_lut.pixel_type == VideoInfo::CS_BGR48) {
      src_planes[0] = PLANAR_Y;
      write_function_to_use = &ApplyLUT::write_1plane8bit_to_3component16bit_packed;
    } else
      env->ThrowError("ApplyLUT: Unimplemented colorspace combination.");
    
    /////////////////////////
    
    switch(dimensions) {
      case 1 :
        if (num_src_planes == num_dst_planes)
          write_function_to_use = pick_types(srcBitDepth, dstBitDepth, write_1plane_to_1plane_wrapper);
        else { //num_src_planes == 1
          
        }
      
    }
    
  }
  
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
    
    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);
    
    (this->*write_function_to_use)(src, dst);
    
    return dst;
    
  }
  
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    return new ApplyLUT(args[0].AsClip(),args[1].AsClip(),env);
  }
  
};

const AVS_Linkage *AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, AVS_Linkage* vectors)
{
  AVS_linkage = vectors;
  env->AddFunction("LUTClip", "[planes]s[bitDepth]i", LUTClip::Create_LUTClip, 0);
  env->AddFunction("LUTClip2D", "[id]s[planes]s[bitDepth]i", LUTClip::Create_LUTClip2D, 0);
  env->AddFunction("LUTClip3D", "[id]s[planes]s[bitDepth]i", LUTClip::Create_LUTClip3D, 0);
  env->AddFunction("ApplyLUT", "c+", ApplyLUT::Create, 0);
  return 0;
}
