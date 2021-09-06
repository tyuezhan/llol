#include "sv/node/pcl.h"

#include <pcl_conversions/pcl_conversions.h>
#include <tbb/parallel_for.h>

namespace sv {

void Pano2Cloud(const DepthPano& pano,
                const std_msgs::Header header,
                CloudXYZ& cloud) {
  const auto size = pano.size();
  if (cloud.empty()) {
    cloud.resize(pano.total());
    cloud.width = size.width;
    cloud.height = size.height;
  }

  pcl_conversions::toPCL(header, cloud.header);
  tbb::parallel_for(tbb::blocked_range<int>(0, size.height),
                    [&](const auto& blk) {
                      for (int r = blk.begin(); r < blk.end(); ++r) {
                        for (int c = 0; c < size.width; ++c) {
                          const auto rg = pano.RangeAt({c, r});
                          auto& pc = cloud.at(c, r);
                          if (rg == 0) {
                            pc.x = pc.y = pc.z = kNaNF;
                          } else {
                            const auto pp = pano.model.Backward(r, c, rg);
                            pc.x = pp.x;
                            pc.y = pp.y;
                            pc.z = pp.z;
                          }
                        }
                      }
                    });
}

void Sweep2Cloud(const LidarSweep& sweep,
                 const std_msgs::Header header,
                 CloudXYZI& cloud) {
  const auto size = sweep.size();
  if (cloud.empty()) {
    cloud.resize(sweep.total());
    cloud.width = size.width;
    cloud.height = size.height;
  }

  pcl_conversions::toPCL(header, cloud.header);
  tbb::parallel_for(tbb::blocked_range<int>(0, size.height),
                    [&](const auto& blk) {
                      for (int r = blk.begin(); r < blk.end(); ++r) {
                        for (int c = 0; c < size.width; ++c) {
                          const bool col_in_curr =
                              (sweep.curr.start <= c && c < sweep.curr.end);
                          float intensity = col_in_curr ? 1.0 : 0.5;

                          const auto& tf = sweep.TfAt(c);
                          const auto& xyzr = sweep.XyzrAt({c, r});
                          auto& pc = cloud.at(c, r);
                          if (std::isnan(xyzr[0])) {
                            pc.x = pc.y = pc.z = pc.intensity = kNaNF;
                          } else {
                            Eigen::Map<const Vector3f> xyz(&xyzr[0]);
                            pc.getArray3fMap() = tf * xyz;
                            pc.intensity = intensity;
                          }
                        }
                      }
                    });
}

}  // namespace sv