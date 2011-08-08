#ifdef _GPU

    #define _MMVOff(M,N,E,L)         (_g_cMeta[_mvOff                 + M] + (E) * L)
    #define _GMVOff(M,N,E,L)         (_g_cMeta[_p.m_gmvOff            + M] + (E) * L)
    #define _LMVOff(M,N,E,L)         (_g_cMeta[_p.m_mvOff             + M] + (E) * L)
    #define _ZGMVOff(Z,M,N,E,L)      (_g_cMeta[_LTValue(Z,_LT_GMVOFF) + M] + (E) * L)
    #define _ZLMVOff(Z,M,N,E,L)      (_g_cMeta[_LTValue(Z,_LT_MVOFF)  + M] + (E) * L)

    #define _ERROR(...)              do {} while(false)
    #define _PRINT(...)              do {} while(false)

    #define _Int32ArrayElement(F,E)  _int32Arrays [(F + (E)) * _blockSize + _tid]
    #define _FloatArrayElement(F,E)  _floatArrays [(F + (E)) * _blockSize + _tid]
    #define _DoubleArrayElement(F,E) _doubleArrays[(F + (E)) * _blockSize + _tid]

    #define _SELECT_SYN(E) \
        const unsigned int _si = (E); \
        const ushort4      _sm = _p.m_smPtr[(_si * _p.m_xCount + _x) * _p.m_ySize + _y];

#else

    #define _MMVOff(M,N,E,L)         _MVOff(_g_cMeta + _mvOff                 + M, #N, E, L, _z, _y, _x)
    #define _GMVOff(M,N,E,L)         _MVOff(_g_cMeta + _p.m_gmvOff            + M, #N, E, L, _z, _y, _x)
    #define _LMVOff(M,N,E,L)         _MVOff(_g_cMeta + _p.m_mvOff             + M, #N, E, L, _z, _y, _x)
    #define _ZGMVOff(Z,M,N,E,L)      _MVOff(_g_cMeta + _LTValue(Z,_LT_GMVOFF) + M, #N, E, L, _z, _y, _x)
    #define _ZLMVOff(Z,M,N,E,L)      _MVOff(_g_cMeta + _LTValue(Z,_LT_MVOFF)  + M, #N, E, L, _z, _y, _x)

    #define _ERROR(...)              _NeuronExit(_z, _y, _x, __VA_ARGS__)
    #define _PRINT(...)              _NeuronInfo(_z, _y, _x, __VA_ARGS__)

    #define _Int32ArrayElement(F,E)  _int32Arrays [F + (E)]
    #define _FloatArrayElement(F,E)  _floatArrays [F + (E)]
    #define _DoubleArrayElement(F,E) _doubleArrays[F + (E)]

    #define _SELECT_SYN(E) \
        const unsigned int _si = _CheckSyn(_sc, E, _z, _y, _x); \
        const ushort4      _sm = _p.m_smPtr[(_si * _p.m_xCount + _x) * _p.m_ySize + _y];

    inline unsigned int _CheckSyn(unsigned int count, unsigned int e, unsigned int z, unsigned int y, unsigned int x) {
        if (e >= count) {
            _NeuronExit(z, y, x, "requested synapse number (%u) exceeds number of synapses (%u)", e + 1, count);
        }
        return e;
    }

    inline unsigned int _MVOff(const unsigned short *meta, const char *n, unsigned int e, unsigned int len,
        unsigned int z, unsigned int y, unsigned int x) {
        if (e >= meta[1]) {
            _NeuronExit(z, y, x, "requested value number (%u) exceeds number of values (%u) for field '%s'",
                e + 1, meta[1], n);
        }
        return meta[0] + e * len;
    }

    inline int   min  (int   a, int   b) { return (a <= b) ? a : b; }
    inline int   max  (int   a, int   b) { return (a >= b) ? a : b; }
    inline float fminf(float a, float b) { return (a <= b) ? a : b; }
    inline float fmaxf(float a, float b) { return (a >= b) ? a : b; }

#endif

#define _THIS_Z                    ((int)_z)

