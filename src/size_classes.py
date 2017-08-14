#!/usr/bin/env python3

import math

kHeaderFilename = '../include/size_classes.h'
kSourceFilename = 'size_classes.cc'
kInitClasses = [8,]
kMinClass = 16
kMaxClass = 2 * 1024 * 1024 # 2M
kMinSpace = 16
kMultiple = 5
kPage = 4 * 1024
kMaxSmallClass = 14 * 1024 # 14K
kMinLargeClass = 16 * 1024 # 14K

kMaxLookupClass = 4 * 1024

kUnsignedLong = 'unsigned long'
kUnsignedInt = 'unsigned int'
kUnsignedShort = 'unsigned short'

def lcm(a, b):
    return a * b // math.gcd(a, b)


def dump_code(f, cpp_type, name, size, array):
    f.write('%s %s[%s] = {\n' % (cpp_type, name, size))
    [f.write(str(s) + ', ') for s in array]
    f.write('\n};\n\n')


def gen_sz_to_ind_lookup(classes):
    size_lookup = []
    index_lookup = []
    curr_size = 8
    curr_index = 0
    while curr_size <= kMaxLookupClass:
        if curr_size > classes[curr_index]:
            curr_index += 1
        index_lookup.append(curr_index)
        # print(curr_size, classes[curr_index])

        curr_size += 8
    return index_lookup


def generate_classes():

    curr_size = kMinClass
    curr_space = kMinSpace

    classes = kInitClasses
    while curr_size <= kMaxClass:
        classes.append(curr_size)
        if curr_size + curr_space*2 >= kMultiple * curr_space*2:
            curr_space *= 2
        curr_size += curr_space

    small_classes = [ s for s in classes if s <= kMaxSmallClass ]
    ge_page_classes = [ s for s in classes if s >= kPage ]
    slab_sizes = [lcm(s, kPage) for s in small_classes]
    slab_num = [ slab_sizes[i] // small_classes[i] 
            for i in range(len(small_classes))]

    index_lookup= gen_sz_to_ind_lookup(classes)

    print(classes)
    print(small_classes)
    print(slab_sizes)
    print(slab_num)

    kNumSizeClasses = 'kNumSizeClasses'
    kNumSmallClasses = 'kNumSmallClasses'
    kNumGePageClasses = 'kNumGePageClasses'

    with open(kHeaderFilename, 'w') as header:
        header.write('#pragma once\n\n')
        header.write(
            'constexpr int %s = ' % (kNumSizeClasses) 
            + str(len(classes)) + ';\n');
        header.write(
            'constexpr int %s = ' % (kNumGePageClasses) 
            + str(len(ge_page_classes)) + ';\n');
        header.write(
            'constexpr int %s = ' % (kNumSmallClasses) 
            + str(len(small_classes)) + ';\n');
        header.write(
            'constexpr int kMaxSlabRegions = '
            + str(slab_num[0]) + ';\n');
        header.write(
            'constexpr int kMaxSmallClass = '
            + str(kMaxSmallClass) + ';\n');
        header.write(
            'constexpr int kMinLargeClass = '
            + str(kMinLargeClass) + ';\n');
        header.write('extern %s %s[%s];\n' % (
            kUnsignedInt, 'g_size_classes', kNumSizeClasses))
        header.write('extern %s %s[%s];\n' % (
            kUnsignedInt, 'g_ge_page_classes', kNumGePageClasses))
        header.write('extern %s %s[%s];\n' % (
            kUnsignedShort, 'g_small_classes', kNumSmallClasses))
        header.write('extern %s %s[%s];\n' % (
            kUnsignedShort, 'g_slab_sizes', kNumSmallClasses))
        header.write('extern %s %s[%s];\n' % (
            kUnsignedShort, 'g_slab_num', kNumSmallClasses))
        header.write('extern %s %s[%s];\n' % (
            kUnsignedShort, 'g_index_lookup', len(index_lookup)))

    with open(kSourceFilename, 'w') as source:
        source.write('#include "size_classes.h"\n\n')
        dump_code(source, kUnsignedInt, 
            'g_size_classes', kNumSizeClasses, classes)
        dump_code(source, kUnsignedInt, 
            'ge_page_classes', kNumGePageClasses, ge_page_classes)
        dump_code(source, kUnsignedShort, 
            'g_small_classes', kNumSmallClasses, small_classes)
        dump_code(source, kUnsignedShort, 
            'g_slab_sizes', kNumSmallClasses, slab_sizes)
        dump_code(source, kUnsignedShort, 
            'g_slab_num', kNumSmallClasses, slab_num)
        dump_code(source, kUnsignedShort, 
            'g_index_lookup', len(index_lookup), index_lookup)

if __name__ == '__main__':
    generate_classes()
