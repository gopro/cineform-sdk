/*! @file sse2_runtime_shift.h

*  @brief Runtime shift helper functions for SSE2 intrinsics on ARM/NEON
*
*  @version 1.0.0
*
*  (C) Copyright 2017 GoPro Inc (http://gopro.com/).
*
*  Licensed under either:
*  - Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0
*  - MIT license, http://opensource.org/licenses/MIT
*  at your option.
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/

#ifndef _SSE2_RUNTIME_SHIFT_H
#define _SSE2_RUNTIME_SHIFT_H

#include <simde/x86/sse2.h>

/* Runtime shift helper functions for ARM/NEON
 * These are needed because NEON intrinsics require compile-time constant
 * shift amounts, unlike SSE2 on x86 which supports runtime shifts.
 */

/* Right shift by runtime amount for 16-bit integers */
static inline __m128i _mm_srli_epi16_runtime(__m128i a, int count)
{
    switch(count & 0xF) {
        case 0: return a;
        case 1: return _mm_srli_epi16(a, 1);
        case 2: return _mm_srli_epi16(a, 2);
        case 3: return _mm_srli_epi16(a, 3);
        case 4: return _mm_srli_epi16(a, 4);
        case 5: return _mm_srli_epi16(a, 5);
        case 6: return _mm_srli_epi16(a, 6);
        case 7: return _mm_srli_epi16(a, 7);
        case 8: return _mm_srli_epi16(a, 8);
        case 9: return _mm_srli_epi16(a, 9);
        case 10: return _mm_srli_epi16(a, 10);
        case 11: return _mm_srli_epi16(a, 11);
        case 12: return _mm_srli_epi16(a, 12);
        case 13: return _mm_srli_epi16(a, 13);
        case 14: return _mm_srli_epi16(a, 14);
        case 15: return _mm_srli_epi16(a, 15);
        default: return _mm_setzero_si128();
    }
}

/* Left shift by runtime amount for 16-bit signed integers */
static inline __m128i _mm_slli_epi16_runtime(__m128i a, int count)
{
    if((count) > 15) return _mm_setzero_si128();
    switch(count & 0xF) {
        case 0: return a;
        case 1: return _mm_slli_epi16(a, 1);
        case 2: return _mm_slli_epi16(a, 2);
        case 3: return _mm_slli_epi16(a, 3);
        case 4: return _mm_slli_epi16(a, 4);
        case 5: return _mm_slli_epi16(a, 5);
        case 6: return _mm_slli_epi16(a, 6);
        case 7: return _mm_slli_epi16(a, 7);
        case 8: return _mm_slli_epi16(a, 8);
        case 9: return _mm_slli_epi16(a, 9);
        case 10: return _mm_slli_epi16(a, 10);
        case 11: return _mm_slli_epi16(a, 11);
        case 12: return _mm_slli_epi16(a, 12);
        case 13: return _mm_slli_epi16(a, 13);
        case 14: return _mm_slli_epi16(a, 14);
        case 15: return _mm_slli_epi16(a, 15);
        default: return _mm_setzero_si128();
    }
}

