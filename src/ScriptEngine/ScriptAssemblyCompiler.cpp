#include "ScriptAssemblyCompiler.hpp"
#include "../Registries.hpp"

static constexpr const char *opcodes[] {
    "nop", "rv", "returnDoNothing", "returnFail", "returnDestroy",
    "returnPlace", "returnKeep", "returnUpdate", "returnReverseUpdate",
    "end", "frameset", "arg", "random", "v", "sv", "ex",
    "i8", "i16", "i32", "u8", "u16", "u32",
    "i8b", "i16b", "i32b", "u8b", "u16b", "u32b",
    "add", "sub", "mul", "div", "mod", "and", "or", "xor", "lor", "land", "inc", "dec",
    "addf", "subf", "mulf", "divf", "modf", "powf", "nanf", "inff",
    "push", "pop", "pushb", "popb", "ba", "bz", "bnz", "jsr", "rts", "jsrz", "jsrnz", "rtsz", "rtsnz",
    "eq", "neq", "gt", "lt", "gteq", "lteq", "eqf", "neqf", "gtf", "ltf", "gteqf", "lteqf",
    "bzset32", "bnzset32", "pusharg", "pushvar", "abs", "absf", "sqrt", "sqrtf", "itof", "ftoi",
    "i64", "u64", "i64b", "u64b",
    nullptr,
};
static constexpr const char *opcodes80[] {
    "gettile", "getlight", "tilelight", "tileissolid", "tileisspawnable", "tileiswall",
    "tilefloor", "tileceiling", "tilewall",
    "camerax", "cameray", "cameraz", "entityx", "entityy", "entityz",
    "entitymovetowards", "entityrotate", "entityteleport", "canseeplayer",
    "getentitytimer", "setentitytimer", "randomteleportentity", "getdeltatime",
    nullptr,
};

static char *subcstr(const char *data, size_t datalen, size_t start, size_t len) {
    if (start >= datalen) {
        return nullptr;
    }
    if (start+len > datalen) {
        len = datalen - start;
    }
    char *s = new char[len+1];
    memcpy(s, &data[start], len);
    s[len] = 0;
    return s;
}

ScriptAssemblyCompiler::ScriptAssemblyCompiler() {
    lno = 1;
}

size_t ScriptAssemblyCompiler::compile(const char *data, size_t datalen, unsigned char **out) {
    Token tk;
    size_t inoffset = 0;
    do {
        tk = next(data, datalen, inoffset);
        if (tk >= Nop && tk < None) {
            outbuf.append(tk);
            switch (tk) {
                case LoadVar:
                case StoreVar:
                case Frameset:
                case ReadArg:
                case PushArg:
                case PushVar:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    break;
                case Return:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    break;
                case ReturnDoNothing:
                    break;
                case Immediate8:
                case Immediate8U:
                case Immediate8B:
                case Immediate8UB:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    break;
                case Immediate16:
                case Immediate16U:
                case Immediate16B:
                case Immediate16UB:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    outbuf.append(token_int >> 8);
                    break;
                case Immediate32:
                case Immediate32U:
                case Immediate32B:
                case Immediate32UB:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    outbuf.append(token_int >> 8);
                    outbuf.append(token_int >> 16);
                    outbuf.append(token_int >> 24);
                    break;
                case Immediate64:
                case Immediate64B:
                case Immediate64U:
                case Immediate64UB:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    outbuf.append(token_int >>  8);
                    outbuf.append(token_int >> 16);
                    outbuf.append(token_int >> 24);
                    outbuf.append(token_int >> 32);
                    outbuf.append(token_int >> 40);
                    outbuf.append(token_int >> 48);
                    outbuf.append(token_int >> 56);
                    break;
                case BA:
                case BZ:
                case BNZ:
                case JSR:
                case JSRZ:
                case JSRNZ:
                    tk = next(data, datalen, inoffset);
                    if (tk == LabelUsage) {
                        outbuf.append(0);
                        outbuf.append(0);
                    } else {
                        outbuf.append(token_int);
                        outbuf.append(token_int >> 8);
                    }
                    break;
                case BZSet32:
                case BNZSet32:
                    tk = next(data, datalen, inoffset);
                    outbuf.append(token_int);
                    outbuf.append(token_int >> 8);
                    outbuf.append(token_int >> 16);
                    outbuf.append(token_int >> 24);
                    tk = next(data, datalen, inoffset);
                    if (tk == LabelUsage) {
                        outbuf.append(0);
                        outbuf.append(0);
                    } else {
                        outbuf.append(token_int);
                        outbuf.append(token_int >> 8);
                    }
                default:
                    break;
            }
        }
    } while (inoffset < datalen);

    labels.add("eof", outbuf.length());

    for (size_t i=0; i<labelusages.length(); i++) {
        labelusage_t *lbl = &labelusages[i];
        const char *name = lbl->label;
        size_t value = -1;
        if (labels.has(name)) {
            // resolved label address
            value = labels[lbl->label];
        } else {
            // label not found / not resolved
            printf("Warning: error loading script: Unknown label name \"%s\" (line %llu)\n", name, lbl->lno);
        }
        // set resolved label address
        outbuf[lbl->offset+0] = value;
        outbuf[lbl->offset+1] = value >> 8;
    }

    outbuf.append(End);
    *out = outbuf.collapse();

    return outbuf.length();
}

