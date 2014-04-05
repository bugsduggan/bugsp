#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <histedit.h>

#include "mpc.h"
#include "bugsp.h"

/* constructors & destructors */

lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->err = malloc(ERROR_BUFFER_LEN);
    vsnprintf(v->err, ERROR_BUFFER_LEN - 1, fmt, va);
    v->err = realloc(v->err, strlen(v->err) + 1);

    va_end(va);
    return v;
}

lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval* lval_bool(int x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_BOOL;
    v->num = x;
    return v;
}

lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {
    switch(v->type) {
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_NUM:
        case LVAL_BOOL:
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_STR:
            free(v->str);
            break;
        case LVAL_FUN:
            if (v->builtin == NULL) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }

    free(v);
}

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

/* lenv helpers */

lval* lenv_get(lenv* e, lval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("unbound symbol '%s'", k->sym);
    }
}

void lenv_put(lenv* e, lval* k, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            e->syms[i] = realloc(e->syms[i], strlen(k->sym) + 1);
            strcpy(e->syms[i], k->sym);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v) {
    while (e->parent) {
        e = e->parent;
    }
    lenv_put(e, k, v);
}

lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->parent = e->parent;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for (int i = 0; i < n->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

/* lval helpers */

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch(v->type) {
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_NUM:
        case LVAL_BOOL:
            x->num = v->num;
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;
        case LVAL_FUN:
            x->builtin = v->builtin;
            if (x->builtin == NULL) {
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return x;
}

lval* lval_read_num(mpc_ast_t* t) {
    long x = strtol(t->contents, NULL, 10);
    if (errno == ERANGE) {
        return lval_err("invalid number");
    }
    return lval_num(x);
}

lval* lval_read_str(mpc_ast_t* t) {
    t->contents[strlen(t->contents) - 1] = '\0';
    char* unescaped = malloc(strlen(t->contents + 1));
    strcpy(unescaped, t->contents + 1);
    unescaped = mpcf_unescape(unescaped);
    lval* str = lval_str(unescaped);
    free(unescaped);
    return str;
}

lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        return lval_read_num(t);
    }
    if (strstr(t->tag, "symbol")) {
        return lval_sym(t->contents);
    }
    if (strstr(t->tag, "string")) {
        return lval_read_str(t);
    }

    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "sexpr")) {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "qexpr")) {
        x = lval_qexpr();
    }

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, ")") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "{") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "}") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->tag, "regex") == 0) {
            continue;
        }
        if (strstr(t->children[i]->tag, "comment")) {
            continue;
        }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(lval* v) {
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

void lval_print(lval* v) {
    switch(v->type) {
        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;
        case LVAL_NUM:
            printf("%li", v->num);
            break;
        case LVAL_BOOL:
            if (v->num == 0) {
                printf("False");
            } else {
                printf("True");
            }
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_STR:
            lval_print_str(v);
            break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
    }
}

void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* lval_join(lval* x, lval* y) {
    while(y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    if (v->count == 0) {
        return v;
    }
    if (v->count == 1) {
        return lval_take(v, 0);
    }

    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err("S-Expression starts with incorrect type (%s)",
                             ltype_name(f->type));
        lval_del(v);
        lval_del(f);
        return err;
    }

    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM) {
        return lenv_get(e, v);
    }
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
    }
    return v;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
    if (f->builtin) {
        return f->builtin(e, a);
    }

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("function received too many arguments");
        }

        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    lval_del(a);

    if (f->formals->count == 0) {
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        return lval_copy(f);
    }
}

int lval_eq(lval* x, lval* y) {
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case LVAL_ERR:
            return (strcmp(x->err, y->err) == 0);
        case LVAL_NUM:
        case LVAL_BOOL:
            return (x->num == y->num);
        case LVAL_SYM:
            return (strcmp(x->sym, y->sym) == 0);
        case LVAL_STR:
            return (strcmp(x->str, y->str) == 0);
        case LVAL_FUN:
            if (x->builtin) {
                return x->builtin == y->builtin;
            } else {
                return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
            }
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            if (x->count != y->count) {
                return 0;
            }
            for (int i = 0; i < x->count; i++) {
                if (!lval_eq(x->cell[0], y->cell[0])) {
                    return 0;
                }
            }
            return 1;
    }

    return 0;
}

char* ltype_name(int t) {
    switch(t) {
        case LVAL_ERR:
            return "Error";
            break;
        case LVAL_NUM:
            return "Number";
            break;
        case LVAL_BOOL:
            return "Bool";
            break;
        case LVAL_SYM:
            return "Symbol";
            break;
        case LVAL_STR:
            return "String";
            break;
        case LVAL_FUN:
            return "Function";
            break;
        case LVAL_SEXPR:
            return "S-Expression";
            break;
        case LVAL_QEXPR:
            return "Q-Expression";
            break;
        default:
            return "Unknown";
    }
}