/* Right shift by runtime amount for 32-bit integers */
static inline __m128i _mm_srli_epi32_runtime(__m128i a, int count)
{
    if((count) > 31) return _mm_setzero_si128();
    switch(count & 0x1F) {
        case 0: return a;
        case 1: return _mm_srli_epi32(a, 1);
        case 2: return _mm_srli_epi32(a, 2);
        case 3: return _mm_srli_epi32(a, 3);
        case 4: return _mm_srli_epi32(a, 4);
        case 5: return _mm_srli_epi32(a, 5);
        case 6: return _mm_srli_epi32(a, 6);
        case 7: return _mm_srli_epi32(a, 7);
        case 8: return _mm_srli_epi32(a, 8);
        case 9: return _mm_srli_epi32(a, 9);
        case 10: return _mm_srli_epi32(a, 10);
        case 11: return _mm_srli_epi32(a, 11);
        case 12: return _mm_srli_epi32(a, 12);
        case 13: return _mm_srli_epi32(a, 13);
        case 14: return _mm_srli_epi32(a, 14);
        case 15: return _mm_srli_epi32(a, 15);
        case 16: return _mm_srli_epi32(a, 16);
        case 17: return _mm_srli_epi32(a, 17);
        case 18: return _mm_srli_epi32(a, 18);
        case 19: return _mm_srli_epi32(a, 19);
        case 20: return _mm_srli_epi32(a, 20);
        case 21: return _mm_srli_epi32(a, 21);
        case 22: return _mm_srli_epi32(a, 22);
        case 23: return _mm_srli_epi32(a, 23);
        case 24: return _mm_srli_epi32(a, 24);
        case 25: return _mm_srli_epi32(a, 25);
        case 26: return _mm_srli_epi32(a, 26);
        case 27: return _mm_srli_epi32(a, 27);
        case 28: return _mm_srli_epi32(a, 28);
        case 29: return _mm_srli_epi32(a, 29);
        case 30: return _mm_srli_epi32(a, 30);
        case 31: return _mm_srli_epi32(a, 31);
        default: return _mm_setzero_si128();
    }
}

/* Left shift by runtime amount for 32-bit integers */
static inline __m128i _mm_slli_epi32_runtime(__m128i a, int count)
{
    if((count) > 31) return _mm_setzero_si128();
    switch(count & 0x1F) {
        case 0: return a;
        case 1: return _mm_slli_epi32(a, 1);
        case 2: return _mm_slli_epi32(a, 2);
        case 3: return _mm_slli_epi32(a, 3);
        case 4: return _mm_slli_epi32(a, 4);
        case 5: return _mm_slli_epi32(a, 5);
        case 6: return _mm_slli_epi32(a, 6);
        case 7: return _mm_slli_epi32(a, 7);
        case 8: return _mm_slli_epi32(a, 8);
        case 9: return _mm_slli_epi32(a, 9);
        case 10: return _mm_slli_epi32(a, 10);
        case 11: return _mm_slli_epi32(a, 11);
        case 12: return _mm_slli_epi32(a, 12);
        case 13: return _mm_slli_epi32(a, 13);
        case 14: return _mm_slli_epi32(a, 14);
        case 15: return _mm_slli_epi32(a, 15);
        case 16: return _mm_slli_epi32(a, 16);
        case 17: return _mm_slli_epi32(a, 17);
        case 18: return _mm_slli_epi32(a, 18);
        case 19: return _mm_slli_epi32(a, 19);
        case 20: return _mm_slli_epi32(a, 20);
        case 21: return _mm_slli_epi32(a, 21);
        case 22: return _mm_slli_epi32(a, 22);
        case 23: return _mm_slli_epi32(a, 23);
        case 24: return _mm_slli_epi32(a, 24);
        case 25: return _mm_slli_epi32(a, 25);
        case 26: return _mm_slli_epi32(a, 26);
        case 27: return _mm_slli_epi32(a, 27);
        case 28: return _mm_slli_epi32(a, 28);
        case 29: return _mm_slli_epi32(a, 29);
        case 30: return _mm_slli_epi32(a, 30);
        case 31: return _mm_slli_epi32(a, 31);
        default: return _mm_setzero_si128();
    }
}

/* Macros to replace runtime shift intrinsics with the helper functions */
#define _mm_srli_epi16(a, count) _mm_srli_epi16_runtime(a, count)
#define _mm_slli_epi16(a, count) _mm_slli_epi16_runtime(a, count)
#define _mm_srli_epi32(a, count) _mm_srli_epi32_runtime(a, count)
#define _mm_slli_epi32(a, count) _mm_slli_epi32_runtime(a, count)

#endif /* _SSE2_RUNTIME_SHIFT_H */
