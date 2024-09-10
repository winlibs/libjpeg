/*
 * Copyright (C)2021-2023 D. R. Commander.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the libjpeg-turbo Project nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS",
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <turbojpeg.h>
#include <stdlib.h>
#include <stdint.h>


#define NUMPF  4


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  tjhandle handle = NULL;
  void *dstBuf = NULL;
  int width = 0, height = 0, precision, sampleSize, pfi;
  /* TJPF_RGB-TJPF_BGR share the same code paths, as do TJPF_RGBX-TJPF_XRGB and
     TJPF_RGBA-TJPF_ARGB.  Thus, the pixel formats below should be the minimum
     necessary to achieve full coverage. */
  enum TJPF pixelFormats[NUMPF] =
    { TJPF_RGB, TJPF_BGRX, TJPF_GRAY, TJPF_CMYK };
#if defined(__has_feature) && __has_feature(memory_sanitizer)
  char env[18] = "JSIMD_FORCENONE=1";

  /* The libjpeg-turbo SIMD extensions produce false positives with
     MemorySanitizer. */
  putenv(env);
#endif

  if ((handle = tj3Init(TJINIT_DECOMPRESS)) == NULL)
    goto bailout;

  /* We ignore the return value of tj3DecompressHeader(), because malformed
     JPEG images that might expose issues in libjpeg-turbo might also have
     header errors that cause tj3DecompressHeader() to fail. */
  tj3DecompressHeader(handle, data, size);
  width = tj3Get(handle, TJPARAM_JPEGWIDTH);
  height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
  precision = tj3Get(handle, TJPARAM_PRECISION);
  sampleSize = (precision > 8 ? 2 : 1);

  /* Ignore 0-pixel images and images larger than 1 Megapixel, as Google's
     OSS-Fuzz target for libjpeg-turbo did.  Casting width to (uint64_t)
     prevents integer overflow if width * height > INT_MAX. */
  if (width < 1 || height < 1 || (uint64_t)width * height > 1048576)
    goto bailout;

  tj3Set(handle, TJPARAM_SCANLIMIT, 500);

  for (pfi = 0; pfi < NUMPF; pfi++) {
    int w = width, h = height;
    int pf = pixelFormats[pfi], i;
    int64_t sum = 0;

    /* Test non-default decompression options on the first iteration. */
    tj3Set(handle, TJPARAM_BOTTOMUP, pfi == 0);
    tj3Set(handle, TJPARAM_FASTUPSAMPLE, pfi == 0);

    if (!tj3Get(handle, TJPARAM_LOSSLESS)) {
      tj3Set(handle, TJPARAM_FASTDCT, pfi == 0);

      /* Test IDCT scaling on the second iteration. */
      if (pfi == 1) {
        tjscalingfactor sf = { 1, 2 };
        tj3SetScalingFactor(handle, sf);
        w = TJSCALED(width, sf);
        h = TJSCALED(height, sf);
      } else
        tj3SetScalingFactor(handle, TJUNSCALED);

      /* Test partial image decompression on the fourth iteration, if the image
         is large enough. */
      if (pfi == 3 && w >= 97 && h >= 75) {
        tjregion cr = { 32, 16, 65, 59 };
        tj3SetCroppingRegion(handle, cr);
      } else
        tj3SetCroppingRegion(handle, TJUNCROPPED);
    }

    if ((dstBuf = malloc(w * h * tjPixelSize[pf] * sampleSize)) == NULL)
      goto bailout;

    if (precision == 8) {
      if (tj3Decompress8(handle, data, size, (unsigned char *)dstBuf, 0,
                         pf) == 0) {
        /* Touch all of the output pixels in order to catch uninitialized reads
           when using MemorySanitizer. */
        for (i = 0; i < w * h * tjPixelSize[pf]; i++)
          sum += ((unsigned char *)dstBuf)[i];
      } else
        goto bailout;
    } else if (precision == 12) {
      if (tj3Decompress12(handle, data, size, (short *)dstBuf, 0, pf) == 0) {
        /* Touch all of the output pixels in order to catch uninitialized reads
           when using MemorySanitizer. */
        for (i = 0; i < w * h * tjPixelSize[pf]; i++)
          sum += ((short *)dstBuf)[i];
      } else
        goto bailout;
    } else {
      if (tj3Decompress16(handle, data, size, (unsigned short *)dstBuf, 0,
                          pf) == 0) {
        /* Touch all of the output pixels in order to catch uninitialized reads
           when using MemorySanitizer. */
        for (i = 0; i < w * h * tjPixelSize[pf]; i++)
          sum += ((unsigned short *)dstBuf)[i];
      } else
        goto bailout;
    }

    free(dstBuf);
    dstBuf = NULL;

    /* Prevent the code above from being optimized out.  This test should never
       be true, but the compiler doesn't know that. */
    if (sum > ((1LL << precision) - 1LL) * 1048576LL * tjPixelSize[pf])
      goto bailout;
  }

bailout:
  free(dstBuf);
  tj3Destroy(handle);
  return 0;
}
