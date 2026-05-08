/* src/engine/llama/Llama_Engine.h */
#ifndef SALIX_NANSHE_LLAMA_ENGINE_H
#define SALIX_NANSHE_LLAMA_ENGINE_H

#include <core/IEngine.h>
#include <memory>


namespace nanshe::engine::llama {
    class Llama_Engine : public nanshe::core::IEngine {
    public:
        Llama_Engine();
        ~Llama_Engine() override;

        // IEngine Contract
        bool load_model_asset(const std::string& asset_path) override;
        void unload_model_asset() override;
        std::string predict_next_sequence(const std::string& input_text) override;
        size_t get_memory_footprint() const override;
        void clear_compute_state() override;
    
    private:
        struct Pimpl;
        std::unique_ptr<Pimpl> pimpl;

    };


}  // namespace nanshe::engine::llama 


#endif
