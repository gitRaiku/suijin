#include "izanagi.h"

FILE *__restrict of;

#define VECTOR_PUSH(name, btype, stype)               \
void name ##vp(btype v, stype val) {                  \
  if (v->l == v->s) {                                 \
    v->s *= 2;                                        \
    v->v = realloc(v->v, sizeof(stype) * v->s);      \
  }                                                   \
  v->v[v->l] = val;                                   \
  ++v->l;                                             \
}                                                     

#define VECTOR_INIT(name, btype, stype)       \
void name ##vi(btype v) {                     \
  v->l = 0;                                   \
  v->s = 4;                                   \
  v->v = malloc(sizeof(v4) * v->s);          \
}                                             

#define VECTOR_TRIM(name, btype, stype)          \
void name ##vt(btype v) {                        \
  if (v->s != v->l) {                            \
    v->s = v->l;                                 \
    v->v = realloc(v->v, sizeof(stype) * v->s); \
  }                                              \
}

#define VECTOR_SUITE(name, btype, stype) \
VECTOR_PUSH(name, btype, stype) \
VECTOR_INIT(name, btype, stype) \
VECTOR_TRIM(name, btype, stype) \

VECTOR_SUITE(v4, struct v4v *__restrict, v4)
VECTOR_SUITE(v3, struct v3v *__restrict, v3)
VECTOR_SUITE(v2, struct v2v *__restrict, v2)
VECTOR_SUITE(mat, struct matv *__restrict, struct material)


void facevp(struct facev *__restrict v, faceev val) {
  if (v->l == v->s) {
    v->s *= 2;
    v->v = realloc(v->v, sizeof(faceev) * v->s);
  }
  // v->v[v->l] = val;
  memcpy(v->v[v->l], val, sizeof(faceev));
  ++v->l;
}

void facevi(struct facev *__restrict v) {
  v->l = 0;
  v->s = 4;
  v->v = malloc(sizeof(faceev) * v->s);
}

void facevt(struct facev *__restrict v) {
  if (v->s != v->l) {
    v->s = v->l;
    v->v = realloc(v->v, sizeof(faceev) * v->s);
  }
}

char file_buf[2048];
uint32_t bufp;
uint32_t bufl;
int32_t cfd;
uint8_t flags; 
#define LINEND_MASK   0b010 /// Read a line end
#define UNREADABLE_MASK   0b100 /// There are still bytes left in the file

char _getc() {
  if (flags & UNREADABLE_MASK) {
    return '\0';
  }
  if (bufp < bufl) {
    if (file_buf[bufp] == '\n') {
      flags |= LINEND_MASK;
    }
    return file_buf[bufp++];
  }
  bufl = read(cfd, file_buf, sizeof(file_buf));
  if (bufl == -1) {
    fprintf(stderr, "Error reading from fd %u: %m", cfd);
    exit(1);
  }
  if (bufl == 0) {
    flags |= UNREADABLE_MASK;
    return '\0';
  }
  bufp = 1;
  return file_buf[0];
}

float read_float() {
  char ch;
  float sgn = 1.0f;
  float res = 0.0f;
  float adm = 0.1f;
  uint8_t afterDot = 0;
  ch = _getc();
  if (ch == '-') {
    sgn = -1.0f;
  } else if (ch == '\\') {
    return 0.0f;
  } else {
    res = ch - '0';
  }
  while ((ch = _getc()) && (('0' <= ch && ch <= '9') || (ch == '.'))) {
    if (afterDot == 1) {
      res += (ch - '0') * adm;
      adm /= 10;
    } else if (ch == '.') {
      afterDot = 1;
    } else {
      res *= 10;
      res += ch - '0';
    }
  }
  if (ch == '\n') {
    flags |= LINEND_MASK;
  }
  if (ch == '\0') {
    flags |= UNREADABLE_MASK;
  }
  return res * sgn;
}

void next_token(char *__restrict tok) {
  --tok;
  do {
    ++tok;
    *tok = _getc();
  } while (*tok != ' ' && *tok != '\r' && *tok != '\n' && *tok != '\0');
  if (*tok == '\r') {
    _getc();
  }
  if (*tok == '\0') {
    flags |= UNREADABLE_MASK;
  }
  *tok = '\0';
}

void init_object(struct object *__restrict o) {
  v4vi(&o->v);
  v3vi(&o->n);
  v2vi(&o->t);
  facevi(&o->f);
  matvi(&o->m);
  o->smooth_shading = 0;
}

union OBJ_MEMBERS {
  vec4 v4;
  vec3 v3;
  vec2 v2;
  faceev fv; 
  struct material mat;
};

enum IDS { VERTEX, TEXTURE_COORDS, VERTEX_NORMAL, FACE, MATERIAL_LIB, USE_MATERIAL, OBJECT, GROUP, SMOOTH_SHADING, COMMENT, UNDEF };