#define _NUM_SYN                   ((int)_sc)
#define _SYN_Z                     ((int)_sm.z)
#define _SYN_TYPE                  ((int)_sm.w)

#define _LTPtr(Z,A)                (_g_cMeta + Z * _LT_LEN + A)
#define _LTValue(Z,A)              _g_cMeta[Z * _LT_LEN + A]

#define _MMVCount(M)               ((int)_g_cMeta[_mvOff                 + M + 1])
#define _GMVCount(M)               ((int)_g_cMeta[_p.m_gmvOff            + M + 1])
#define _LMVCount(M)               ((int)_g_cMeta[_p.m_mvOff             + M + 1])
#define _ZGMVCount(Z,M)            ((int)_g_cMeta[_LTValue(Z,_LT_GMVOFF) + M + 1])
#define _ZLMVCount(Z,M)            ((int)_g_cMeta[_LTValue(Z,_LT_MVOFF)  + M + 1])

#define _GetLayerSz(S,D)           _LTrans##S##_s(_p.m_entry + _LT_SIZ2P, D)
#define _GetZLayerSz(Z,S,D)        _LTrans##S##_s(_LTPtr(Z,_LT_SIZ2P)   , D)

#define _GetCoord(S,D)             _LTrans##S##_c##D(_p.m_entry + _LT_SIZ2P, _y, _x)
#define _GetPCoord(S,D)            _LTrans##S##_c##D(_LTPtr(_sm.z,_LT_SIZ2P), _sm.y, _sm.x)

#define _TPtr(F)                   (_g_cMeta + _p.m_tOff            + F * 2)
#define _ZTPtr(Z,F)                (_g_cMeta + _LTValue(Z,_LT_TOFF) + F * 2)

#define _GetCConst(B,F)            _CTrans##B##_ln(_TPtr(F), _y, _x)
#define _GetCVar(B,F)              _VTrans##B##_ln(_TPtr(F), _y, _x)
#define _GetPCConst(B,F)           _CTrans##B##_ln(_ZTPtr(_sm.z,F), _sm.y, _sm.x)
#define _GetPCVar(B,F)             _VTrans##B##_ln(_ZTPtr(_sm.z,F), _sm.y, _sm.x)
#define _GetZCConst(Z,B,F,...)     _CTrans##B##_lk(_ZTPtr(Z,F), _LTPtr(Z,_LT_SIZ2P), __VA_ARGS__)
#define _GetZCVar(Z,B,F,...)       _VTrans##B##_lk(_ZTPtr(Z,F), _LTPtr(Z,_LT_SIZ2P), __VA_ARGS__)
#define _GetZCConst1(Z,B,F,...)    _CTrans##B##_gc(_ZTPtr(Z,F), _LTPtr(Z,_LT_SIZ2P), __VA_ARGS__)
#define _GetZCVar1(Z,B,F,...)      _VTrans##B##_gc(_ZTPtr(Z,F), _LTPtr(Z,_LT_SIZ2P), __VA_ARGS__)
#define _SetCVar(B,R,F,V)          _VTrans##B##_wn(_tOut.m_ptr[R], _tOut.m_h[R], _TPtr(F), _y, _x, V)

#define _DefCConstH(B)             _CTrans##B##_h
#define _DefCVarH(B)               _VTrans##B##_h
#define _GetZCConstH(Z,B,F)        _CTrans##B##_mh(_ZTPtr(Z,F), _LTPtr(Z,_LT_SIZ2P))
#define _GetZCVarH(Z,B,F)          _VTrans##B##_mh(_ZTPtr(Z,F), _LTPtr(Z,_LT_SIZ2P))
#define _GetHCConstSz(H,B,D)       _CTrans##B##_sh(H, D)
#define _GetHCVarSz(H,B,D)         _VTrans##B##_sh(H, D)
#define _GetHCConst(H,B,...)       _CTrans##B##_lkh(H, __VA_ARGS__)
#define _GetHCVar(H,B,...)         _VTrans##B##_lkh(H, __VA_ARGS__)
#define _GetHCConst1(H,B,...)      _CTrans##B##_gch(H, __VA_ARGS__)
#define _GetHCVar1(H,B,...)        _VTrans##B##_gch(H, __VA_ARGS__)

