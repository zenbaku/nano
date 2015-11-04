
typedef int Hydrogen;
typedef int Oxygen;
/* Nota: su programa en h2o.c no sabe que Hydrogen y Oxygen son int */

/* El tipo concreto de struct h2o es definido en testh2o.c */
typedef struct h2o *H2O;
/* nota: programa en h2o.c no sabe cual como es struct h2o */

/* Prodedimiento dado en testh2o.c: */
H2O makeH2O(Hydrogen h1, Hydrogen h2, Oxygen o);

/* Procedimientos que Ud. debe programar en h2o.c */
void initH2O(void);
H2O combineOxy(Oxygen o);
H2O combineHydro(Hydrogen h);
