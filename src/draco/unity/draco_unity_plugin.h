// Copyright 2017 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef DRACO_UNITY_DRACO_UNITY_PLUGIN_H_
#define DRACO_UNITY_DRACO_UNITY_PLUGIN_H_

#include "draco/compression/config/compression_shared.h"
#include "draco/compression/decode.h"

#ifdef BUILD_UNITY_PLUGIN

// If compiling with Visual Studio.
#if defined(_MSC_VER)
#define EXPORT_API __declspec(dllexport)
#else
// Other platforms don't need this.
#define EXPORT_API
#endif  // defined(_MSC_VER)

namespace draco {

extern "C" {
struct EXPORT_API DracoToUnityMesh {
  DracoToUnityMesh()
      : num_faces(0),
        indices(nullptr),
        num_vertices(0),
        position(nullptr),
        has_normal(false),
        normal(nullptr),
        has_texcoord(false),
        texcoord(nullptr),
        has_color(false),
        color(nullptr),
        has_weights(false),
        weights(nullptr),
        has_joints(false),
        joints(nullptr) {}

  int32_t num_faces;
  int32_t *indices;
  int32_t num_vertices;
  float *position;
  bool has_normal;
  float *normal;
  bool has_texcoord;
  float *texcoord;
  bool has_color;
  float *color;
  bool has_weights;
  float *weights;
  bool has_joints;
  int32_t *joints;
};

void EXPORT_API ReleaseUnityMesh(DracoToUnityMesh **mesh_ptr);

/* To use this function, you do not allocate memory for |tmp_mesh|, just
 * define and pass a null pointer. Otherwise there will be memory leak.
 */
int EXPORT_API DecodeMeshForUnity(char *data, unsigned int length,
                                  DracoToUnityMesh **tmp_mesh,
                                  int32_t weightsId = -1,
                                  int32_t jointsId = -1
                                  );
int DecodeMesh(draco::DecoderBuffer *buffer, DracoToUnityMesh **tmp_mesh,
               int32_t weightsId, int32_t jointsId);

int DecodePointCloud(draco::DecoderBuffer *buffer, DracoToUnityMesh **tmp_mesh,
                     int32_t weightsId, int32_t jointsId);

}  // extern "C"

}  // namespace draco

#endif  // BUILD_UNITY_PLUGIN

#endif  // DRACO_UNITY_DRACO_UNITY_PLUGIN_H_
