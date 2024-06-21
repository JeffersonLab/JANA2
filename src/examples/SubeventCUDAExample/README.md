# Run on JLab ifarm GPUs

This is an example that can run on a single GPU.

1. On the `ifarm` node, ask for a GPU compute node with 8000 MB memory.

    ```bash
    module load cuda

    srun --gres gpu:${gpu_type}:1 -p gpu --mem-per-cpu=8000 --pty bash
    ```
    ${gpu_type} can be `T4`, `TitanRTX` or `A100`.

2. On the compute node, build the whole application with cmake option "USE_CUDA=On".

   ```bash
    # bash-4.2$ rm -rf build/*
    bash-4.2$ cmake -DUSE_CUDA=On -S . -B build  # add the cmake option to config
    #### output
    #-- Found CUDA: /apps/cuda/11.4.2 (found version "11.4")
    bash-4.2$ cmake --build build -j 32 --target install  # build and install as normal
   ```
   Launch the application.
   ```bash
   bash-4.2$ ./install/bin/SubeventCUDAExample
   ```