/* builtins */

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT(a, (a->cell[0]->count != 0),
            "'head' passed {}");

    lval* v = lval_take(a, 0);
    while(v->count > 1) {
        lval_del(lval_pop(v, 1));
    }
    return v;
}

lval* builtin_tail(lenv* e, lval* a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT(a, (a->cell[0]->count != 0),
            "'tail' passed {}");

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_init(lenv* e, lval* a) {
    LASSERT_NUM("init", a, 1);
    LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
    LASSERT(a, (a->cell[0]->count != 0),
            "'init' passed {}");

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, v->count - 1));
    return v;
}

lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval* x = lval_pop(a, 0);
    while(a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }
    lval_del(a);
    return x;
}

lval* builtin_len(lenv* e, lval* a) {
    LASSERT_NUM("len", a, 1);
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);

    lval* v = lval_num(a->cell[0]->count);
    lval_del(a);
    return v;
}

lval* builtin_add(lenv* e, lval* a) {
    LASSERT_NUM_MIN("+", a, 2);

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("+", a, i, LVAL_NUM);
    }

    lval* x = lval_pop(a, 0);
    while (a->count > 0) {
        lval* y = lval_pop(a, 0);
        x->num += y->num;
        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* builtin_sub(lenv* e, lval* a) {
    LASSERT_NUM_MIN("-", a, 1);

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("-", a, i, LVAL_NUM);
    }

    lval* x = lval_pop(a, 0);
    if (a->count == 0) {
        x->num = -x->num;
    }
    while (a->count > 0) {
        lval* y = lval_pop(a, 0);
        x->num -= y->num;
        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* builtin_mul(lenv* e, lval* a) {
    LASSERT_NUM_MIN("*", a, 2);

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("*", a, i, LVAL_NUM);
    }

    lval* x = lval_pop(a, 0);
    while (a->count > 0) {
        lval* y = lval_pop(a, 0);
        x->num *= y->num;
        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* builtin_div(lenv* e, lval* a) {
    LASSERT_NUM_MIN("/", a, 2);

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("/", a, i, LVAL_NUM);
    }

    lval* x = lval_pop(a, 0);
    while (a->count > 0) {
        lval* y = lval_pop(a, 0);
        if (y->num == 0) {
            lval_del(x);
            lval_del(y);
            lval_del(a);
            return lval_err("division by zero");
        }
        x->num /= y->num;
        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* builtin_bool(lenv* e, lval* a) {
    LASSERT_NUM("bool", a, 1);
    LASSERT_TYPE("bool", a, 0, LVAL_NUM);

    lval* x = lval_bool(a->cell[0]->num);
    lval_del(a);
    return x;
}

lval* builtin_lt(lenv* e, lval* a) {
    LASSERT_NUM("<", a, 2);
    LASSERT_TYPE("<", a, 0, LVAL_NUM);
    LASSERT_TYPE("<", a, 1, LVAL_NUM);

    int x = a->cell[0]->num;
    int y = a->cell[1]->num;
    lval_del(a);
    return lval_bool(x < y);
}

lval* builtin_gt(lenv* e, lval* a) {
    LASSERT_NUM(">", a, 2);
    LASSERT_TYPE(">", a, 0, LVAL_NUM);
    LASSERT_TYPE(">", a, 1, LVAL_NUM);

    int x = a->cell[0]->num;
    int y = a->cell[1]->num;
    lval_del(a);
    return lval_bool(x > y);
}

lval* builtin_le(lenv* e, lval* a) {
    LASSERT_NUM("<=", a, 2);
    LASSERT_TYPE("<=", a, 0, LVAL_NUM);
    LASSERT_TYPE("<=", a, 1, LVAL_NUM);

    int x = a->cell[0]->num;
    int y = a->cell[1]->num;
    lval_del(a);
    return lval_bool(x <= y);
}

lval* builtin_ge(lenv* e, lval* a) {
    LASSERT_NUM(">=", a, 2);
    LASSERT_TYPE(">=", a, 0, LVAL_NUM);
    LASSERT_TYPE(">=", a, 1, LVAL_NUM);

    int x = a->cell[0]->num;
    int y = a->cell[1]->num;
    lval_del(a);
    return lval_bool(x >= y);
}

lval* builtin_eq(lenv* e, lval* a) {
    LASSERT_NUM("==", a, 2);
    int r = lval_eq(a->cell[0], a->cell[1]);
    lval_del(a);
    return lval_bool(r);
}

lval* builtin_ne(lenv* e, lval* a) {
    LASSERT_NUM("!=", a, 2);
    int r = !lval_eq(a->cell[0], a->cell[1]);
    lval_del(a);
    return lval_bool(r);
}

lval* builtin_and(lenv* e, lval* a) {
    LASSERT_NUM("&&", a, 2);
    LASSERT_TYPE("&&", a, 0, LVAL_BOOL);
    LASSERT_TYPE("&&", a, 1, LVAL_BOOL);

    int r = (a->cell[0]->num == 1 && a->cell[1]->num == 1);
    lval_del(a);
    return lval_bool(r);
}

lval* builtin_or(lenv* e, lval* a) {
    LASSERT_NUM("||", a, 2);
    LASSERT_TYPE("||", a, 0, LVAL_BOOL);
    LASSERT_TYPE("||", a, 1, LVAL_BOOL);

    int r = (a->cell[0]->num == 1 || a->cell[1]->num == 1);
    lval_del(a);
    return lval_bool(r);
}

lval* builtin_not(lenv* e, lval* a) {
    LASSERT_NUM("!", a, 1);
    LASSERT_TYPE("!", a, 0, LVAL_BOOL);

    int r;
    if (a->cell[0]->num == 1) {
        r = 0;
    } else {
        r = 1;
    }
    lval_del(a);
    return lval_bool(r);
}

lval* builtin_if(lenv* e, lval* a) {
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_BOOL);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->num) {
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_del(a);
    return x;
}

lval* builtin_lambda(lenv* e, lval* a) {
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "cannot define non-symbol (%s)", ltype_name(a->cell[0]->cell[i]->type));
    }

    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
                "'var' cannot define non-symbol (%s)", ltype_name(syms->cell[i]->type));
    }

    LASSERT(a, (syms->count == a->count - 1),
            "'var' passed too many arguments for symbols");

    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i + 1]);
        }
        if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i + 1]);
        }
    }

    lval_del(a);
    return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) {
    return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a) {
    return builtin_var(e, a, "=");
}

