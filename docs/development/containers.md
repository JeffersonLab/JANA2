
# Working with development containers <!-- {docsify-ignore-all} -->

## Building a CUDA Singularity container

These instructions involve creating a Docker
image locally and then producing a Singularity image from
that. These instructions were developed on a computer running
running RHEL7.9 with Docker 20.10.13 installed. Singularity
version 3.9.5 was used.

Docker is used to access the Nvidia supplied cuda image
from dockerhub as a base for the image. Several packages 
are installed in a new docker image mainly to allow building
ROOT with the singularity container. If you do not need to
use ROOT or ZeroMQ support in JANA2, then you can skip building
your own Docker image and just pull the one supplied by Nvida
into a Singularity image directly with:

```bash
singularity build cuda_11.4.2-devel-ubuntu20.04.sif docker://nvidia/cuda:11.4.2-devel-ubuntu20.04
```

The Dockerfile in this directory can be used to build an image
that will allow building JANA2 with support for both ROOT and
ZeroMQ. To build the Singularity image, execute the two commands
below.

NOTE: This will result in a Singularity image that is about
3GB. Docker claims its image takes 5.5GB. Make sure you have
plenty of disk space available for both. Also, be aware that by
default, Singularity uses a subdirectory in your home directory
for its cache so you may run into issues there if you have limited
space.

NOTE: You do NOT actually need to have cuda or a GPU installed
on the computer where you create this image(s). You can transfer
the image to a computer with one or more GPUs and the CUDA
drivers installed to actually use it.

```bash
docker build -f Dockerfile -t epsci/cuda:11.4.2-devel-ubuntu20.04 .

singularity build epsci_cuda_11.4.2-devel-ubuntu20.04.sif docker-daemon://epsci/cuda:11.4.2-devel-ubuntu20.04
```

## Building ROOT

If you are interested in building root with the container, then
here are some instructions. If you don't need ROOT then skip
this section.


The following commands will checkout, build and install root
version 6.26.02 in the local working directory. Note that you
may be able to build this *much* faster on a ramdisk if you have
enough memory. Just make sure to adjust the install location to
to somewhere more permanent.

```bash
singularity run epsci_cuda_11.4.2-devel-ubuntu20.04.sif
git clone --branch v6-26-02 https://github.com/root-project/root.git root-6.26.02
mkdir root-build/
cd root-build/
cmake -DCMAKE_INSTALL_PREFIX=../root-6.26.02-install -DCMAKE_CXX_STANDARD=14 ../root-6.26.02
make -j48 install
```


### TROUBLESHOOTING

Some centralized installations of Singularity may be configured
at a system level to automatically mount one or more network drives.
If the system you are using does not have access to all of these,
singularity may fail to launch properly. In that case you can tell
it not to bind any default directories with the "-c" option and then
explictly bind the directories you need. For example, here is how
it could be built using a ramdisk for the build and a network mounted
directory, /gapps for the install (this assumes a ramdisk already
mounted under /media/ramdisk):

```bash
singularity run -c -B /media/ramdisk,/gapps epsci_cuda_11.4.2-devel-ubuntu20.04.sif
cd /media/ramdisk
git clone --branch v6-26-02 https://github.com/root-project/root.git root-6.26.02
mkdir root-build/
cd root-build/
cmake \
    -DCMAKE_INSTALL_PREFIX=/gapps/root/Linux_Ubuntu20.04-x86_64-gcc9.3.0/root-6.26.02/ \
    -DCMAKE_CXX_STANDARD=14 \
    ../root-6.26.02

make -j48 install
```


### X11

If you run this ROOT from the container and can't open any graphics
windows it may be because you ran singularity with the "-c" option
and your ~/.Xauthority directory is not available. Just start the container
again with this explictly bound. For example:

```bash
singularity run -c -B /gapps,${HOME}/.Xauthority epsci_cuda_11.4.2-devel-ubuntu20.04.sif
source /gapps/root/Linux_Ubuntu20.04-x86_64-gcc9.3.0/root-6.26.02/bin/thisroot.sh
root
root [0] TCanvas c("c","", 400, 400)
```


## Building JANA2 with CUDA and libtorch

Singularity makes it really easy to access Nvidia GPU's from within
a container by just adding the "-nv" option. Create a singlularity
container with something like the following command and then use it
for the rest of these instructions. Note that some of these directories 
and versions represent a specific example so adjust for your specific
system as appropriate.

