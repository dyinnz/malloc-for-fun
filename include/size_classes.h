#pragma once

constexpr int kNumSizeClasses = 65;
constexpr int kNumGePageClasses = 37;
constexpr int kNumSmallClasses = 36;
constexpr int kMaxSlabRegions = 512;
constexpr int kMaxSmallClass = 14336;
constexpr int kMinLargeClass = 16384;
extern unsigned int g_size_classes[kNumSizeClasses];
extern unsigned int g_ge_page_classes[kNumGePageClasses];
extern unsigned short g_small_classes[kNumSmallClasses];
extern unsigned short g_slab_sizes[kNumSmallClasses];
extern unsigned short g_slab_num[kNumSmallClasses];
extern unsigned short g_index_lookup[512];
