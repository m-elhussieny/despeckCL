despeckCL
=========

A C++ library for InSAR and PolSAR denoising images using OpenCL with Python bindings.
The code is mostly developed and tested on an Intel HD4000, Intel UHD Graphics 620 and a NVIDIA Tesla V100.
I also had some success running it on an AMD RX480, but don't hold your breath.
To get started have a look at the examples directory, the <a href="https://gbaier.github.io/despeckCL/">sphinx documentation</a> or checkout despeckcl.h.
If you have any comments, questions or suggestions just write me an email.

Implemented Filters
-------------------

So far the following filters are implemented:

* **Boxcar**: The simple boxcar filter
* **Goldstein**: The Goldstein InSAR filter, *Goldstein, R. M.; Werner, C. L., "Radar interferogram filtering for geophysical applications," Geophysical Research Letters,vol. 25, no. 21, pp.4035,4038, 1998*
* **NL-SAR**: A nonlocal filter for SAR, InSAR and PolSAR, *Deledalle, C.-A.; Denis, L.; Tupin, F.; Reigber, A.; Jäger, M., "NL-SAR: A Unified Nonlocal Framework for Resolution-Preserving (Pol)(In)SAR Denoising," Geoscience and Remote Sensing, IEEE Transactions on , vol.53, no.4, pp.2021,2038, April 2015*
    * currently this implementation uses only a single search window of fixed size in contrast to the original paper, which uses multiple search window sizes.
    * so far only InSAR filtering is supported.
* **NL-InSAR**: A nonlocal InSAR filter, *Deledalle, C.-A.; Denis, L.; Tupin, F., "NL-InSAR: Nonlocal Interferogram Estimation," Geoscience and Remote Sensing, IEEE Transactions on , vol.49, no.4, pp.1441,1452, April 2011*

Requirements
------------

1. A compiler with C++14 and at least OpenMP 4.0 support. So far development is done using gcc but I also occasionally test using clang.
2. An OpenCL Runtime:
    * <a href="https://developer.nvidia.com/cuda-toolkit">CUDA Toolkit</a> from NVIDIA also has OpenCL support
    * <a href="http://www.freedesktop.org/wiki/Software/Beignet/">Beignet</a> for Intel GPUs under Linux
    * <a href="https://rocm.github.io/QuickStartOCL.html">AMD ROCm</a> for AMD GPUs
3. <a href="https://cmake.org/">CMake</a> of at least version 3.12.
4. <a href="https://www.boost.org/">Boost</a> is used for serialization and deserialization.
5. <a href="https://www.gnu.org/software/gsl/">GNU Scientific Library</a> provides some special functions needed by NL-SAR
6. <a href="https://github.com/clMathLibraries/clFFT">clFFT</a> AMD's OpenCL FFT library, which also works on NVIDIA and Intel devices. This is used by the Goldstein filter. Is automatically checked out from github and build when running cmake.
7. <a href="http://swig.org/">SWIG</a> for generating Python and Octave bindings (optional)
8. <a href="http://sphinx-doc.org/">Sphinx</a> for building the documentation (optional)
9. despeckCL uses <a href="https://github.com/google/googletest">Google Test</a> for unit testing and <a href="https://github.com/easylogging/easyloggingpp/">Easylogging++</a> for logging. Google Test is automatically checked out from github when building despeckCL and Easylogging++ is shipped directly with the source.

libs
------------
```shell
sudo apt-get install cmake python3-sphinx libboost-all-dev ocl-icd-opencl-dev gsl swig
```

Building, Testing and Installation
----------------------------------

```shell

$ git clone https://github.com/gbaier/despeckCL.git

$ cd despeckCL

$ mkdir build

$ cd build

$ cmake ../
```

The Python version can also be selected manually in case multiple versions are installed.
```shell
$ cmake -DPython3_ROOT_DIR=~/anaconda3/ ../
```

Compilation is straightforward.
```shell
$ make -j4

$ make test

$ make install
```
