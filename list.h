#ifndef LIST_H_
#define LIST_H_
#include <stdio.h>
#include <stdlib.h>

typedef struct list Lista;

Lista* criaLista();
Lista* insereLista(int info, Lista* l);
Lista* removeLista(int info, Lista* l);
Lista* liberaLista(Lista* l);
Lista* proxLista(Lista* p);
int elementoLista(Lista* p);

void imprime(Lista* l);
#endif