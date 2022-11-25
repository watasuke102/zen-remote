#pragma once

#include "client/resource.h"
#include "core/common.h"
#include "core/types.h"

namespace zen::remote::client {

class GlVertexArray;
class GlProgram;
class GlTexture;
class AtomicCommandQueue;
struct Camera;

class GlBaseTechnique final : public IResource {
  enum class DrawMethod {
    kNone,
    kArrays,
  };

  union DrawArgs {
    struct {
      uint32_t mode;
      int32_t count;
      int32_t first;
    } arrays;
  };

  struct UniformVariable {
    uint32_t location;
    std::string name;
    UniformVariableType type;
    uint32_t col;
    uint32_t row;
    uint32_t count;
    bool transpose;
    std::string value;
  };

  struct TextureBinding {
    std::string name;
    std::weak_ptr<GlTexture> texture;
    uint32_t target;
  };

  struct RenderingState {
    DrawArgs draw_args;
    DrawMethod draw_method = DrawMethod::kNone;
    std::weak_ptr<GlVertexArray> vertex_array;  // nullable
    std::weak_ptr<GlProgram> program;           // nullable
    std::unordered_map<uint32_t, TextureBinding> texture_bindings;
    std::unordered_map<uint32_t, UniformVariable> uniform_variables;
  };

 public:
  DISABLE_MOVE_AND_COPY(GlBaseTechnique);
  GlBaseTechnique() = delete;
  GlBaseTechnique(uint64_t id, AtomicCommandQueue *update_rendering_queue);
  ~GlBaseTechnique();

  /** Used in the update thread */
  void Commit();

  /** Used in the update thread */
  void Bind(std::weak_ptr<GlProgram> program);

  /** Used in the update thread */
  void Bind(std::weak_ptr<GlVertexArray> vertex_array);

  /** Used in the update thread */
  void Bind(uint32_t binding, std::string name,
      std::weak_ptr<GlTexture> texture, uint32_t target);

  /** Used in the update thread */
  void GlDrawArrays(uint32_t mode, int32_t first, uint32_t count);

  /** Used in the update thread */
  void GlUniform(uint32_t location, std::string name, UniformVariableType type,
      uint32_t col, uint32_t row, uint32_t count, bool transpose,
      std::string value);

  /** Used in the rendering thread */
  void ApplyUniformVariables(
      GLuint program_id, Camera *camera, const glm::mat4 &model);

  /** Used in the rendering thread */
  void SetupTextures(GLuint program_id);

  /** Used in the rendering thread */
  void Render(Camera *camera, const glm::mat4 &model);

  uint64_t id() override;

 private:
  const uint64_t id_;
  AtomicCommandQueue *update_rendering_queue_;

  struct {
    DrawArgs draw_args;
    DrawMethod draw_method = DrawMethod::kNone;
    bool draw_method_damaged = false;

    std::weak_ptr<GlVertexArray> vertex_array;  // nullable
    bool vertex_array_damaged = false;

    std::weak_ptr<GlProgram> program;  // the value is preserved after commit
    bool program_damaged = false;

    std::unordered_map<uint32_t, TextureBinding> texture_bindings;
    bool texture_damaged = false;

    std::list<UniformVariable> uniform_variables;
  } pending_;

  std::shared_ptr<RenderingState> rendering_;
};

}  // namespace zen::remote::client
