#include "izanagi.h"

FILE *__restrict of;

// VECTOR_SUITE(v4, struct v4v *__restrict, v4)
VECTOR_SUITE(v3, struct v3v *__restrict, v3)
VECTOR_SUITE(v2, struct v2v *__restrict, v2)
VECTOR_SUITE(float, struct floatv *__restrict, float)

void facevp(struct facev *__restrict v, struct faceev val) {
  if (v->l == v->s) {
    v->s *= 2;
    v->v = realloc(v->v, sizeof(struct faceev) * v->s);
  }
  v->v[v->l] = val;
  // memcpy(v->v[v->l], val, sizeof(struct faceev));
  ++v->l;
}

void facevi(struct facev *__restrict v) {
  v->l = 0;
  v->s = 4;
  v->v = malloc(sizeof(struct faceev) * v->s);
}

void facevt(struct facev *__restrict v) {
  if (v->s != v->l) {
    v->s = v->l;
    v->v = realloc(v->v, sizeof(struct faceev) * v->s);
  }
}

void init_object(struct object *__restrict o) {
  /*v3vi(&o->v);
  v3vi(&o->n);
  v2vi(&o->t);
  facevi(&o->f);*/
  floatvi(&o->v);
  o->smooth_shading = 0;
}

union OBJ_MEMBERS {
  //vec4 v4;
  vec3 v3;
  vec2 v2;
  struct faceev fv; 
  struct material mat;
};

enum IDS { VERTEX, TEXTURE_COORDS, VERTEX_NORMAL, FACE, MATERIAL_LIB, USE_MATERIAL, OBJECT, GROUP, SMOOTH_SHADING, COMMENT, UNDEF };

enum IDS id_tok(char tok[64]) {
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
      break;
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
    case '\n':
    case '#':
      return COMMENT;
    default:
      return UNDEF;
  }
  return UNDEF;
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

void destroy_object(struct object *__restrict obj) {
  /*free(obj->v.v);
  free(obj->t.v);
  free(obj->n.v);
  free(obj->f.v);*/
  // Free materials separately
}

void add_i_dont_even_know(v3 *__restrict v, struct floatv *__restrict fv, struct v3v *__restrict verts, struct v3v *__restrict norms, struct v2v *__restrict texts) {
  floatvp(fv, verts->v[(uint32_t)v->x].x); // TODO: Boy do i love shite code
  floatvp(fv, verts->v[(uint32_t)v->x].y);
  floatvp(fv, verts->v[(uint32_t)v->x].z);
  floatvp(fv, norms->v[(uint32_t)v->z].x);
  floatvp(fv, norms->v[(uint32_t)v->z].y);
  floatvp(fv, norms->v[(uint32_t)v->z].z);
  floatvp(fv, texts->v[(uint32_t)v->y].x);
  floatvp(fv, texts->v[(uint32_t)v->y].y);
}

struct object *__restrict parse_object_file(char *__restrict fname, uint32_t *__restrict objl, struct matv *__restrict materials) {
  struct object *__restrict objs = NULL;
  struct fbuf cb;
  struct v3v verts;
  struct v3v norms;
  struct v2v texts;
  v3vi(&verts);
  v3vi(&norms);
  v2vi(&texts);

  memset(&cb, 0, sizeof(cb));
  
  cb.bufp = cb.bufl = 0;
  cb.fd = open(fname, O_RDONLY);
  if (cb.fd == -1) {
    fprintf(stderr, "Could not open '%s': %m\n", fname);
    exit(1);
  }

  *objl = 0;

  char tok[128] = {0};
  enum IDS id;
  union OBJ_MEMBERS om;
  uint32_t cobj = -1;
  uint32_t line_number = 0;
  while ((cb.flags & UNREADABLE_MASK) == 0) {
    ++line_number;
    cb.flags &= ~LINEND_MASK;
    next_token(tok, &cb);
    if (cb.flags & UNREADABLE_MASK) {
      break;
    }
    id = id_tok(tok);
    //id_to_str(id);
    switch (id) {
      case VERTEX:
        om.v3.x = read_float(&cb);
        om.v3.y = read_float(&cb);
        om.v3.z = read_float(&cb);
        v3vp(&verts, om.v3);
        break;
      case TEXTURE_COORDS:
        om.v2.x = read_float(&cb);
        om.v2.y = read_float(&cb);
        v2vp(&texts, om.v2);
        break;
      case VERTEX_NORMAL:
        om.v3.x = read_float(&cb);
        om.v3.y = read_float(&cb);
        om.v3.z = read_float(&cb);
        v3vp(&norms, om.v3);
        break;
      case FACE:
        {
          int32_t i;
          for(i = 0; i < 3; ++i) {
            om.v3.x = read_float(&cb) - 1;
            om.v3.y = read_float(&cb) - 1;
            om.v3.z = read_float(&cb) - 1;
            add_i_dont_even_know(&om.v3, &objs[cobj].v, &verts, &norms, &texts);
          }
        }
        break;
      case MATERIAL_LIB:
        next_token(tok, &cb);
        fprintf(stdout, "Material lib: %s\n", tok);
        parse_material(materials, tok);
        break;
      case USE_MATERIAL:
        next_token(tok, &cb);
        /// USE MATERIAL
        break;
      case OBJECT:
        if (cobj != -1) {
          floatvt(&objs[cobj].v);
        }
        objs = realloc(objs, sizeof(struct object) * (*objl + 1));
        init_object(objs + *objl);
        cobj = *objl;
        ++*objl;
        next_token(tok, &cb);
        strncpy(objs[cobj].name, tok, 64);
        break;
      case GROUP:
        next_token(tok, &cb);
        /// GROUP
        break;
      case SMOOTH_SHADING:
        next_token(tok, &cb);
        objs[cobj].smooth_shading = tok[0] == 'o' ? 0 : 1;
        break;
      case COMMENT:
        while ((cb.flags & LINEND_MASK) == 0) {
          next_token(tok, &cb);
        }
        break;
      default:
        fprintf(stderr, "Undefined token at %s:%u [%s]\n", fname, line_number, tok);
        break;
    }
  }
  
  free(verts.v);
  free(norms.v);
  free(texts.v);
  floatvt(&objs[cobj].v);

  close(cb.fd);

  return objs;
}
