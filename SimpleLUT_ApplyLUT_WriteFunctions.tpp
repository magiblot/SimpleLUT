template <typename src_pixel_t, typename dst_pixel_t>
void write_1plane_to_1plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  for (int dp = 0; dp < num_dst_planes; ++dp) {
    int sp = min(dp, num_src_planes - 1),
        sc = min(dp, num_src_clips - 1);
      
    int src_pitch = src[sc]->GetPitch(src_planes[sc][sp]) >> src_pitch_bitshift;
    const src_pixel_t* srcp = (const src_pixel_t*) src[sc]->GetReadPtr(src_planes[sc][sp]);
    const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
    int dst_pitch = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
    dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[dp]);
    
    write_1plane_to_1plane <src_pixel_t, dst_pixel_t>
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[dp], dst_height[dp]);
  }
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_1plane_to_1plane
(int src_pitch, const src_pixel_t* srcp,
  int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
  int width, int height) {
  
  for (int y = 0; y < height; ++y) {
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp, 64);
    __assume_aligned(lutp, 64);
    __assume_aligned(dstp, 64);
#endif
    for (int x = 0; x < width; ++x)
      dstp[x] = lutp[srcp[x]];
    srcp += src_pitch;
    dstp += dst_pitch;
  }
  
}

template <typename src_pixel_t, typename dst_pixel_t>
void write_1plane_to_3plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  
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
  
  write_1plane_to_3plane <src_pixel_t, dst_pixel_t>
  (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
  
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_1plane_to_3plane
(int src_pitch, const src_pixel_t* srcp,
  int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
  int width, int height) {
  
  for (int y = 0; y < height; ++y) {
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp, 64);
    __assume_aligned(lutp[0], 64);
    __assume_aligned(lutp[1], 64);
    __assume_aligned(lutp[2], 64);
    __assume_aligned(dstp[0], 64);
    __assume_aligned(dstp[1], 64);
    __assume_aligned(dstp[2], 64);
#endif
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

template <typename src_pixel_t, typename dst_pixel_t>
void write_1plane_to_3plane_packed_rgb_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  
  int src_pitch = src[0]->GetPitch(src_planes[0][0]) >> src_pitch_bitshift;
  const src_pixel_t* srcp = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[0][0]);
  
  int dst_pitch = dst->GetPitch() >> dst_pitch_bitshift;
  const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr();
  dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr();
  
  write_1plane_to_3plane_packed_rgb <src_pixel_t, dst_pixel_t>
  (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
  
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_1plane_to_3plane_packed_rgb
(int src_pitch, const src_pixel_t* srcp,
  int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
  int width, int height) {
  
  srcp += height * src_pitch;
  for (int y = 0; y < height; ++y) {
    srcp -= src_pitch;
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp, 64);
    __assume_aligned(lutp, 64);
    __assume_aligned(dstp, 64);
#endif
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

template <typename src_pixel_t, typename dst_pixel_t>
void write_1plane_to_3plane_packed_rgba_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  
  int src_pitch = src[0]->GetPitch(src_planes[0][0]) >> src_pitch_bitshift;
  const src_pixel_t* srcp = (const src_pixel_t*) src[0]->GetReadPtr(src_planes[0][0]);
  
  int dst_pitch = dst->GetPitch() >> dst_pitch_bitshift;
  const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr();
  dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr();
  
  write_1plane_to_3plane_packed_rgba <src_pixel_t, dst_pixel_t>
  (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
  
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_1plane_to_3plane_packed_rgba
(int src_pitch, const src_pixel_t* srcp,
  int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
  int width, int height) {
  
  srcp += height * src_pitch;
  for (int y = 0; y < height; ++y) {
    srcp -= src_pitch;
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp, 64);
    __assume_aligned(lutp, 64);
    __assume_aligned(dstp, 64);
#endif
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

template <typename src_pixel_t, typename dst_pixel_t>
void write_2plane_to_1plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  for (int dp = 0; dp < num_dst_planes; ++dp) {
    
    const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
    int dst_pitch = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
    dst_pixel_t* dstp = (dst_pixel_t*) dst->GetWritePtr(dst_planes[dp]);
    
    int src_pitch[2];
    const src_pixel_t* srcp[2];
    
    for (int i = 0; i < 2; ++i) {
      int sc = min(i, num_src_clips - 1),
          sp = min(dp, num_src_planes - 1);
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
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp[0], 64);
    __assume_aligned(srcp[1], 64);
    __assume_aligned(lutp, 64);
    __assume_aligned(dstp, 64);
#endif
    for (int x = 0; x < width; ++x) {
      int a = srcp[0][x], b = srcp[1][x];
      dstp[x] = lutp[(b << srcBitDepth) + a];
    }
    srcp[0] += src_pitch[0];
    srcp[1] += src_pitch[1];
    dstp += dst_pitch;
  }
  
}

template <typename src_pixel_t, typename dst_pixel_t>
void write_2plane_to_3plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  
  int src_pitch[2];
  const src_pixel_t* srcp[2];
  
  for (int i = 0; i < 2; ++i) {
    int sc = min(i, num_src_clips - 1);
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
  
  write_2plane_to_3plane <src_pixel_t, dst_pixel_t>
  (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0], srcBitDepth);
  
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_2plane_to_3plane
(int src_pitch[2], const src_pixel_t* srcp[2],
  int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
  int width, int height, int srcBitDepth) {
  
  for (int y = 0; y < height; ++y) {
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp[0], 64);
    __assume_aligned(srcp[1], 64);
    __assume_aligned(lutp[0], 64);
    __assume_aligned(lutp[1], 64);
    __assume_aligned(lutp[2], 64);
    __assume_aligned(dstp[0], 64);
    __assume_aligned(dstp[1], 64);
    __assume_aligned(dstp[2], 64);
#endif
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

template <typename src_pixel_t, typename dst_pixel_t>
void write_3plane_to_1plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  for (int dp = 0; dp < num_dst_planes; ++dp) {
    
    int src_pitch[3];
    const src_pixel_t* srcp[3];
    
    for (int i = 0; i < 3; ++i) {
      int sc = min(i, num_src_clips - 1),
          sp = min(num_src_clips == 1 ? i : dp, num_src_planes - 1);
      src_pitch[i] = src[sc]->GetPitch(src_planes[sc][sp]) >> src_pitch_bitshift;
      srcp[i] = (const src_pixel_t*) src[sc]->GetReadPtr(src_planes[sc][sp]);
    }
    
    const dst_pixel_t* lutp = (const dst_pixel_t*) lut->GetReadPtr(dst_planes[dp]);
    int dst_pitch = dst->GetPitch(dst_planes[dp]) >> dst_pitch_bitshift;
    dst_pixel_t* dstp = (dst_pixel_t*)dst->GetWritePtr(dst_planes[dp]);
    
    write_3plane_to_1plane <src_pixel_t, dst_pixel_t>
    (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[dp], dst_height[dp]);
    
  }
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_3plane_to_1plane
(int src_pitch[3], const src_pixel_t* srcp[3],
  int dst_pitch, const dst_pixel_t* lutp, dst_pixel_t* dstp,
  int width, int height) {
  
  for (int y = 0; y < height; ++y) {
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp[0], 64);
    __assume_aligned(srcp[1], 64);
    __assume_aligned(srcp[2], 64);
    __assume_aligned(lutp, 64);
    __assume_aligned(dstp, 64);
#endif
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

template <typename src_pixel_t, typename dst_pixel_t>
void write_3plane_to_3plane_wrapper (std::vector<PVideoFrame>& src, PVideoFrame& dst) const {
  
  int src_pitch[3];
  const src_pixel_t* srcp[3];
  
  for (int i = 0; i < 3; ++i) {
    int sc = min(i, num_src_clips - 1),
        sp = min(num_src_clips == 1 ? i : 0, num_src_planes - 1);
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
  
  write_3plane_to_3plane <src_pixel_t, dst_pixel_t>
  (src_pitch, srcp, dst_pitch, lutp, dstp, dst_width[0], dst_height[0]);
  
}

template <typename src_pixel_t, typename dst_pixel_t>
static void write_3plane_to_3plane
(int src_pitch[3], const src_pixel_t* srcp[3],
  int dst_pitch[3], const dst_pixel_t* lutp[3], dst_pixel_t* dstp[3],
  int width, int height) {
  
  for (int y = 0; y < height; ++y) {
#ifdef __INTEL_COMPILER
    #pragma ivdep
    __assume_aligned(srcp[0], 64);
    __assume_aligned(srcp[1], 64);
    __assume_aligned(srcp[2], 64);
    __assume_aligned(lutp[0], 64);
    __assume_aligned(lutp[1], 64);
    __assume_aligned(lutp[2], 64);
    __assume_aligned(dstp[0], 64);
    __assume_aligned(dstp[1], 64);
    __assume_aligned(dstp[2], 64);
#endif
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