#define _GetCConst2(B,Y,X)         _CTrans##B##_lc(Y, X)
#define _GetCVar2(B,Y,X)           _VTrans##B##_lc(Y, X)

#define _DefArrayH(R)              _ATrans##R##_h
#define _DefTexH(R)                _TTrans##R##_h
#define _GetHArray(R,H,...)        _ATrans##R##_lkh(_dData, H, __VA_ARGS__)
#define _GetHTex(R,H,...)          _TTrans##R##_lkh(        H, __VA_ARGS__)
#define _GetHArray1(R,H,...)       _ATrans##R##_gch(H, __VA_ARGS__)
#define _GetHTex1(R,H,...)         _TTrans##R##_gch(H, __VA_ARGS__)
#define _GetHArraySz(R,H,D)        _ATrans##R##_sh(H, D)
#define _GetHTexSz(R,H,D)          _TTrans##R##_sh(H, D)

#define _GetMConst(F)              _g_cData[_cOff + F       ]
#define _GetMConstMV(M,N,E)        _g_cData[_MMVOff(M,N,E,1)]
#define _GetMArrayH(R,M,N,E)       _ATrans##R##_mh(_g_cMeta + _MMVOff(M,N,E,_ALEN_##R))
#define _GetMTexH(R,M,N,E)         _TTrans##R##_mh(_g_cMeta + _MMVOff(M,N,E,_TLEN_##R))
#define _GetMArray(R,M,N,E,...)    _ATrans##R##_lk(_dData, _g_cMeta + _MMVOff(M,N,E,_ALEN_##R), __VA_ARGS__)
#define _GetMTex(R,M,N,E,...)      _TTrans##R##_lk(        _g_cMeta + _MMVOff(M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetMTex1(R,M,N,E,...)     _TTrans##R##_gc(_g_cMeta + _MMVOff(M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetMArraySz(R,M,N,E,D)    _ATrans##R##_s(_g_cMeta + _MMVOff(M,N,E,_ALEN_##R), D)
#define _GetMTexSz(R,M,N,E,D)      _TTrans##R##_s(_g_cMeta + _MMVOff(M,N,E,_TLEN_##R), D)

#define _GetGConst(F)              _g_cData[_p.m_gcOff + F  ]
#define _GetGConstMV(M,N,E)        _g_cData[_GMVOff(M,N,E,1)]
#define _GetGArrayH(R,M,N,E)       _ATrans##R##_mh(_g_cMeta + _GMVOff(M,N,E,_ALEN_##R))
#define _GetGTexH(R,M,N,E)         _TTrans##R##_mh(_g_cMeta + _GMVOff(M,N,E,_TLEN_##R))
#define _GetGArray(R,M,N,E,...)    _ATrans##R##_lk(_dData, _g_cMeta + _GMVOff(M,N,E,_ALEN_##R), __VA_ARGS__)
#define _GetGTex(R,M,N,E,...)      _TTrans##R##_lk(        _g_cMeta + _GMVOff(M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetGTex1(R,M,N,E,...)     _TTrans##R##_gc(_g_cMeta + _GMVOff(M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetGArraySz(R,M,N,E,D)    _ATrans##R##_s(_g_cMeta + _GMVOff(M,N,E,_ALEN_##R), D)
#define _GetGTexSz(R,M,N,E,D)      _TTrans##R##_s(_g_cMeta + _GMVOff(M,N,E,_TLEN_##R), D)

#define _GetLConst(F)              _g_cData[_p.m_cOff + F   ]
#define _GetLConstMV(M,N,E)        _g_cData[_LMVOff(M,N,E,1)]
#define _GetLArrayH(R,M,N,E)       _ATrans##R##_mh(_g_cMeta + _LMVOff(M,N,E,_ALEN_##R))
#define _GetLTexH(R,M,N,E)         _TTrans##R##_mh(_g_cMeta + _LMVOff(M,N,E,_TLEN_##R))
#define _GetLArray(R,M,N,E,...)    _ATrans##R##_lk(_dData, _g_cMeta + _LMVOff(M,N,E,_ALEN_##R), __VA_ARGS__)
#define _GetLTex(R,M,N,E,...)      _TTrans##R##_lk(        _g_cMeta + _LMVOff(M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetLTex1(R,M,N,E,...)     _TTrans##R##_gc(_g_cMeta + _LMVOff(M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetLArraySz(R,M,N,E,D)    _ATrans##R##_s(_g_cMeta + _LMVOff(M,N,E,_ALEN_##R), D)
#define _GetLTexSz(R,M,N,E,D)      _TTrans##R##_s(_g_cMeta + _LMVOff(M,N,E,_TLEN_##R), D)

