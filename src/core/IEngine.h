/* src/core/IEngine.h */
#ifndef SALIX_NANSHE_IENGINE_H
#define SALIX_NANSHE_IENGINE_H

#include <string>
#include <vector>
#include <memory>


namespace nanshe::core {

class IEngine {
public:
    virtual ~IEngine() = default;

    // Lifecycle
    virtual bool load_model_asset(const std::string& asset_path) = 0;
    virtual void unload_model_asset() = 0;

    // Execution
    virtual std::string predict_next_sequence(const std::string& input_text) = 0;

    // Performance/Internal Vitals
    virtual size_t get_memory_footprint() const = 0;
    virtual void clear_compute_state() = 0;

};


} // namespace nanshe::core

#endif
