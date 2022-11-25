#include "server/gl-texture.h"

#include <GLES3/gl32.h>

#include "core/logger.h"
#include "gl-texture.grpc.pb.h"
#include "server/async-grpc-caller.h"
#include "server/async-grpc-queue.h"
#include "server/buffer.h"
#include "server/job.h"
#include "server/serial-request-context.h"
#include "server/session.h"

namespace zen::remote::server {

GlTexture::GlTexture(std::shared_ptr<Session> session)
    : id_(session->NewSerial(Session::kResource)), session_(std::move(session))
{
}

void
GlTexture::Init()
{
  auto session = session_.lock();
  if (!session) return;

  auto context_raw = new SerialRequestContext(session.get());

  auto job = CreateJob([id = id_, connection = session->connection(),
                           context_raw,
                           grpc_queue = session->grpc_queue()](bool cancel) {
    auto context = std::unique_ptr<grpc::ClientContext>(context_raw);
    if (cancel) {
      return;
    }

    auto stub = GlTextureService::NewStub(connection->grpc_channel());

    auto caller = new AsyncGrpcCaller<&GlTextureService::Stub::PrepareAsyncNew>(
        std::move(stub), std::move(context),
        [connection](EmptyResponse* /*response*/, grpc::Status* status) {
          if (!status->ok() && status->error_code() != grpc::CANCELLED) {
            LOG_WARN("Failed to call remote GlTexture::New");
            connection->NotifyDisconnection();
          }
        });

    caller->request()->set_id(id);

    grpc_queue->Push(std::unique_ptr<AsyncGrpcCallerBase>(caller));
  });

  session->job_queue()->Push(std::move(job));
}

void
GlTexture::GlTexImage2D(uint32_t target, int32_t level, int32_t internal_format,
    uint32_t width, uint32_t height, int32_t border, uint32_t format,
    uint32_t type, std::unique_ptr<IBuffer> buffer)
{
  auto session = session_.lock();
  if (!session) return;

  auto context_raw = new SerialRequestContext(session.get());

  auto job = CreateJob([id = id_, connection = session->connection(),
                           context_raw, grpc_queue = session->grpc_queue(),
                           target, level, internal_format, width, height,
                           border, format, type,
                           buffer = std::move(buffer)](bool cancel) {
    auto context = std::unique_ptr<grpc::ClientContext>(context_raw);
    if (cancel) {
      return;
    }

    auto stub = GlTextureService::NewStub(connection->grpc_channel());

    auto caller =
        new AsyncGrpcCaller<&GlTextureService::Stub::PrepareAsyncGlTexImage2D>(
            std::move(stub), std::move(context),
            [connection](EmptyResponse* /*response*/, grpc::Status* status) {
              if (!status->ok() && status->error_code() != grpc::CANCELLED) {
                LOG_WARN("Failed to call remote GlTexture::GlTexImage2D");
                connection->NotifyDisconnection();
              }
            });

    size_t size = 0;
    switch (type) {
      case GL_UNSIGNED_BYTE:
        size = sizeof(GLubyte);
        break;
      case GL_BYTE:
        size = sizeof(GLbyte);
        break;
      case GL_UNSIGNED_SHORT:
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
        size = sizeof(GLushort);
        break;
      case GL_SHORT:
        size = sizeof(GLshort);
        break;
      case GL_UNSIGNED_INT:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
      case GL_UNSIGNED_INT_10F_11F_11F_REV:
      case GL_UNSIGNED_INT_5_9_9_9_REV:
      case GL_UNSIGNED_INT_24_8:
        size = sizeof(GLuint);
        break;
      case GL_INT:
        size = sizeof(GLint);
        break;
      case GL_HALF_FLOAT:
        size = sizeof(GLfloat) / 2;
        break;
      case GL_FLOAT:
      case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
        size = sizeof(GLfloat);
        break;
    }

    switch (format) {
      case GL_RED:
      case GL_RED_INTEGER:
      case GL_DEPTH_COMPONENT:
      case GL_ALPHA:
      case GL_STENCIL_INDEX:
      case GL_LUMINANCE:
        size *= 1;
        break;
      case GL_RG:
      case GL_RG_INTEGER:
      case GL_DEPTH_STENCIL:
      case GL_LUMINANCE_ALPHA:
        size *= 2;
        break;
      case GL_RGB:
      case GL_RGB_INTEGER:
        size *= 3;
        break;
      case GL_RGBA:
      case GL_RGBA_INTEGER:
        size *= 4;
        break;
    }

    caller->request()->set_id(id);
    caller->request()->set_target(target);
    caller->request()->set_level(level);
    caller->request()->set_internal_format(internal_format);
    caller->request()->set_width(width);
    caller->request()->set_height(height);
    caller->request()->set_border(border);
    caller->request()->set_format(format);
    caller->request()->set_type(type);
    caller->request()->set_data(buffer->data(), size * width * height);

    grpc_queue->Push(std::unique_ptr<AsyncGrpcCallerBase>(caller));
  });

  session->job_queue()->Push(std::move(job));
}

GlTexture::~GlTexture()
{
  auto session = session_.lock();
  if (!session) return;

  auto context_raw = new SerialRequestContext(session.get());

  auto job = CreateJob([id = id_, connection = session->connection(),
                           context_raw,
                           grpc_queue = session->grpc_queue()](bool cancel) {
    auto context = std::unique_ptr<grpc::ClientContext>(context_raw);
    if (cancel) {
      return;
    }

    auto stub = GlTextureService::NewStub(connection->grpc_channel());

    auto caller =
        new AsyncGrpcCaller<&GlTextureService::Stub::PrepareAsyncDelete>(
            std::move(stub), std::move(context),
            [connection](EmptyResponse* /*response*/, grpc::Status* status) {
              if (!status->ok() && status->error_code() != grpc::CANCELLED) {
                LOG_WARN("Failed to call remote GlTexture::Delete");
                connection->NotifyDisconnection();
              }
            });

    caller->request()->set_id(id);

    grpc_queue->Push(std::unique_ptr<AsyncGrpcCallerBase>(caller));
  });

  session->job_queue()->Push(std::move(job));
}

uint64_t
GlTexture::id()
{
  return id_;
}

std::unique_ptr<IGlTexture>
CreateGlTexture(std::shared_ptr<ISession> session)
{
  auto gl_texture =
      std::make_unique<GlTexture>(std::dynamic_pointer_cast<Session>(session));
  gl_texture->Init();

  return gl_texture;
}

}  // namespace zen::remote::server
