#ifndef HYMOD_H
#define HYMOD_H

#include "LinearReservoir.h"

//! Hymod paramaters struct
/*!
    This structure provides storage for the parameters of the hymod hydrological model
*/

struct hymod_params
{
    double max_storage;     //!< maximum amount of water stored
    double a;               //!< coefficent for distributing runoff and slowflow
    double b;               //!< exponent for flux equation
    double Ks;              //!< slow flow coefficent
    double Kq;              //!< quick flow coefficent
    double n;               //!< number of nash cascades
};

//! Hymod state structure
/*!
    This structure provides storage for the state used by hymod hydrological model at a particular time step.
    Warning: to allow bindings to other languages the storage amounts for the reserviors in the nash cascade
    are stored in a pointer which is not allocated or deallocated by this code. Backing storage for these arrays
    must be provide and managed by the creator of these structures.
*/

struct hymod_state
{
    double storage;         //!< the current water storage of the modeled area
    double* Sr;             //!< amount of water in each linear reservoir unsafe for binding suport check latter

    //! Constructuor for hymod state
    /*!
        Default constructor for hymod_state objects. This is necessary for the structure to be usable in a map
        Warning: the value of the Sr pointer must be set before this object is used by the hymod_kernel::run() or hymod()
    */
    hymod_state(double inital_storage = 0.0, double* storage_reservoir_ptr = 0x0) :
        storage(inital_storage), Sr(storage_reservoir_ptr)
    {

    }
};

//! Hymod flux structure
/*!
    This structure provides storage for the fluxes generated by hymod at any time step
*/

struct hymod_fluxes
{
    double slow_flow_in;    //!< The flow entering slow flow at this time step
    double slow_flow_out;   //!< The flow exiting slow flow at this time step
    double runnoff;         //!< The caluclated runoff amount for this time step
    double et_loss;         //!< The amount of water lost to

    //! Constructor for hymod fluxes
    /*!
        Default constructor for hymod fluxes this is needed for the structs to be storeable in C++ containers.
    */

    hymod_fluxes(double si = 0.0, double so = 0.0, double r = 0.0, double et = 0.0) :
        slow_flow_in(si), slow_flow_out(so), runnoff(r), et_loss(et)
    {}
};

//! Hymod kernel class
/*!
    This class implements the hymod hydrological model


*/

class hymod_kernel
{
    public:

    //! stub function to simulate losses due to evapotransportation
    static double calc_et(double soil_m, void* et_params)
    {
        return 0.0;
    }

    //! run one time step of hymod
    static void run(
        hymod_params params,        //!< static parameters for hymod
        hymod_state state,          //!< model state
        hymod_fluxes ks_fluxes,     //!< fluxes from Ks time steps in the past
        hymod_state& new_state,     //!< model state struct to hold new model state
        hymod_fluxes& fluxes,       //!< model flux object to hold calculated fluxes
        double input_flux,          //!< the amount water entering the system this time step
        void* et_params)            //!< parameters for the et function
    {

        // initalize the nash cascade
        std::vector<LinearReservoir> nash_cascade;

        nash_cascade.resize(params.n);
        for ( unsigned long i = 0; i < nash_cascade.size(); ++i )
        {
            nash_cascade[i] = LinearReservoir(params.Kq, state.Sr[i]);
        }

        // add flux to the current state
        state.storage += input_flux;

        // calculate fs, runoff and slow
        double fs = (1.0 - std::pow((1.0 - state.storage/params.max_storage),params.b) );
        double runoff = fs * params.a;
        double slow = fs * (1.0 - params.a );
        double soil_m = state.storage - fs;

        // calculate et
        double et = calc_et(soil_m, et_params);

        // get the slow flow output for this time - ks
        double slow_flow_out = ks_fluxes.slow_flow_in;

        for(unsigned long int i = 0; i < nash_cascade.size(); ++i)
        {
            runoff = nash_cascade[i].response(runoff);
        }

        // record all fluxs
        fluxes.slow_flow_in = slow;
        fluxes.slow_flow_out = slow_flow_out;
        fluxes.runnoff = runoff;
        fluxes.et_loss = et;

        // update new state
        new_state.storage = soil_m - et;
        for ( unsigned long i = 0; i < nash_cascade.size(); ++i )
        {
            new_state.Sr[i] = nash_cascade[i].get_storage();
        }
    }
};

extern "C"
{
    /*!
        C entry point for calling hymod_kernel::run
    */

    inline void hymod(hymod_params params,  //!< static parameters for hymod
        hymod_state state,                  //!< model state
        hymod_fluxes ks_fluxes,             //!< fluxes from Ks time steps in the past
        hymod_state* new_state,             //!< model state struct to hold new model state
        hymod_fluxes* fluxes,               //!< model flux object to hold calculated fluxes
        double input_flux,                  //!< the amount water entering the system this time step
        void* et_params)                    //!< parameters for the et function
    {
        hymod_kernel::run(params, state, ks_fluxes, *new_state, *fluxes, input_flux, et_params);
    }
}

#endif
