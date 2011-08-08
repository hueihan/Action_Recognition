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

    );

#else

    static void _USER_KERNEL_NAME(

        unsigned int  _mvOff,
        unsigned int  _cOff,
        const float  *_dData,
        _OutTable     _tOut,
        unsigned int  _stepNo,

        unsigned int  _z,
        _LayerData    _p

    );

#endif
