#ifndef __SCRIPT_ASSEMBLY_COMPILER_HPP__
#define __SCRIPT_ASSEMBLY_COMPILER_HPP__

#include "../Dictionary.hpp"
#include "../DynamicArray.hpp"
#include <string.h>

class ScriptAssemblyCompiler {
    Dictionary<size_t> vars;
    typedef struct {
        size_t offset;
        size_t lno;
        const char *label;
    } labelusage_t;
    Dictionary<size_t> labels;
    DynamicArray<labelusage_t, 128> labelusages;
    DynamicArray<unsigned char, 512> outbuf;

    long long token_int;
    char *token_str;
    size_t lno;
    
    enum Token {
        Nop,
        Return, ReturnDoNothing, ReturnFail, ReturnDestroy, ReturnPlace, ReturnKeep, ReturnUpdate, ReturnReverseUpdate,
        End, Frameset, ReadArg, Random, LoadVar, StoreVar, Exchange,
        Immediate8, Immediate16, Immediate32, Immediate8U, Immediate16U, Immediate32U,
        Immediate8B, Immediate16B, Immediate32B, Immediate8UB, Immediate16UB, Immediate32UB,
        Add, Sub, Mul, Div, Mod, And, Or, Xor, Lor, Land, Inc, Dec,
        AddF, SubF, MulF, DivF, ModF, PowF, NanF, InfF,
        Push, Pop, PushB, PopB, BA, BZ, BNZ, JSR, RTS, JSRZ, JSRNZ, RTSZ, RTSNZ,
        EQ, NEQ, GT, LT, GTEQ, LTEQ, EQF, NEQF, GTF, LTF, GTEQF, LTEQF,
        BZSet32, BNZSet32, PushArg, PushVar, Abs, AbsF, Sqrt, SqrtF, Itof, Ftoi,

        GetTileId=0x80, GetLightLevel, TileLightLevel, TileIsSolid, TileIsSpawnable, TileIsWall,
        TileFloor, TileCeiling, TileWall,

        None=0xF8, Integer, Label, LabelUsage,
    };
    public:
    ScriptAssemblyCompiler();
    char peek(const char *data, size_t datalen, size_t i);
    bool consumeToken(const char* data, size_t datalen, size_t& i, const char* tok);
    size_t compile(const char *data, size_t datalen, unsigned char **out);
    Token next(const char *data, size_t datalen, size_t &i);
};

#endif