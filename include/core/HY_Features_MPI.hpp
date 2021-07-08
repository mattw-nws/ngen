#ifndef HY_FEATURES_MPI_H
#define HY_FEATURES_MPI_H

#include <unordered_map>

#include <HY_Catchment.hpp>
#include <HY_HydroNexus.hpp>
#include <network.hpp>
#include <Formulation_Manager.hpp>
#include <HY_Features.hpp>

namespace hy_features {

    class HY_Features_MPI : public HY_Features{
      public:
        HY_Features_MPI( PartitonData partition_data,geojson::GeoJSON catchments, std::string* link_key, std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs);

    };
} 

#endif //HY_GRAPH_H
