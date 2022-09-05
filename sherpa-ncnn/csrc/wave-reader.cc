/**
 * Copyright      2021  Xiaomi Corporation (authors: Fangjun Kuang)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "sherpa-ncnn/csrc/wave-reader.h"

namespace sherpa_ncnn {
namespace {
// see http://soundfile.sapp.org/doc/WaveFormat/
//
// Note: We assume little endian here
// TODO(fangjun): Support big endian
struct WaveHeader {
  void Validate() const {
    //                       F F I R
    assert(chunk_id == 0x46464952);
    assert(chunk_size == 36 + subchunk2_size);
    //                     E V A W
    assert(format == 0x45564157);
    assert(subchunk1_id == 0x20746d66);
    assert(subchunk1_size == 16); // 16 for PCM
    assert(audio_format == 1);    // 1 for PCM
    assert(num_channels == 1);    // we support only single channel for now
    assert(byte_rate == sample_rate * num_channels * bits_per_sample / 8);
    assert(block_align == num_channels * bits_per_sample / 8);
    assert(bits_per_sample == 16); // we support only 16 bits per sample
  }

  int32_t chunk_id;
  int32_t chunk_size;
  int32_t format;
  int32_t subchunk1_id;
  int32_t subchunk1_size;
  int16_t audio_format;
  int16_t num_channels;
  int32_t sample_rate;
  int32_t byte_rate;
  int16_t block_align;
  int16_t bits_per_sample;
  int32_t subchunk2_id;
  int32_t subchunk2_size;
};
static_assert(sizeof(WaveHeader) == 44, "");

// Read a wave file of mono-channel.
// Return its samples normalized to the range [-1, 1).
std::vector<float> ReadWaveImpl(std::istream &is, float *sample_rate) {
  WaveHeader header;
  is.read(reinterpret_cast<char *>(&header), sizeof(header));
  assert((bool)is);

  header.Validate();

  *sample_rate = header.sample_rate;

  // header.subchunk2_size contains the number of bytes in the data.
  // As we assume each sample contains two bytes, so it is divided by 2 here
  std::vector<int16_t> samples(header.subchunk2_size / 2);

  is.read(reinterpret_cast<char *>(samples.data()), header.subchunk2_size);

  assert((bool)is);

  std::vector<float> ans(samples.size());
  for (int32_t i = 0; i != ans.size(); ++i) {
    ans[i] = samples[i] / 32768.;
  }

  return ans;
}

} // namespace

std::vector<float> ReadWave(const std::string &filename,
                            float expected_sample_rate) {
  std::ifstream is(filename, std::ifstream::binary);
  float sample_rate;
  auto samples = ReadWaveImpl(is, &sample_rate);
  if (expected_sample_rate != sample_rate) {
    std::cerr << "Expected sample rate: " << expected_sample_rate
              << ". Given: " << sample_rate << ".\n";
    exit(-1);
  }
  return samples;
}

} // namespace sherpa_ncnn