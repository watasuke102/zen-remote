#pragma once

#include <memory>

#include "zen-remote/loop.h"

namespace zen::remote::server {

struct IRemote {
  virtual ~IRemote() = default;

  virtual void Start() = 0;

  // Call IRemote::Stop before destroy
  virtual void Stop() = 0;
};

std::unique_ptr<IRemote> CreateRemote(std::unique_ptr<ILoop> loop);

}  // namespace zen::remote::server