uint32_t id_tok(char tok[64]) {
  switch (tok[0]) {
    case 'v':
      switch(tok[1]) {
        case '\0':
          return VERTEX;
        case 't':
          return TEXTURE_COORDS;
        case 'n':
          return VERTEX_NORMAL;
        default:
          return UNDEF;
      }
    case 'f':
      return FACE;
    case 'm':
      return MATERIAL_LIB;
    case 'u':
      return USE_MATERIAL;
    case 'o':
      return OBJECT;
    case 'g':
      return GROUP;
    case 's':
      return SMOOTH_SHADING;
    case '#':
      return COMMENT;
    default:
      return UNDEF;
  }
}

void id_to_str(enum IDS id) {
  char *VE = "VERTEX";
  char *TE = "TEXTURE_COORDS";
  char *VN = "VERTEX_NORMAL";
  char *FA = "FACE";
  char *ML = "MATERIAL_LIB";
  char *UM = "USE_MATERIAL";
  char *OB = "OBJECT";
  char *GR = "GROUP";
  char *SM = "SMOOTH_SHADING";
  char *CO = "COMMENT";
  char *UN = "UNDEF";
  char *s = UN;
  switch (id) {
    case VERTEX:
      s = VE;
      break;
    case TEXTURE_COORDS:
      s = TE;
      break;
    case VERTEX_NORMAL:
      s = VN;
      break;
    case FACE:
      s = FA;
      break;
    case MATERIAL_LIB:
      s = ML;
      break;
    case USE_MATERIAL:
      s = UM;
      break;
    case OBJECT:
      s = OB;
      break;
    case GROUP:
      s = GR;
      break;
    case SMOOTH_SHADING:
      s = SM;
      break;
    case COMMENT:
      s = CO;
      break;
    case UNDEF:
      s = UN;
      break;
  }
  fprintf(stdout, "Got id %u [%s]\n", id, s);
}

struct object *__restrict parse_object_file(char *__restrict fname, uint32_t *__restrict objl) {
  struct object *__restrict objs = NULL;
  
  bufp = bufl = 0;
  cfd = open(fname, O_RDONLY);
  if (cfd == -1) {
    fprintf(stderr, "Could not open '%s': %m\n", fname);
    exit(1);
  }

  *objl = 0;

  char tok[128] = {0};
  enum IDS id;
  union OBJ_MEMBERS om;
  uint32_t cobj = -1;
  uint32_t line_number = 0;
  while ((flags & UNREADABLE_MASK) == 0) {
    ++line_number;
    flags &= ~LINEND_MASK;
    next_token(tok);
    if (flags & UNREADABLE_MASK) {
      break;
    }
    id = id_tok(tok);
    //id_to_str(id);
    switch (id) {
      case VERTEX:
        om.v4.x = read_float();
        om.v4.y = read_float();
        om.v4.z = read_float();
        if ((flags & LINEND_MASK) == 0) {
          om.v4.w = read_float();
        } else {
          om.v4.w = 1.0f;
        }
        v4vp(&objs[cobj].v, om.v4);
        break;
      case TEXTURE_COORDS:
        om.v2.x = read_float();
        om.v2.y = read_float();
        v2vp(&objs[cobj].t, om.v2);
        break;
      case VERTEX_NORMAL:
        om.v3.x = read_float();
        om.v3.y = read_float();
        om.v3.z = read_float();
        v3vp(&objs[cobj].n, om.v3);
        break;
      case FACE:
        {
          int32_t i;
          for(i = 0; i < 4; ++i) {
            om.fv[i].v = (uint32_t)read_float();
            om.fv[i].t = (uint32_t)read_float();
            om.fv[i].n = (uint32_t)read_float();
          }
        }
        facevp(&objs[cobj].f, om.fv);
        break;
      case MATERIAL_LIB:
        next_token(tok);
        fprintf(stdout, "Material lib: %s\n", tok);
        /// READ MATERIAL
        break;
      case USE_MATERIAL:
        next_token(tok);
        /// USE MATERIAL
        break;
      case OBJECT:
        if (cobj != -1) {
          v4vt(&objs[cobj].v);
          v3vt(&objs[cobj].n);
          v2vt(&objs[cobj].t);
          facevt(&objs[cobj].f);
          matvt(&objs[cobj].m);
        }
        objs = realloc(objs, sizeof(struct object) * (*objl + 1));
        init_object(objs + *objl);
        cobj = *objl;
        ++*objl;
        next_token(tok);
        strncpy(objs[cobj].name, tok, 64);
        break;
      case GROUP:
        /// GROUP
        break;
      case SMOOTH_SHADING:
        next_token(tok);
        objs[cobj].smooth_shading = tok[0] == 'o' ? 0 : 1;
        break;
      case COMMENT:
        while ((flags & LINEND_MASK) == 0) {
          next_token(tok);
        }
        break;
      default:
        fprintf(stderr, "Undefined at line %u\n", line_number);
        break;
    }
  }
  v4vt(&objs[cobj].v);
  v3vt(&objs[cobj].n);
  v2vt(&objs[cobj].t);
  facevt(&objs[cobj].f);
  matvt(&objs[cobj].m);

  return objs;
}
