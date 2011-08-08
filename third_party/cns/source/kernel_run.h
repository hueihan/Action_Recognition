#ifdef _GPU

    case _USER_KERNEL_TYPENO:

        _USER_KERNEL_NAME<<<gridDim, blockDim, m_blockSize * _USER_KERNEL_ARRAYBYTES>>>(

            _g_mvOff,
            _g_cOff,
            _g_dData,
            _g_tOut,
            _g_stepNo,

            m_bCount,
            m_bPtr,
            _g_dLayers

            );

        // First, check to see if the kernel failed to launch at all.

        if ((_g_cudaErr = cudaGetLastError()) != cudaSuccess) {
            _CudaExit("unable to launch kernel (type '%s')", m_type);
        }

        // Kernel calls return immediately after launching.  If you made a bunch in a row (without synchronizing),
        // CUDA would queue them, but only one kernel at a time would actually run on the device.  We synchronize
        // after every call.  This allows us to retrieve the error code if the kernel fails after launch.

        if ((_g_cudaErr = cudaThreadSynchronize()) != cudaSuccess) {
            _CudaExit("kernel failed after launch (type '%s')", m_type);
        }

        break;

#else

    case _USER_KERNEL_TYPENO:

        _USER_KERNEL_NAME(

            _g_mvOff,
            _g_cOff,
            _g_dData,
            _g_tOut,
            _g_stepNo,

            m_z,
            *(_LayerData *)this

            );

        break;

#endif
