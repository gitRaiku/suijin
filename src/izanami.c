#include "izanami.h"

VECTOR_SUITE(mat, struct matv *__restrict, struct material)


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
  if (ch == '-') {
    sgn = -1.0f;
  } else if (ch == '\\') {
    return 0.0f;
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
  NEWMTL, AMBIENT, DIFFUSE, SPEC, SPECE, TRANSP, OPTD, ILLUM, EMMISIVE, COMMENT, UNDEF
};

enum MIDS mid_tok(char *__restrict tok) {
  switch (tok[0]) {
    case 'n':
      return NEWMTL;
    case '\0':
    case '#':
      return COMMENT;
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
    default:
      return UNDEF;
  }
  return UNDEF;
}

void parse_material(struct matv *__restrict mats, char *__restrict fname) {
  struct material cmat = {0};
  memset(&cmat, 0, sizeof(cmat));

  matvi(mats);
  
  struct fbuf cb;
  char tok[128] = {0};
  uint32_t line_number = 0; 
  enum MIDS id;
  v3 om;

  cb.bufp = cb.bufl = 0;
  cb.fd = open(fname, O_RDONLY);
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
  matvp(mats, cmat);
  matvt(mats);

  close(cb.fd);
}

