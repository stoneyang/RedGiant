#ifndef SRC_MAIN_DATA_FEATURE_VECTOR_H_
#define SRC_MAIN_DATA_FEATURE_VECTOR_H_

#include <memory>
#include <utility>
#include <vector>

#include "data/feature.h"
#include "data/feature_space.h"

namespace redgiant {
/*
 * A group of weighted features in the given feature space
 */
class FeatureVector {
public:
  typedef double FeatureWeight;
  typedef std::pair<std::shared_ptr<Feature>, FeatureWeight> FeaturePair;

  FeatureVector(std::shared_ptr<FeatureSpace> space)
  : space_(std::move(space)) {
  }

  // This constructor is mainly for test purpose
  FeatureVector(std::shared_ptr<FeatureSpace> space,
      const std::vector<std::pair<std::string, FeatureWeight>>& features)
  : space_(std::move(space)) {
    for (const auto& f: features) {
      features_.emplace_back(space_->create_feature(f.first), f.second);
    }
  }

  FeatureVector(const FeatureVector&) = default;
  FeatureVector(FeatureVector&&) = default;
  ~FeatureVector() = default;

  void add_feature(std::shared_ptr<Feature> feature, FeatureWeight weight) {
    features_.emplace_back(std::move(feature), weight);
  }

  void add_feature(const std::string& feature_key, FeatureWeight weight) {
    features_.emplace_back(space_->create_feature(feature_key), weight);
  }

  const FeatureSpace& get_space() const {
    return *space_;
  }

  std::shared_ptr<FeatureSpace> get_space_shared() const {
    return space_;
  }

  const std::string& get_space_name() const {
    return space_->get_name();
  }

  const std::vector<FeaturePair>& get_features() const {
    return features_;
  }

private:
  std::shared_ptr<FeatureSpace> space_;
  std::vector<FeaturePair> features_;
};

} /* namespace redgiant */

#endif /* SRC_MAIN_DATA_FEATURE_VECTOR_H_ */
