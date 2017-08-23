//
// Created by 郭映中 on 2017/8/23.
//

#pragma once

extern "C" {

void *ff_malloc(unsigned long size) noexcept;
void ff_free(void *ptr) noexcept;
void *ff_calloc(unsigned long count, unsigned long size) noexcept;
void *ff_realloc(void *ptr, unsigned long size) noexcept;

}