char ScriptAssemblyCompiler::peek(const char *data, size_t datalen, size_t i) {
    if (i < datalen) {
        return data[i];
    }
    return 0;
}

bool ScriptAssemblyCompiler::consumeToken(const char* data, size_t datalen, size_t& i, const char* tok) {
    size_t len = strlen(tok);
    if (i + len < datalen) {
        if (!memcmp(&data[i], tok, len)) {
            i += len;
            return true;
        }
    }
    return false;
}

ScriptAssemblyCompiler::Token ScriptAssemblyCompiler::next(const char *data, size_t datalen, size_t &i) {
    char c = peek(data, datalen, i);
    Token tk = None;
    if (c == 0) {
        return tk;
    }
    // skip whitespace
    while (c > 0 && c <= ' ') {
        if (c == '\n') {
            lno++;
        }
        i++;
        c = peek(data, datalen, i);
    }
    if (c == 0) {
        return tk;
    }
    if (c == ';') {
        // comment
        i++;
        do {
            c = peek(data, datalen, i); i++;
        } while (c >= ' ');
        i--;
    } else if (c == '#') {
        // block/item ID
        i++;
        bool istexture = false;
        bool istile = false;
        bool isentity = false;
        bool isitem = false;
        if (consumeToken(data, datalen, i, "texture:")) {
            istexture = true;
        } else if (consumeToken(data, datalen, i, "tile:")) {
            istile = true;
        } else if (consumeToken(data, datalen, i, "entity:")) {
            isentity = true;
        } else {
            printf("Script Warning: Unknown content type on line %llu\n", lno);
            token_int = 0;
        }

        size_t j = i;
        do {
            c = peek(data, datalen, i); i++;
        } while (c > ' ');
        i--;
        const char* contentid = subcstr(data, datalen, j, i-j);
        if (istexture) {
            RegisteredTexture* tex = GlobalTextureRegistry->of(contentid);
            if (tex == nullptr) {
                printf("Script Warning: Unknown texture id \"%s\" on line %llu\n", contentid, lno);
                token_int = 0;
            } else {
                token_int = tex->id;
            }
        } else if (istile) {
            MapTile* tile = GlobalMapTileRegistry->of(contentid);
            if (tile == nullptr) {
                printf("Script Warning: Unknown tile id \"%s\" on line %llu\n", contentid, lno);
                token_int = 0;
            } else {
                token_int = tile->id;
            }
        } else if (isentity) {
            EntityType* ent = GlobalEntityRegistry->of(contentid);
            if (ent == nullptr) {
                printf("Script Warning: Unknown entity id \"%s\" on line %llu\n", contentid, lno);
                token_int = 0;
            } else {
                token_int = ent->id;
            }
        // } else if (isitem) {
        //     ;
        }
        tk = Integer;
    } else if (c == '$' || c == '-' || c == '.' || c >= '0' && c <= '9') {
        // number
        bool isfloat = false, neg = false, decimal = false;
        char base = 10;
        long long num = 0;
        double dec = 0;
        double place = 0.1f;
        if (c == '-') {
            i++;
            c = peek(data, datalen, i);
            neg = true;
        }
        if (c == '.') {
            i++;
            c = peek(data, datalen, i);
            isfloat = decimal = true;
        } else if (c == '$') {
            i++;
            base = 16;
        }
        do {
            c = peek(data, datalen, i); i++;
            if (c == '.') {
                isfloat = true;
                decimal = true;
                continue;
            } else if (base == 10 && c == 'f' || c == 'F') {
                isfloat = true;
                break;
            } else if (c >= '0' && c <= '9') {
                if (decimal) {
                    dec += (c - '0') * place;
                    place *= 0.1f;
                } else {
                    num = num * base + c - '0';
                }
            } else if (base > 10 && !isfloat) {
                if (c >= 'A' && c <= 'F') {
                    num = num * base + c + 10 - 'A';
                } else if (c >= 'a' && c <= 'f') {
                    num = num * base + c + 10 - 'a';
                } else {
                    break;
                }
            } else {
                break;
            }
        } while (1);
        if (neg) {
            num = -num;
        }
        if (isfloat) {
            token_float = dec + num;
        } else {
            token_int = num;
        }
        tk = Integer;
    } else if (c == '_') {
        // argument/variable
        i++;
        c = peek(data, datalen, i); i++;
        if (c == 'I' && peek(data, datalen, i) == 'D') {
            i++;
            token_int = 0;
        } if (c == 'X') {
            token_int = 1;
        } else if (c == 'Y') {
            token_int = 2;
        } else if (c == 'Z') {
            token_int = 3;
        } else {
            size_t j = i;
            do {
                c = peek(data, datalen, i); i++;
            } while (c > ' ');
            const char *name = subcstr(data, datalen, j, i-j);
            if (vars.has(name)) {
                token_int = vars[name];
            } else {
                token_int = vars.length()+1;
                vars.add(name, token_int);
            }
        }
        tk = Integer;
    } else if (c == ':') {
        // label define
        i++;
        size_t j = i;
        do {
            c = peek(data, datalen, i); i++;
        } while (c > ' ');
        i--;
        const char *name = subcstr(data, datalen, j, i-j);
        if (labels.has(name)) {
            labels[name] = outbuf.length();
        } else {
            token_int = outbuf.length();
            labels.add(name, token_int);
        }
        tk = Label;
    } else if (c == '@') {
        // label usage
        i++;
        size_t j = i;
        do {
            c = peek(data, datalen, i); i++;
        } while (c > ' ');
        i--;
        const char *name = subcstr(data, datalen, j, i-j);
        labelusages.append({outbuf.length(), lno, name});
        tk = LabelUsage;
    } else if (c >= 'a' && c <= 'z') {
        size_t j = i;
        do {
            c = peek(data, datalen, i); i++;
        } while (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '_');
        i--;
        char *name = subcstr(data, datalen, j, i-j);
        for (unsigned int i=0; name[i]>0; i++) {
            name[i] = tolower(name[i]);
        }
        token_int = -1;
        for (size_t k=0; opcodes[k]!=nullptr; k++) {
            if (!strcmp(name, opcodes[k])) {
                token_int = k;
                break;
            }
        }
        if (token_int == -1) {
            for (size_t k=0; opcodes80[k]!=nullptr; k++) {
                if (!strcmp(name, opcodes80[k])) {
                    token_int = k+0x80;
                    break;
                }
            }
        }
        if (token_int == -1) {
            printf("Warning: error loading script: Unknown opcode '%s' (line %llu)\n", name, lno);
        } else {
            tk = (Token)token_int;
        }
    } else {
        printf("Warning: error loading script: Unexpected character '%c' (line %llu)\n", c, lno);
        i++;
    }

    return tk;
}