#define _GetZGConst(Z,F)           _g_cData[_LTValue(Z,_LT_GCOFF) + F]
#define _GetZGConstMV(Z,M,N,E)     _g_cData[_ZGMVOff(Z,M,N,E,1)      ]
#define _GetZGArrayH(Z,R,M,N,E)    _ATrans##R##_mh(_g_cMeta + _ZGMVOff(Z,M,N,E,_ALEN_##R))
#define _GetZGTexH(Z,R,M,N,E)      _TTrans##R##_mh(_g_cMeta + _ZGMVOff(Z,M,N,E,_TLEN_##R))
#define _GetZGArray(Z,R,M,N,E,...) _ATrans##R##_lk(_dData, _g_cMeta + _ZGMVOff(Z,M,N,E,_ALEN_##R), __VA_ARGS__)
#define _GetZGTex(Z,R,M,N,E,...)   _TTrans##R##_lk(        _g_cMeta + _ZGMVOff(Z,M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetZGTex1(Z,R,M,N,E,...)  _TTrans##R##_gc(_g_cMeta + _ZGMVOff(Z,M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetZGArraySz(Z,R,M,N,E,D) _ATrans##R##_s(_g_cMeta + _ZGMVOff(Z,M,N,E,_ALEN_##R), D)
#define _GetZGTexSz(Z,R,M,N,E,D)   _TTrans##R##_s(_g_cMeta + _ZGMVOff(Z,M,N,E,_TLEN_##R), D)

#define _GetZLConst(Z,F)           _g_cData[_LTValue(Z,_LT_COFF) + F]
#define _GetZLConstMV(Z,M,N,E)     _g_cData[_ZLMVOff(Z,M,N,E,1)     ]
#define _GetZLArrayH(Z,R,M,N,E)    _ATrans##R##_mh(_g_cMeta + _ZLMVOff(Z,M,N,E,_ALEN_##R))
#define _GetZLTexH(Z,R,M,N,E)      _TTrans##R##_mh(_g_cMeta + _ZLMVOff(Z,M,N,E,_TLEN_##R))
#define _GetZLArray(Z,R,M,N,E,...) _ATrans##R##_lk(_dData, _g_cMeta + _ZLMVOff(Z,M,N,E,_ALEN_##R), __VA_ARGS__)
#define _GetZLTex(Z,R,M,N,E,...)   _TTrans##R##_lk(        _g_cMeta + _ZLMVOff(Z,M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetZLTex1(Z,R,M,N,E,...)  _TTrans##R##_gc(_g_cMeta + _ZLMVOff(Z,M,N,E,_TLEN_##R), __VA_ARGS__)
#define _GetZLArraySz(Z,R,M,N,E,D) _ATrans##R##_s(_g_cMeta + _ZLMVOff(Z,M,N,E,_ALEN_##R), D)
#define _GetZLTexSz(Z,R,M,N,E,D)   _TTrans##R##_s(_g_cMeta + _ZLMVOff(Z,M,N,E,_TLEN_##R), D)

#define _GetArray2(R,H,Y,X)        _ATrans##R##_lc(_dData, H, Y, X)
#define _GetTex2(R,Y,X)            _TTrans##R##_lc(           Y, X)

