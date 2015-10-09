#include "despeckcl.h"

#include <CL/cl.h>
#include <chrono>
#include <string.h> // for memcpy
#include <vector>
#include <string>

#include "cl_wrappers.h"
#include "insar_data.h"
#include "nlsar_filter_sub_image.h"
#include "sub_images.h"
#include "stats.h"
#include "get_dissims.h"
#include "clcfg.h"
#include "logging.h"
#include "best_params.h"
#include "timings.h"

int despeckcl::nlsar(float* ampl_master,
                     float* ampl_slave,
                     float* dphase,
                     float* ampl_filt,
                     float* dphase_filt,
                     float* coh_filt,
                     const int height,
                     const int width,
                     const int search_window_size,
                     const std::vector<int> patch_sizes,
                     const std::vector<int> scale_sizes,
                     const bbox training_dims,
                     std::vector<std::string> enabled_log_levels)
{
    timings::map tm;

    const int patch_size_max = *std::max_element(patch_sizes.begin(), patch_sizes.end());
    const int scale_size_max = *std::max_element(scale_sizes.begin(), scale_sizes.end());

    // FIXME
    const int dimension = 2;
    const int lut_size = 256;
    // overlap consists of:
    // - (patch_size_max - 1)/2 + (search_window_size - 1)/2 for similarities
    // - (window_width - 1)/2 for spatial averaging of covariance matrices
    const int overlap = (patch_size_max - 1)/2 + (search_window_size - 1)/2 + (scale_size_max - 1)/2;

    // the sub image size needs to be picked so that all buffers fit in the GPUs memory
    // Use the following formula to get a rough estimate of the memory consumption
    // sws: search window size
    // sis: sub image size
    // the factor of 5 is due to the number of large buffers
    // similarty, patch_similarity, kullback_leibler, patch_kullback_leibler, weights
    // memory consumption in bytes:
    // sws^2 * sis^2 * n_threads * 4 (float) * 5
    const int sub_image_size = 100;

    logging_setup(enabled_log_levels);


    LOG(INFO) << "filter parameters";
    LOG(INFO) << "search window size: " << search_window_size;
    auto intvec2string = [] (std::vector<int> ints) { return std::accumulate(ints.begin(), ints.end(), (std::string)"",
                                                                             [] (std::string acc, int i) {return acc + std::to_string(i) + ", ";});
                                                    };

    LOG(INFO) << "patch_sizes: " << intvec2string(patch_sizes);
    LOG(INFO) << "scale_sizes: " << intvec2string(scale_sizes);
    LOG(INFO) << "overlap: " << overlap;

    LOG(INFO) << "data dimensions";
    LOG(INFO) << "height: " << height;
    LOG(INFO) << "width: " << width;

    // legacy opencl setup
    cl::Context context = opencl_setup();

    // new build kernel interface
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> elapsed_seconds = end-start;
    start = std::chrono::system_clock::now();
    VLOG(0) << "Building kernels";
    nlsar::cl_wrappers nlsar_cl_wrappers (context, search_window_size, dimension);
    end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    VLOG(0) << "Time it took to build all kernels: " << elapsed_seconds.count() << "secs";

    // prepare data
    insar_data total_image{ampl_master, ampl_slave, dphase,
                           ampl_filt, dphase_filt, coh_filt,
                           height, width};
    std::map<nlsar::params, nlsar::stats> nlsar_stats;
    for(int patch_size : patch_sizes) {
        for(int scale_size : scale_sizes) {
            nlsar_stats.emplace(nlsar::params{patch_size, scale_size},
                                nlsar::stats(nlsar::get_dissims(context, total_image.get_sub_insar_data(training_dims), patch_size, scale_size), patch_size, lut_size));
        }
    }
    total_image.pad(overlap);
    insar_data total_image_temp = total_image;

    // filtering
    start = std::chrono::system_clock::now();
    LOG(INFO) << "starting filtering";
#pragma omp parallel shared(total_image, total_image_temp)
{
#pragma omp master
    {
    for( auto boundaries : gen_sub_images(total_image.height, total_image.width, sub_image_size, overlap) ) {
#pragma omp task firstprivate(boundaries)
        {
        insar_data sub_image = total_image.get_sub_insar_data(boundaries);
        try {
            timings::map tm_sub = filter_sub_image(context, nlsar_cl_wrappers, // opencl stuff
                                                  sub_image, // data
                                                  search_window_size,
                                                  patch_sizes,
                                                  scale_sizes,
                                                  dimension,
                                                  nlsar_stats);
#pragma omp critical
            tm = timings::join(tm, tm_sub);
        } catch (cl::Error error) {
            LOG(ERROR) << error.what() << "(" << error.err() << ")";
            LOG(ERROR) << "ERR while filtering sub image";
            std::terminate();
        }
        total_image_temp.write_sub_insar_data(sub_image, overlap, boundaries);
        }
    }
#pragma omp taskwait
    total_image = total_image_temp;
    }
}
    total_image.unpad(overlap);
    LOG(INFO) << "filtering done";
    timings::print(tm);
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> duration = end-start;
    std::cout << "filtering ran for " << duration.count() << " secs" << std::endl;

    memcpy(ampl_filt,   total_image.amp_filt, sizeof(float)*height*width);
    memcpy(dphase_filt, total_image.phi_filt, sizeof(float)*height*width);
    memcpy(coh_filt,    total_image.coh_filt, sizeof(float)*height*width);

    return 0;
}
