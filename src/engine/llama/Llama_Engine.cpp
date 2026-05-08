/* src/engine/llama/Llama_Engine.cpp */
#include <engine/llama/Llama_Engine.h>
#include <llama.h>
#include <iostream>
#include <vector>

namespace nanshe::engine::llama {

    struct Llama_Engine::Pimpl {
        llama_model* _ai_model = nullptr;
        llama_context* _compute_context = nullptr;
        const struct llama_vocab* _vocab = nullptr;
    };

    Llama_Engine::Llama_Engine() : pimpl(std::make_unique<Pimpl>()) {
        llama_backend_init();
    }

    Llama_Engine::~Llama_Engine() {
        unload_model_asset();
        llama_backend_free();
    }

    bool Llama_Engine::load_model_asset(const std::string& asset_path) {
        if (asset_path.empty()) return false;

        llama_model_params model_parameters = llama_model_default_params();
        pimpl->_ai_model = llama_model_load_from_file(asset_path.c_str(), model_parameters);

        if (pimpl->_ai_model == nullptr) return false;

        pimpl->_vocab = llama_model_get_vocab(pimpl->_ai_model);

        llama_context_params context_parameters = llama_context_default_params();
        context_parameters.n_ctx = 2048;
        context_parameters.n_threads = 4;
        
        pimpl->_compute_context = llama_init_from_model(pimpl->_ai_model, context_parameters);
        return pimpl->_compute_context != nullptr;
    }

    void Llama_Engine::unload_model_asset() {
        if (pimpl->_compute_context) {
            llama_free(pimpl->_compute_context);
            pimpl->_compute_context = nullptr;
        }
        if (pimpl->_ai_model) {
            llama_model_free(pimpl->_ai_model);
            pimpl->_ai_model = nullptr;
        }
        pimpl->_vocab = nullptr;
    }

   
    std::string Llama_Engine::predict_next_sequence(const std::string& input_text) {
        if (!pimpl->_compute_context || !pimpl->_vocab) return "Error: Engine not ready.";

        // 1. TOKENIZATION: Map text to internal "AI integers"
        std::vector<llama_token> tokens_list(input_text.length() + 1);
        int n_tokens = llama_tokenize(pimpl->_vocab, input_text.c_str(), input_text.length(), 
                                     tokens_list.data(), tokens_list.size(), true, false);
        tokens_list.resize(n_tokens);

        // 2. BATCH PREP: Assembler-grade memory mapping for the decode process
        llama_batch batch = llama_batch_init(2048, 0, 1);
        for (int i = 0; i < n_tokens; ++i) {
            batch.token[i] = tokens_list[i];
            batch.pos[i]   = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = (i == n_tokens - 1); // Only calculate logits for the end
        }
        batch.n_tokens = n_tokens;

        // 3. INITIAL DECODE: Feed the prompt to the backend
        if (llama_decode(pimpl->_compute_context, batch) != 0) {
            llama_batch_free(batch);
            return "Error: Hardware decode failure.";
        }

        // 4. GENERATION LOOP: Predict tokens until the model hits EOT (End of Text)
        std::string result_text = "";
        int max_tokens = 256; 
        
        for (int n_cur = 0; n_cur < max_tokens; n_cur++) {
            // SAMPLING: Pick the most probable next token (Greedy approach)
            struct llama_sampler * smpl = llama_sampler_init_greedy();
            llama_token new_token = llama_sampler_sample(smpl, pimpl->_compute_context, -1);
            llama_sampler_free(smpl);

            // CHECK: Did the model finish the sentence?
            if (llama_vocab_is_eog(pimpl->_vocab, new_token)) break;

            // DETOKENIZATION: Map the integer back to a human string fragment
            char piece_buf[128];
            int n_char = llama_token_to_piece(pimpl->_vocab, new_token, piece_buf, sizeof(piece_buf), 0, true);
            result_text.append(piece_buf, n_char);

            // FEEDBACK: Prepare for the next "lap" using the KV cache
            batch.token[0] = new_token;
            batch.pos[0]   = n_tokens + n_cur;
            batch.n_seq_id[0] = 1;
            batch.seq_id[0][0] = 0;
            batch.logits[0] = true;
            batch.n_tokens = 1;
            
            if (llama_decode(pimpl->_compute_context, batch) != 0) break;
        }

        llama_batch_free(batch);
        return result_text;
    }

   size_t Llama_Engine::get_memory_footprint() const {
        if (!pimpl->_ai_model || !pimpl->_compute_context) return 0;

        // 1. Get the size of the model weights (Returns uint64_t)
        uint64_t model_bytes = llama_model_size(pimpl->_ai_model);

        // 2. Get the context size (The KV Cache + Work Buffers)
        // In your 2026 version, llama_state_get_size is the standard for 
        // querying the total bytes allocated for the context.
        uint64_t context_bytes = llama_state_get_size(pimpl->_compute_context);

        // Cast the sum to size_t for the IEngine interface
        return static_cast<size_t>(model_bytes + context_bytes);
    }



    void Llama_Engine::clear_compute_state() {
        if (pimpl->_compute_context) {
            // MODERN API: As found in your Image 009646
            // We grab the memory handle and clear the sequence (-1 = ALL)
            auto mem = llama_get_memory(pimpl->_compute_context);
            llama_memory_seq_rm(mem, -1, -1, -1);
        }
    }

} // namespace nanshe::engine::llama