#define _GetNField(F)              _p.m_ndPtr[(F                * _p.m_xCount + _x) * _p.m_ySize + _y]
#define _SetNField(F,V)           (_p.m_ndPtr[(F                * _p.m_xCount + _x) * _p.m_ySize + _y] = (float)(V))
#define _GetNFieldMV(M,N,E)        _p.m_ndPtr[(_LMVOff(M,N,E,1) * _p.m_xCount + _x) * _p.m_ySize + _y]
#define _SetNFieldMV(M,N,E,V)     (_p.m_ndPtr[(_LMVOff(M,N,E,1) * _p.m_xCount + _x) * _p.m_ySize + _y] = (float)(V))

#define _GetSField(F)              _p.m_sdPtr[((F                * _p.m_sSize + _si) * _p.m_xCount + _x) * _p.m_ySize + _y]
#define _SetSField(F,V)           (_p.m_sdPtr[((F                * _p.m_sSize + _si) * _p.m_xCount + _x) * _p.m_ySize + _y] = (float)(V))
#define _GetSFieldMV(M,N,E)        _p.m_sdPtr[((_LMVOff(M,N,E,1) * _p.m_sSize + _si) * _p.m_xCount + _x) * _p.m_ySize + _y]
#define _SetSFieldMV(M,N,E,V)     (_p.m_sdPtr[((_LMVOff(M,N,E,1) * _p.m_sSize + _si) * _p.m_xCount + _x) * _p.m_ySize + _y] = (float)(V))

#define _STEP_NO                   ((int)_stepNo)

/**********************************************************************************************************************/

#define _GetCenter(S,D,F)  (_GetLConst(F)    + (float)_GetCoord(S,D) * _GetLConst(F+1)   )
#define _GetZCenter(Z,F,C) (_GetZLConst(Z,F) + (float)(C)            * _GetZLConst(Z,F+1))

#define _GetRFDistAt(Z,S,D,F,P,R,...) \
    _RFDist(_GetZLayerSz(Z,S,D), _GetZLConst(Z,F), _GetZLConst(Z,F+1), P, R, __VA_ARGS__)

#define _GetRFNearAt(Z,S,D,F,P,N,...) \
    _RFNear(_GetZLayerSz(Z,S,D), _GetZLConst(Z,F), _GetZLConst(Z,F+1), P, N, __VA_ARGS__)

#define _GetRFDist(Z,S1,D1,F1,S2,D2,F2,R,...) _GetRFDistAt(Z, S2, D2, F2, _GetCenter(S1,D1,F1), R, __VA_ARGS__)
#define _GetRFNear(Z,S1,D1,F1,S2,D2,F2,N,...) _GetRFNearAt(Z, S2, D2, F2, _GetCenter(S1,D1,F1), N, __VA_ARGS__)

/*--------------------------------------------------------------------------------------------------------------------*/

_INLINE bool _RFDist(int t, float s, float d, float c, float r, int &v1, int &v2, int &c1, int &c2) {

    float dd = 1.0f / d;

    c1 = (int)ceilf ((c - r - s) * dd - 0.001f);
    c2 = (int)floorf((c + r - s) * dd + 0.001f);

    v1 = min(max(c1,  0), t    );
    v2 = min(max(c2, -1), t - 1);

    return (v1 <= v2);

}

/*--------------------------------------------------------------------------------------------------------------------*/

_INLINE bool _RFDist(int t, float s, float d, float c, float r, int &v1, int &v2) {

    int c1, c2;
    _RFDist(t, s, d, c, r, v1, v2, c1, c2);

    return (v1 == c1) && (v2 == c2);

}

/*--------------------------------------------------------------------------------------------------------------------*/

_INLINE bool _RFNear(int t, float s, float d, float c, int n, int &v1, int &v2, int &c1, int &c2) {

    float dd = 1.0f / d;

    c1 = (int)ceilf((c - s) * dd - 0.5f * (float)n - 0.001f);
    c2 = c1 + n - 1;

    v1 = min(max(c1,  0), t    );
    v2 = min(max(c2, -1), t - 1);

    return (v1 <= v2);

}

/*--------------------------------------------------------------------------------------------------------------------*/

_INLINE bool _RFNear(int t, float s, float d, float c, int n, int &v1, int &v2) {

    int c1, c2;
    _RFNear(t, s, d, c, n, v1, v2, c1, c2);

    return (v1 == c1) && (v2 == c2);

}
