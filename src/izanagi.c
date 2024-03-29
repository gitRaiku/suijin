#include "izanagi.h"

VECTOR_SUITE(v3, struct v3v *__restrict, v3)
VECTOR_SUITE(v2, struct v2v *__restrict, v2)
VECTOR_SUITE(mati, struct mativ *__restrict, struct mate)
VECTOR_SUITE(float, struct floatv *__restrict, float)
VECTOR_SUITE(mat, struct matv *__restrict, struct material)
VECTOR_SUITE(mod, struct modv *__restrict, struct model)
VECTOR_SUITE(obj, struct objv *__restrict, struct object)
VECTOR_SUITE(lin, struct linv *__restrict, struct line)
VECTOR_SUITE(minf, struct minfv *__restrict, struct minf)

char _getc(struct fbuf *__restrict cb) {
  if (cb->flags & UNREADABLE_MASK) {
    return '\0';
  }
  if (cb->bufp < cb->bufl) {
    if (cb->fb[cb->bufp] == '\n') {
      cb->flags |= LINEND_MASK;
    }
    return cb->fb[cb->bufp++];
  }
  cb->bufl = read(cb->fd, cb->fb, sizeof(cb->fb));
  if (cb->bufl == -1) {
    fprintf(stderr, "Error reading from fd %u: %m", cb->fd);
    exit(1);
  }
  if (cb->bufl == 0) {
    cb->flags |= UNREADABLE_MASK;
    return '\0';
  }
  cb->bufp = 1;
  return cb->fb[0];
}

float read_float(struct fbuf *__restrict cb) {
  char ch;
  float sgn = 1.0f;
  float res = 0.0f;
  float adm = 0.1f;
  uint8_t afterDot = 0;
  ch = _getc(cb);
  //fprintf(stdout, "[%c]\n", ch);
  if (ch == '-') {
    sgn = -1.0f;
  } else if (ch == '/') {
    return 1.0f;
  } else {
    res = ch - '0';
  }
  while ((ch = _getc(cb)) && (('0' <= ch && ch <= '9') || (ch == '.'))) {
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
    cb->flags |= LINEND_MASK;
  }
  if (ch == '\0') {
    cb->flags |= UNREADABLE_MASK;
  }
  return res * sgn;
}

void next_token(char *__restrict tok, struct fbuf *__restrict cb) {
  --tok;
  do {
    ++tok;
    *tok = _getc(cb);
  } while (*tok != ' ' && *tok != '\r' && *tok != '\n' && *tok != '\0');
  if (*tok == '\r') {
    _getc(cb);
  }
  if (*tok == '\0') {
    cb->flags |= UNREADABLE_MASK;
  }
  *tok = '\0';
}

enum MIDS {
  NEWMTL, AMBIENT, DIFFUSE, SPEC, SPECE, TRANSP, OPTD, ILLUM, EMMISIVE, MCOMMENT, TAMB, TDIFF, TSPEC, TBUMP, TDISP, MUNDEF
};

enum MIDS mid_tok(char *__restrict tok) {
  switch (tok[0]) {
    case 'n':
      return NEWMTL;
    case '\0':
    case '#':
      return MCOMMENT;
    case 'K':
      switch (tok[1]) {
        case 'a':
          return AMBIENT;
        case 'd':
          return DIFFUSE;
        case 's':
          return SPEC;
        case 'e':
          return EMMISIVE;
      }
      break;
    case 'N':
      switch (tok[1]) {
        case 's':
          return SPECE;
        case 'i':
          return OPTD;
      }
      break;
    case 'd':
    case 'T':
      return TRANSP;
    case 'i':
      return ILLUM;
    case 'm':
      switch (tok[4]) {
        case 'K':
          switch (tok[1]) {
            case 'a':
              return TAMB;
            case 'd':
              return TDIFF;
            case 's':
              return TSPEC;
          }
          break;
      }
      break;
    default:
      return MUNDEF;
  }
  return MUNDEF;
}

uint8_t *__restrict read_png(char *__restrict fname, char *__restrict dname, uint32_t *__restrict width, uint32_t *__restrict height) {
  png_image img;

  memset(&img, 0, sizeof(img));
  img.version = PNG_IMAGE_VERSION;

  { /// TODO: Libpng openat
    char tmpname[256];
    snprintf(tmpname, 256, "%s/%s", dname, fname);
    if (png_image_begin_read_from_file(&img, tmpname) == 0) {
      fprintf(stderr, "ERR1: %s\n", fname);
      return NULL;
    }
  }

  png_bytep buf;

  img.format = PNG_FORMAT_RGB;

  buf = malloc(PNG_IMAGE_SIZE(img) + 3);

  if (png_image_finish_read(&img, NULL, buf, 0, NULL) == 0) {
    fprintf(stderr, "ERR2: %s\n", fname);
    free(buf);
    return NULL;
  }

  *width = img.width;
  *height = img.height;

  return buf;
}


