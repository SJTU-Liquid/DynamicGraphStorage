#include "apps/teseo_like/teseo_like_index.hpp"
#include "edge_wrapper.h"
#include "types/types.hpp"
#include "edge_wrapper/edge_driver.h"

class Teseo_Like_Wrapper {
private:
    void *m_teseo_like_index;

public:
    Teseo_Like_Wrapper() {
        m_teseo_like_index = new container::TeseoLikeIndex();
        static_cast<container::TeseoLikeIndex*>(m_teseo_like_index)->insert_vertex(0, 0);
    }
    
    ~Teseo_Like_Wrapper() {
        delete static_cast<container::TeseoLikeIndex*>(m_teseo_like_index);
    }

    void insert_edge(uint64_t dest, uint64_t timestamp) {
        static_cast<container::TeseoLikeIndex*>(m_teseo_like_index)->insert_edge(nullptr, dest, timestamp);
    }

    bool has_edge(uint64_t dest, uint64_t timestamp) {
        return static_cast<container::TeseoLikeIndex*>(m_teseo_like_index)->has_edge(nullptr, 0, dest, timestamp);
    }

    template<class F>
    void edges(F&& callback, uint64_t timestamp) {
        static_cast<container::TeseoLikeIndex*>(m_teseo_like_index)->edges(nullptr, callback, timestamp);
    }

    void init_neighbor(uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end) {
        
    }
};

void execute(std::string output_dir, int num_threads, operationType op) {
    for (int i = 2; i <= 19; i++) {
        auto wrapper = Teseo_Like_Wrapper();
        uint64_t size = (1 << i);
        EdgeDriver<Teseo_Like_Wrapper> d(wrapper, output_dir, num_threads, size);
        d.execute(op);
    }
}

#include "edge_driver_main.h"