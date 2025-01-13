#pragma once

#include "scene_graph/components/camera.h"
#include "xihe_app.h"

namespace xihe
{
class syiAPP : public XiheApp
{
  public:
	syiAPP();
	~syiAPP() override = default;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	void request_gpu_features(backend::PhysicalDevice &gpu) override;

  private:
	xihe::sg::Camera *camera_{nullptr};
};
}        // namespace xihe