void get_texture(char *__restrict fname, char *__restrict dname, struct texture *__restrict tex) {
  glGenTextures(1, &tex->i);

  uint8_t *buf = read_png(fname, dname, &tex->w, &tex->h);

  glBindTexture(GL_TEXTURE_2D, tex->i);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, tex->w, tex->h, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
  glGenerateMipmap(GL_TEXTURE_2D);

  free(buf);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  ////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void parse_material(struct matv *__restrict mats, uint32_t dfd, char *__restrict fname, char *__restrict dname) {
  struct material cmat = {0};
  memset(&cmat, 0, sizeof(cmat));

  struct fbuf cb;
  memset(&cb, 0, sizeof(cb));
  char tok[128] = {0};
  uint32_t line_number = 0; 
  enum MIDS id;
  v3 om;

  cb.bufp = cb.bufl = 0;
  cb.fd = openat(dfd, fname, O_RDONLY);
  if (cb.fd == -1) {
    fprintf(stderr, "Could not open '%s': %m\n", fname);
    exit(1);
  }

  while ((cb.flags & UNREADABLE_MASK) == 0) {
    ++line_number;
    cb.flags &= ~LINEND_MASK;
    next_token(tok, &cb);
    if (cb.flags & UNREADABLE_MASK) {
      break;
    }
    id = mid_tok(tok);
    switch (id) {
      case NEWMTL:
        if (cmat.name[0] != '\0') {
          matvp(mats, cmat);
          memset(&cmat, 0, sizeof(cmat));
        }
        next_token(tok, &cb);
        strcpy(cmat.name, tok);
        break;
      case AMBIENT:
        om.x = read_float(&cb);
        om.y = read_float(&cb);
        om.z = read_float(&cb);
        cmat.ambient = om;
        break;
      case DIFFUSE:
        om.x = read_float(&cb);
        om.y = read_float(&cb);
        om.z = read_float(&cb);
        cmat.diffuse = om;
        break;
      case SPEC:
        om.x = read_float(&cb);
        om.y = read_float(&cb);
        om.z = read_float(&cb);
        cmat.spec = om;
        break;
      case EMMISIVE:
        om.x = read_float(&cb);
        om.y = read_float(&cb);
        om.z = read_float(&cb);
        cmat.emmisive = om;
        break;
      case SPECE:
        om.x = read_float(&cb);
        cmat.spece = om.x;
        break;
      case TRANSP:
        om.x = read_float(&cb);
        cmat.transp = om.x;
        break;
      case OPTD:
        om.x = read_float(&cb);
        cmat.optd = om.x;
        break;
      case ILLUM:
        om.x = read_float(&cb);
        cmat.illum = (uint32_t)om.x;
        break;
      case MCOMMENT:
        while ((cb.flags & LINEND_MASK) == 0) {
          next_token(tok, &cb);
        }
        break;
      case TAMB:
        next_token(tok, &cb);
        get_texture(tok, dname, &cmat.tamb);
        break;
      default:
        fprintf(stderr, "Undefined token at %s:%u [%s]\n", fname, line_number, tok);
        break;
    }
  }
  matvp(mats, cmat);
  matvt(mats);

  close(cb.fd);
}

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

void maff(struct minf *__restrict m) {
  mat4 ps;
  ps[0][0] = m->scale.x; ps[0][1] =          0; ps[0][2] =          0; ps[0][3] = m->pos.x; 
  ps[1][0] =          0; ps[1][1] = m->scale.y; ps[1][2] =          0; ps[1][3] = m->pos.y; 
  ps[2][0] =          0; ps[2][1] =          0; ps[2][2] = m->scale.z; ps[2][3] = m->pos.z; 
  ps[3][0] =          0; ps[3][1] =          0; ps[3][2] =          0; ps[3][3] =        1; 
  
  mat3 qm;
  quat_to_mat(qm, m->rot);
  matmul43(m->aff, ps, qm);
}

void init_model(struct model *__restrict m) {
  memset(m, 0, sizeof(struct model));
  floatvi(&m->v);
  mativi(&m->m);
  m->b.i = (vec3) { 9999999999.0,  9999999999.0,  9999999999.0};
  m->b.a = (vec3) {-9999999999.0, -9999999999.0, -9999999999.0};
}

void init_object(struct object *__restrict o, char *__restrict name) {
  memset(o, 0, sizeof(struct object));

  minfvi(&o->mins);
  strncpy(o->name, name, 64);
}

union OBJ_MEMBERS {
  vec3 v3;
  vec2 v2;
  struct faceev fv; 
  struct material mat;
};

enum IDS { VERTEX, TEXTURE_COORDS, VERTEX_NORMAL, FACE, MATERIAL_LIB, USE_MATERIAL, OBJECT, GROUP, SMOOTH_SHADING, OCOMMENT, OUNDEF };

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
          return OUNDEF;
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
      return OCOMMENT;
    default:
      return OUNDEF;
  }
  return OUNDEF;
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

