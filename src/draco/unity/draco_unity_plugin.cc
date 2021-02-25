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
#include "draco/unity/draco_unity_plugin.h"

#ifdef BUILD_UNITY_PLUGIN

namespace draco {

void ReleaseUnityMesh(DracoToUnityMesh **mesh_ptr) {
  DracoToUnityMesh *mesh = *mesh_ptr;
  if (!mesh)
    return;
  if (mesh->indices) {
    delete[] mesh->indices;
    mesh->indices = nullptr;
  }
  if (mesh->position) {
    delete[] mesh->position;
    mesh->position = nullptr;
  }
  if (mesh->has_normal && mesh->normal) {
    delete[] mesh->normal;
    mesh->has_normal = false;
    mesh->normal = nullptr;
  }
  if (mesh->has_texcoord && mesh->texcoord) {
    delete[] mesh->texcoord;
    mesh->has_texcoord = false;
    mesh->texcoord = nullptr;
  }
  if (mesh->has_color && mesh->color) {
    delete[] mesh->color;
    mesh->has_color = false;
    mesh->color = nullptr;
  }
  if (mesh->has_weights && mesh->weights) {
    delete[] mesh->weights;
    mesh->has_weights = false;
    mesh->weights = nullptr;
  }
  if (mesh->has_joints && mesh->joints) {
    delete[] mesh->joints;
    mesh->has_joints = false;
    mesh->joints = nullptr;
  }
  delete mesh;
  *mesh_ptr = nullptr;
}

template <typename MeshOrPointCloud>
bool DecodePositionsAttribute(MeshOrPointCloud* meshOrPts, DracoToUnityMesh *unity_mesh) {
  unity_mesh->position = new float[meshOrPts->num_points() * 3];
  const auto pos_att =
      meshOrPts->GetNamedAttribute(draco::GeometryAttribute::POSITION);
  for (draco::PointIndex i(0); i < meshOrPts->num_points(); ++i) {
    const draco::AttributeValueIndex val_index = pos_att->mapped_index(i);
    float *dest = unity_mesh->position + i.value() * 3;
    if (!pos_att->ConvertValue<float>(val_index, 3, dest)) {
      return false;
    }
    // right-hand to left-handed coordinate system switch: flip Z axis
    *(dest + 2) *= -1;
  }
  return true;
}

template <typename MeshOrPointCloud>
bool DecodeNormalsAttribute(MeshOrPointCloud *meshOrPts,
                           DracoToUnityMesh *unity_mesh) {
  // Get normal attributes.
  const auto normal_att =
      meshOrPts->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
  if (normal_att != nullptr) {
    unity_mesh->normal = new float[meshOrPts->num_points() * 3];
    unity_mesh->has_normal = true;
    for (draco::PointIndex i(0); i < meshOrPts->num_points(); ++i) {
      float *dest = unity_mesh->normal + i.value() * 3;
      const draco::AttributeValueIndex val_index = normal_att->mapped_index(i);
      if (!normal_att->ConvertValue<float, 3>(val_index, dest)) {
        return false;
      }
      // right-hand to left-handed coordinate system switch: flip Z axis
      *(dest + 2) *= -1;
    }
  }
  return true;
}

template <typename MeshOrPointCloud>
bool DecodeColorsAttribute(MeshOrPointCloud *meshOrPts,
                           DracoToUnityMesh *unity_mesh) {
  // Get color attributes.
  const auto color_att =
      meshOrPts->GetNamedAttribute(draco::GeometryAttribute::COLOR);
  if (color_att != nullptr) {
    unity_mesh->color = new float[meshOrPts->num_points() * 4];
    unity_mesh->has_color = true;
    for (draco::PointIndex i(0); i < meshOrPts->num_points(); ++i) {
      const draco::AttributeValueIndex val_index = color_att->mapped_index(i);
      if (!color_att->ConvertValue<float, 4>(
              val_index, unity_mesh->color + i.value() * 4)) {
        return false;
      }
      if (color_att->num_components() < 4) {
        // If the alpha component wasn't set in the input data we should set
        // it to an opaque value.
        unity_mesh->color[i.value() * 4 + 3] = 1.f;
      }
    }
  }
  return true;
}

template <typename MeshOrPointCloud>
bool DecodeTextureCoordinatesAttribute(MeshOrPointCloud *meshOrPts,
                                       DracoToUnityMesh *unity_mesh) {
  // Get texture coordinates attributes.
  const auto texcoord_att =
      meshOrPts->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);
  if (texcoord_att != nullptr) {
    unity_mesh->texcoord = new float[meshOrPts->num_points() * 2];
    unity_mesh->has_texcoord = true;
    for (draco::PointIndex i(0); i < meshOrPts->num_points(); ++i) {
      const draco::AttributeValueIndex val_index =
          texcoord_att->mapped_index(i);
      float *dest = unity_mesh->texcoord + i.value() * 2;
      if (!texcoord_att->ConvertValue<float, 2>(val_index, dest)) {
        return false;
      }
      // right-handed top left to left-handed lower left conversion
      *(dest + 1) = 1 - *(dest + 1);
    }
  }
  return true;
}

template <typename MeshOrPointCloud>
bool DecodeWeightsAttribute(MeshOrPointCloud *meshOrPts,
                              DracoToUnityMesh *unity_mesh, int32_t weightsId) {
  // Get skin bone weights attributes.
  const auto weights_att = meshOrPts->GetAttributeByUniqueId(weightsId);
  if (weights_att != nullptr && weights_att->num_components() == 4) {
    unity_mesh->weights = new float[meshOrPts->num_points() * 4];
    unity_mesh->has_weights = true;
    for (draco::PointIndex i(0); i < meshOrPts->num_points(); ++i) {
      const draco::AttributeValueIndex val_index = weights_att->mapped_index(i);
      float *dest = unity_mesh->weights + i.value() * 4;
      if (!weights_att->ConvertValue<float, 4>(val_index, dest)) {
        return false;
      } 
    }
  }
  return true;
}