lval* builtin_load(lenv* e, lval* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Bugsp, &r)) {
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            lval* x = lval_eval(e, lval_pop(expr, 0));
            if (x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }

        lval_del(expr);
        lval_del(a);

        return lval_sexpr();
    } else {
        /* parser error */
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        lval* err = lval_err("Could not load library %s", err_msg);
        free(err_msg);
        lval_del(a);

        return err;
    }
}

lval* builtin_print(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]);
        putchar(' ');
    }

    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    lval* err = lval_err(a->cell[0]->str);
    lval_del(a);
    return err;
}

/* deal with builtins */

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, "list",  builtin_list);
    lenv_add_builtin(e, "head",  builtin_head);
    lenv_add_builtin(e, "tail",  builtin_tail);
    lenv_add_builtin(e, "init",  builtin_init);
    lenv_add_builtin(e, "eval",  builtin_eval);
    lenv_add_builtin(e, "join",  builtin_join);
    lenv_add_builtin(e, "len",   builtin_len);
    lenv_add_builtin(e, "+",     builtin_add);
    lenv_add_builtin(e, "-",     builtin_sub);
    lenv_add_builtin(e, "*",     builtin_mul);
    lenv_add_builtin(e, "/",     builtin_div);
    lenv_add_builtin(e, "bool",  builtin_bool);
    lenv_add_builtin(e, "<",     builtin_lt);
    lenv_add_builtin(e, ">",     builtin_gt);
    lenv_add_builtin(e, "<=",    builtin_le);
    lenv_add_builtin(e, ">=",    builtin_ge);
    lenv_add_builtin(e, "==",    builtin_eq);
    lenv_add_builtin(e, "!=",    builtin_ne);
    lenv_add_builtin(e, "&&",    builtin_and);
    lenv_add_builtin(e, "||",    builtin_or);
    lenv_add_builtin(e, "!",     builtin_not);
    lenv_add_builtin(e, "if",    builtin_if);
    lenv_add_builtin(e, "\\",    builtin_lambda);
    lenv_add_builtin(e, "def",   builtin_def);
    lenv_add_builtin(e, "=",     builtin_put);
    lenv_add_builtin(e, "load",  builtin_load);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);
}

/* main */

int main(int argc, char**argv) {
    Number  = mpc_new("number");
    Symbol  = mpc_new("symbol");
    String  = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
    Expr    = mpc_new("expr");
    Bugsp   = mpc_new("bugsp");

    mpca_lang(MPC_LANG_DEFAULT,
        "                                                 \
            number  : /-?[0-9]+/ ;                        \
            symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&|]+/ ; \
            string  : /\"(\\\\.|[^\"])*\"/ ;              \
            comment : /;[^\\r\\n]*/ ;                     \
            sexpr   : '(' <expr>* ')' ;                   \
            qexpr   : '{' <expr>* '}' ;                   \
            expr    : <number>  | <symbol> | <string>     \
                    | <comment> | <sexpr>  | <qexpr> ;    \
            bugsp   : /^/ <expr>* /$/ ;                   \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Bugsp);

    puts("Bugsp version 0.0.1");
    puts("Press CTRL+C to exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
            lval* x = builtin_load(e, args);
            if (x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }
    }

    while(1) {
        char* input = readline("bugsp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Bugsp, &r)) {
            //mpc_ast_print(r.output);
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    lenv_del(e);
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Bugsp);
    return 0;
}