uint32_t gmi(struct matv *__restrict materials, char *__restrict matn) {
  int32_t i;
  for(i = 0; i < materials->l; ++i) {
    if (strcmp(matn, materials->v[i].name) == 0) { 
      return i;
    }
  }
  return 0;
}

void create_vao(float *__restrict v, uint32_t s, uint32_t *__restrict vbo, uint32_t *__restrict vao) {
  glGenVertexArrays(1, vao);
  glGenBuffers(1, vbo);

  glBindVertexArray(*vao);
  glBindBuffer(GL_ARRAY_BUFFER, *vbo);
  glBufferData(GL_ARRAY_BUFFER, s * sizeof(float), v, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (0 * sizeof(float)));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
}

void parse_model_file(struct modv *__restrict mods, struct matv *__restrict materials, uint32_t fd, uint32_t dfd, char *__restrict fname, char *__restrict dname) {
  struct fbuf cb;
  struct v3v verts;
  struct v3v norms;
  struct v2v texts;
  struct model cmod;
  v3vi(&verts);
  v3vi(&norms);
  v2vi(&texts);

  memset(&cb, 0, sizeof(cb));
  memset(&cmod, 0, sizeof(cmod));
  
  cb.bufp = cb.bufl = 0;
  cb.fd = fd;

  char tok[128] = {0};
  enum IDS id;
  union OBJ_MEMBERS om;
  uint32_t line_number = 0;
  while ((cb.flags & UNREADABLE_MASK) == 0) {
    ++line_number;
    cb.flags &= ~LINEND_MASK;
    next_token(tok, &cb);
    if (cb.flags & UNREADABLE_MASK) {
      break;
    }
    
    // fprintf(stdout, "CLine: %u, tok: %s\n", line_number, tok);
    id = id_tok(tok);
    //id_to_str(id);
    switch (id) {
      case VERTEX:
        om.v3.x = read_float(&cb);
        om.v3.y = read_float(&cb);
        om.v3.z = read_float(&cb);

        cmod.b.i.x = min(cmod.b.i.x, om.v3.x);
        cmod.b.i.y = min(cmod.b.i.y, om.v3.y);
        cmod.b.i.z = min(cmod.b.i.z, om.v3.z);

        cmod.b.a.x = max(cmod.b.a.x, om.v3.x);
        cmod.b.a.y = max(cmod.b.a.y, om.v3.y);
        cmod.b.a.z = max(cmod.b.a.z, om.v3.z);

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
            add_i_dont_even_know(&om.v3, &cmod.v, &verts, &norms, &texts);
          }
        }
        break;
      case MATERIAL_LIB:
        next_token(tok, &cb);
        // fprintf(stdout, "Material lib: %s\n", tok);
        parse_material(materials, dfd, tok, dname);
        break;
      case USE_MATERIAL:
        next_token(tok, &cb);
        mativp(&cmod.m, (struct mate) { gmi(materials, tok), cmod.v.l });
        break;
      case OBJECT:
        if (cmod.name[0] != '\0') {
          create_vao(cmod.v.v, cmod.v.l, &cmod.vbo, &cmod.vao);

          mativt(&cmod.m);
          // free(cmod.v.v);
          modvp(mods, cmod);
        }
        init_model(&cmod);
        next_token(tok, &cb);
        strncpy(cmod.name, tok, 64);
        break;
      case GROUP:
        next_token(tok, &cb);
        /// GROUP
        break;
      case SMOOTH_SHADING:
        next_token(tok, &cb);
        /// Smooth Shading
        break;
      case OCOMMENT:
        while ((cb.flags & LINEND_MASK) == 0) {
          next_token(tok, &cb);
        }
        break;
      default:
        fprintf(stderr, "Undefined token at %s:%u [%s]\n", fname, line_number, tok);
        break;
    }
  }
  
  create_vao(cmod.v.v, cmod.v.l, &cmod.vbo, &cmod.vao);
  mativt(&cmod.m);
  //free(cmod.v.v);
  modvp(mods, cmod);

  free(verts.v);
  free(norms.v);
  free(texts.v);
}

void parse_folder(struct modv *__restrict mods, struct matv *__restrict mats, char *__restrict dname) {
  DIR *d = opendir(dname);
  if (d == NULL) {
    fprintf(stderr, "Could not open directory %s! %m\n", dname);
    exit(1);
  }

  struct dirent *__restrict di;
  uint32_t fd, dfd;
  dfd = dirfd(d);
  while ((di = readdir(d)) != NULL) {
    if (strstr(di->d_name, ".obj")) {
      fd = openat(dfd, di->d_name, O_RDONLY);
      parse_model_file(mods, mats, fd, dfd, di->d_name, dname); /// TODO: libpng openat
      close(fd);
    }
  }

  if (mods->v[mods->l - 1].name[0] == '\0') {
    fprintf(stderr, "Couldn't find any .obj in %s!\n", dname);
  }
  closedir(d);
}

void destroy_model(struct model *__restrict mod) {
  free(mod->v.v);
  free(mod->m.v);
  free(mod);
}