```
setenv SINGIMG /gapps/singularity/epsci_cuda_11.4.2-devel-ubuntu20.04.sif
singularity run -c -B /media/ramdisk,/gapps,/gluonwork1,${HOME}/.Xauthority --nv ${SINGIMG}

# Unpack libtorch and cudnn (you must download these seperately)
# n.b. Ubuntu 20.04 using gcc 9.3.0 needs to use:
# libtorch-cxx11-abi-shared-with-deps-1.11.0+cu113.zip
# while RHEL7 using SCL 9.3.0 needs to use:
# libtorch-cxx11-abi-shared-with-deps-1.11.0+cu113.zip
cd ${WORKDIR}
unzip libtorch-cxx11-abi-shared-with-deps-1.11.0+cu113.zip
tar xzf cudnn-11.4-linux-x64-v8.2.4.15.tgz


# Setup environment
export WORKDIR=./
export CMAKE_PREFIX_PATH=${WORKDIR}/libtorch/share/cmake/Torch:${CMAKE_PREFIX_PATH}
export CUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-11.4
export CUDACXX=$CUDA_TOOLKIT_ROOT_DIR/bin/nvcc
export CUDNN_LIBRARY_PATH=${WORKDIR}/cuda/lib64
export CUDNN_INCLUDE_PATH=${WORKDIR}/cuda/include
export PATH=${CUDA_TOOLKIT_ROOT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${WORKDIR}/libtorch/lib:${CUDA_TOOLKIT_ROOT_DIR}/lib64:${CUDNN_LIBRARY_PATH}:${LD_LIBRARY_PATH}
source /gapps/root/Linux_Ubuntu20.04-x86_64-gcc9.3.0/root-6.26.02/bin/thisroot.sh



# Clone and build JANA2 
# (n.b. the git clone failed to work from within ths singularity
# container on my internal system so I had to run that outside
# of the container.)
cd ${WORKDIR}
git clone https://github.com/JeffersonLab/JANA2
mkdir JANA2/build
cd JANA2/build
cmake -DCMAKE_INSTALL_PREFIX=../install \
	-DUSE_PYTHON=ON \
	-DUSE_ROOT=ON \
	-DUSE_ZEROMQ=ON \
	-DCMAKE_BUILD_TYPE=Debug \
	../
make -j48 install
source ../install/bin/jana-this.sh


# Create and build a JANA2 plugin
cd ${WORKDIR}
jana-generate.py Plugin JANAGPUTest 1 0 1
mkdir JANAGPUTest/build
cd  JANAGPUTest/build
cmake -DCMAKE_BUILD_TYPE=Debug ../
make -j48 install


# Test plugin works without GPU or libtorch before continuing
jana -PPLUGINS=JTestRoot,JANAGPUTest


# Add factory which uses GPU
cd ${WORKDIR}/JANAGPUTest
jana-generate.py JFactory GPUPID

< edit JANAGPUTest.cc to add factory generator (see comments at top of JFactory_GPUPID.cc)  >


# Add CUDA/libtorch to plugin's CMakeLists.txt
cd ${WORKDIR}/JANAGPUTest

< edit CMakeLists.txt and add following in appropriate places:

  # At top right under project(...) line
  enable_language(CUDA)


  # Right after the line with "find_package(ROOT REQUIRED)"
  find_package(Torch REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
  
  # This is just a modification of the existing target_link_libraries line
  target_link_libraries(JANAGPUTest_plugin ${JANA_LIBRARY} ${ROOT_LIBRARIES} ${TORCH_LIBRARIES})

>
  
  
# Add code that uses libtorch  

< edit JFactory_GPUPID.cc to include the folllwing:

  // Place this at top of file
  #include <torch/torch.h>

  // Place these in the Init() method
  torch::Tensor tensor = torch::rand({2, 3});
  std::cout << tensor << std::endl;
>

# A CUDA kernel can also be added and called without using libtorch.
# Create a file called tmp.cu with the following content:

   #include <stdio.h>

   __global__ void cuda_hello(){
       printf("Hello World from GPU!\n");
   }

   int cuda_hello_world() {
       printf("%s:%d\n", __FILE__,__LINE__);
       cuda_hello<<<2,3>>>(); 
       printf("%s:%d\n", __FILE__,__LINE__);
       return 0;
   }

# Add a call in JFactory_GPUPID::Init() to

   cuda_hello_world();
   


# Rebuild the plugin
cd ${WORKDIR}/JANAGPUTest/build
rm CMakeCache.txt
cmake \
    -DCUDA_TOOLKIT_ROOT_DIR=${CUDA_TOOLKIT_ROOT_DIR} \
    -DCUDNN_LIBRARY_PATH=${CUDNN_LIBRARY_PATH} \
    -DCUDNN_INCLUDE_PATH=${CUDNN_INCLUDE_PATH} \
    -DCMAKE_BUILD_TYPE=Debug \
    ../
make -j48 install


# Test the plugin. You should see a message with values from the libtorch
# tensor followed by 6 Hello World messages from the CUDA kernel.
jana -PPLUGINS=JTestRoot,JANAGPUTest -PAUTOACTIVATE=GPUPID
```

Note: You can confirm that this is using the GPU by checking the
output of "nvidia-smi" while running. The jana program should be
listed at the bottom of the output.