template <typename MeshOrPointCloud>
bool DecodeJointsAttribute(MeshOrPointCloud *meshOrPts,
                           DracoToUnityMesh *unity_mesh,
                  int32_t jointsId) {
  // Get skin joints (bone ID) attributes.
  const auto joints_att = meshOrPts->GetAttributeByUniqueId(jointsId);
  if (joints_att != nullptr && joints_att->num_components() == 4) {
    unity_mesh->joints = new int32_t[meshOrPts->num_points() * 4];
    unity_mesh->has_joints = true;
    for (draco::PointIndex i(0); i < meshOrPts->num_points(); ++i) {
      const draco::AttributeValueIndex val_index = joints_att->mapped_index(i);
      int32_t *dest = unity_mesh->joints + i.value() * 4;
      if (!joints_att->ConvertValue<int32_t, 4>(val_index, dest)) {
        return false;
      }
    }
  }
  return true;
}

int DecodePointCloud(draco::DecoderBuffer *buffer, DracoToUnityMesh **tmp_mesh,
                     int32_t weightsId, int32_t jointsId) {
  draco::Decoder decoder;
  auto statusor = decoder.DecodePointCloudFromBuffer(buffer);
  if (!statusor.ok()) {
    return -3;
  }
  std::unique_ptr<draco::PointCloud> in_pts = std::move(statusor).value();

  *tmp_mesh = new DracoToUnityMesh();
  DracoToUnityMesh *unity_mesh = *tmp_mesh;
  unity_mesh->num_vertices = in_pts->num_points();

  auto draco_pts = in_pts.get();
  if (!DecodePositionsAttribute(draco_pts, unity_mesh) ||
      !DecodeNormalsAttribute(draco_pts, unity_mesh) ||
      !DecodeColorsAttribute(draco_pts, unity_mesh) ||
      !DecodeTextureCoordinatesAttribute(draco_pts, unity_mesh) || 
      (weightsId >= 0 &&
       !DecodeWeightsAttribute(draco_pts, unity_mesh, weightsId)) ||
      (jointsId >= 0 &&
       !DecodeJointsAttribute(draco_pts, unity_mesh, jointsId))
      ) {
    ReleaseUnityMesh(&unity_mesh);
    return -8;
  }

  return in_pts->num_points();
}


int DecodeMesh(draco::DecoderBuffer *buffer, DracoToUnityMesh **tmp_mesh,
               int32_t weightsId, int32_t jointsId) {
  draco::Decoder decoder;
  auto statusor = decoder.DecodeMeshFromBuffer(buffer);
  if (!statusor.ok()) {
    return -3;
  }
  std::unique_ptr<draco::Mesh> in_mesh = std::move(statusor).value();

  *tmp_mesh = new DracoToUnityMesh();
  DracoToUnityMesh *unity_mesh = *tmp_mesh;
  unity_mesh->num_faces = in_mesh->num_faces();
  unity_mesh->num_vertices = in_mesh->num_points();

  unity_mesh->indices = new int[in_mesh->num_faces() * 3];
  int32_t tmp;
  for (draco::FaceIndex face_id(0); face_id < in_mesh->num_faces(); ++face_id) {
    const Mesh::Face &face = in_mesh->face(draco::FaceIndex(face_id));
    int32_t *dest = unity_mesh->indices + face_id.value() * 3;
    memcpy(dest, reinterpret_cast<const int32_t *>(face.data()),
           sizeof(int32_t) * 3);
    // right-hand to left-handed coordinate system switch: change triangle order
    tmp = *(dest + 1);
    *(dest + 1) = *(dest + 2);
    *(dest + 2) = tmp;
  }

  auto draco_mesh = in_mesh.get();
  if (!DecodePositionsAttribute(draco_mesh, unity_mesh) ||
      !DecodeNormalsAttribute(draco_mesh, unity_mesh) ||
      !DecodeColorsAttribute(draco_mesh, unity_mesh) ||
      !DecodeTextureCoordinatesAttribute(draco_mesh, unity_mesh) ||
      (weightsId >= 0 &&
       !DecodeWeightsAttribute(draco_mesh, unity_mesh, weightsId)) ||
      (jointsId >= 0 &&
       !DecodeJointsAttribute(draco_mesh, unity_mesh, jointsId))) {
    ReleaseUnityMesh(&unity_mesh);
    return -8;
  }

  return in_mesh->num_faces();
}

int DecodeMeshForUnity(char *data, unsigned int length,
                       DracoToUnityMesh **tmp_mesh, int32_t weightsId,
                       int32_t jointsId) {
  draco::DecoderBuffer buffer;
  buffer.Init(data, length);
  auto type_statusor = draco::Decoder::GetEncodedGeometryType(&buffer);
  if (!type_statusor.ok()) {
    // TODO(draco-eng): Use enum instead.
    return -1;
  }
  const draco::EncodedGeometryType geom_type = type_statusor.value();

  if (geom_type == draco::TRIANGULAR_MESH) {
    return DecodeMesh(&buffer, tmp_mesh, weightsId, jointsId);
  } else if (geom_type == draco::POINT_CLOUD) {
    return DecodePointCloud(&buffer, tmp_mesh, weightsId, jointsId);
  } else {
    return -2;
  }
}

}  // namespace draco

#endif  // BUILD_UNITY_PLUGIN
