#pragma once
/*! @file utils.h

*  @brief wavelet demo
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

typedef struct quantizer_stats {
	int min;
	int max;
	int minq;
	int maxq;
	int total;
	int overflow;
	int energy;
	unsigned short invnonlinearquant[256];
	unsigned short nonlinearquant[1025];
} quantizer_stats;


void QuantValue(int *src, int w, int x, int y, int multiplier, int midpoint, quantizer_stats *s);

void QuantizeHighpass(int *src, int w, int h, int regw, int regh, int qh, int qv, int qd, quantizer_stats *s);

void InvQuantValue(int *src, int w, int x, int y, int q, quantizer_stats *s);

void InverseQuantizeHighpass(int *src, int w, int h, int regw, int regh, int qh, int qv, int qd, quantizer_stats *s);

void OffsetBuffer(int *src, int w, int h, int regw, int regh, int offset);

void V22Wavelet(int *src, int *dest, int w, int h, int regw, int regh);

void InvertV22Wavelet(int *src, int *dest, int w, int h, int regw, int regh);

void H26Wavelet(int *src, int *dest, int w, int h, int regw, int regh);

void InvertH26Wavelet(int *src, int *dest, int w, int h, int regw, int regh);

void Limit(int *dest, int w, int h, int max);

void V26Wavelet(int *src, int *dest, int w, int h, int regw, int regh);

void InvertV26Wavelet(int *src, int *dest, int w, int h, int regw, int regh);

void PrescaleLowPass(int *dest, int w, int h, int regw, int regh, int shift);

void OutputSubbands(char *filename, int *src, int w, int h, int regw, int regh, int level);

void CopyBuff(int *src, int *dest, int w, int h);

void Stats(int *src, int w, int h, int regw, int regh);

double psnr(int *lptr, int *lptr2, int w, int h, int depth);

void ExportPGM(char *filename, unsigned char *frameBuffer, int frameWidth, int frameHeight);

void DiffBuffers(int *lptr, int *lptr2, unsigned char *cptr, int w, int h, int depth, int errorgain);

void ScaleBuffers(int *lptr, unsigned char *cptr, int w, int h, int scale);

void ScaleThumbnail(int *lptr, unsigned char *cptr, int w, int h, int lowpass_w, int lowpass_h, int scaleLow);

void GenerateDeepBufferFrom8bit(int *lptr, unsigned char *cptr, int source_w, int source_h, int w, int h, int depth);

void Init(quantizer_stats *s);