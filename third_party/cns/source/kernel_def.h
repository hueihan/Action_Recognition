#ifdef _GPU

    __global__ void _USER_KERNEL_NAME(

        unsigned int      _mvOff,
        unsigned int      _cOff,
        const float      *_dData,
        _OutTable         _tOut,
        unsigned int      _stepNo,

        unsigned int      _bCount,
        const ushort4    *_bPtr,
        const _LayerData *_dLayers

    ) {

        const unsigned int _bid = blockIdx.y * gridDim.x + blockIdx.x;
        if (_bid >= _bCount) return;

        const unsigned int _tid = threadIdx.y * blockDim.x + threadIdx.x;

        __shared__ unsigned int _xStart;
        __shared__ unsigned int _yStart;
        __shared__ unsigned int _z;
        __shared__ unsigned int _tyCount;
        if (_tid == 0) {
            const ushort4 _bd = _bPtr[_bid];
            _xStart  = _bd.x;
            _yStart  = _bd.y;
            _z       = _bd.z;
            _tyCount = _bd.w;
        }
        __syncthreads();

        __shared__ _LayerData _p;
        if (_tid < _LAYERDATA_UINTS) {
            _p.m_array[_tid] = _dLayers[_z].m_array[_tid];
        }
        __syncthreads();

        const unsigned int _tx = _tid / _p.m_blockYSize;
        const unsigned int _ty = _tid - _tx * _p.m_blockYSize;
        const unsigned int _x = _xStart + _tx;
        const unsigned int _y = _yStart + _ty;
        if ((_x >= _p.m_xCount) || (_ty >= _tyCount)) return;

        #ifdef _USES_SYNS
            const unsigned int _sc = _p.m_nmPtr[_x * _p.m_ySize + _y];
        #endif

        #if defined(_USES_INT32_ARRAYS) || defined(_USES_FLOAT_ARRAYS) || defined(_USES_DOUBLE_ARRAYS)
            const unsigned int _blockSize = blockDim.x * blockDim.y;
        #endif
        #if defined(_USES_INT32_ARRAYS)
            extern __shared__ int _int32Arrays[];
        #endif
        #if defined(_USES_FLOAT_ARRAYS)
            extern __shared__ float _floatArrays[];
        #endif
        #if defined(_USES_DOUBLE_ARRAYS)
            extern __shared__ double _doubleArrays[];
        #endif

        #include _USER_KERNEL_DEF

    }

#else

    void _USER_KERNEL_NAME(

        unsigned int  _mvOff,
        unsigned int  _cOff,
        const float  *_dData,
        _OutTable     _tOut,
        unsigned int  _stepNo,

        unsigned int  _z,
        _LayerData    _p

    ) {

        unsigned int _ySkip = _p.m_ySize0 - _p.m_yCount0;

        for (unsigned int _x = 0; _x < _p.m_xCount; _x++) {
            for (unsigned int _y = 0, _yCheck = _p.m_yCount0; _y < _p.m_yCount; ) {

                #ifdef _USES_SYNS
                    const unsigned int _sc = _p.m_nmPtr[_x * _p.m_ySize + _y];
                #endif

                #if defined(_USES_INT32_ARRAYS)
                    int _int32Arrays[_INT32_ARRAY_SIZE];
                #endif
                #if defined(_USES_FLOAT_ARRAYS)
                    float _floatArrays[_FLOAT_ARRAY_SIZE];
                #endif
                #if defined(_USES_DOUBLE_ARRAYS)
                    double _doubleArrays[_DOUBLE_ARRAY_SIZE];
                #endif

                #include _USER_KERNEL_DEF

                if (_y++ == _yCheck) {
                    _y      += _ySkip;
                    _yCheck += _p.m_ySize0;
                }

            }
        }

    }

#endif
