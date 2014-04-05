#ifndef BUGSP_H
#define BUGSP_H

#define ERROR_BUFFER_LEN 512

#define LASSERT(args, cond, fmt, ...)             \
    if (!(cond)) {                                \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args);                           \
        return err;                               \
    }

#define LASSERT_NUM(func, args, exp)                                            \
    if (args->count != exp) {                                                   \
        lval* err = lval_err(                                                   \
            "'%s' incorrect number of arguments received, expected %d, got %d", \
            func, exp, args->count);                                            \
        lval_del(args);                                                         \
        return err;                                                             \
    }

#define LASSERT_NUM_MIN(func, args, exp)                      \
    if (args->count < exp) {                                  \
        lval* err = lval_err(                                 \
            "'%s' not enough arguments, expected %d, got %d", \
            func, exp, args->count);                          \
        lval_del(args);                                       \
        return err;                                           \
    }

#define LASSERT_TYPE(func, args, i, exp)                             \
    if (args->cell[i]->type != exp) {                                \
        lval* err = lval_err(                                        \
            "'%s' incorrect type for arg %d, expected %s, got %s",   \
            func, i, ltype_name(exp), ltype_name(a->cell[i]->type)); \
        lval_del(args);                                              \
        return err;                                                  \
    }

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* lval types & structures */

enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;

    long num;
    char* err;
    char* sym;

    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    int count;
    lval** cell;
};

struct lenv {
    lenv* parent;
    int count;
    char** syms;
    lval** vals;
};

/* constructors & destructors */

lval* lval_err(char* fmt, ...);
lval* lval_num(long x);
lval* lval_sym(char* s);
lval* lval_fun(lbuiltin func);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
void lval_del(lval* v);

lenv* lenv_new(void);
void lenv_del(lenv* e);

/* lval helpers */

lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
char* ltype_name(int t);

lval* lval_add(lval* v, lval* x);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
lval* lval_copy(lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* lval_call(lenv* e, lval* f, lval* a);
int lval_eq(lval* x, lval* y);

/* lenv helpers */

lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_def(lenv* e, lval* k, lval* v);
lenv* lenv_copy(lenv* e);

/* builtin functions */

lval* builtin_list(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_init(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_len(lenv* e, lval* a);
lval* builtin_add(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_lt(lenv* e, lval* a);
lval* builtin_gt(lenv* e, lval* a);
lval* builtin_le(lenv* e, lval* a);
lval* builtin_ge(lenv* e, lval* a);
lval* builtin_eq(lenv* e, lval* a);
lval* builtin_ne(lenv* e, lval* a);
lval* builtin_lambda(lenv* e, lval* a);
lval* bulitin_var(lenv* e, lval* a, char* func);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_put(lenv* e, lval* a);

void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

#endif
