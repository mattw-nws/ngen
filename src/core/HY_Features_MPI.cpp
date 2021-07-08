#include <HY_Features_MPI.hpp>
#include <HY_PointHydroNexus.hpp>

using namespace hy_features;

HY_Features_MPI( PartitonData partition_data,geojson::GeoJSON global_catchments, std::string* link_key, std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs)
{
	// extract the local catchment subset from the catchment data
}
