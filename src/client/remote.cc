#include "client/remote.h"

#include "client/grpc-server.h"
#include "client/resource-pool.h"
#include "client/session-manager.h"
#include "client/session.h"
#include "core/logger.h"

namespace zen::remote::client {

Remote::Remote(std::unique_ptr<ILoop> loop) : loop_(std::move(loop)) {}

void
Remote::Start()
{
  session_manager_.Start();

  grpc_server_ = std::make_unique<GrpcServer>("0.0.0.0", kGrpcPort, this);
  grpc_server_->Start();
}

void
Remote::UpdateScene()
{
  auto execute_count = update_rendering_queue_.commit_count();

  for (uint i = 0; i < execute_count; i++) {
    if (update_rendering_queue_.ExecuteOnce() == false) break;
  }
}

void
Remote::Render(Camera* camera)
{
  auto pool = session_manager_.GetCurrentResourcePool();
  if (!pool) return;

  glEnable(GL_DEPTH_TEST);

  pool->virtual_objects()->ForEach(
      [camera](const std::shared_ptr<VirtualObject>& virtual_object) {
        if (virtual_object->commited()) virtual_object->Render(camera);
      });
}

std::unique_ptr<IRemote>
CreateRemote(std::unique_ptr<ILoop> loop)
{
  return std::make_unique<Remote>(std::move(loop));
}

}  // namespace zen::remote::client
