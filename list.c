#include "list.h"

struct list{
    int info;
    Lista* prox;
};

Lista* criaLista(){
    return NULL;
}

Lista* insereLista(int info, Lista* l){
    Lista* no = (Lista*) malloc(sizeof(Lista));
    no->info = info;
    no->prox = NULL;

    if(l == NULL)
        l = no;
    else{
        no->prox = l;
        l = no;
    }

    return l;
}

Lista* removeLista(int info, Lista* l){
    Lista* p = l, *aux;
    Lista*  ant =  NULL;
    while(p!=NULL){
        if(p->info == info){
            if(ant == NULL){ //Primeiro da lista
                aux = p;
                l = p->prox;
                free(aux);
            }else{
                aux = p;
                ant->prox = p->prox;
                free(aux);
            }
            break;
        }
        ant = p;
        p = p->prox;
    }

    return l;
}

Lista* liberaLista(Lista* l){
    Lista* p = l, *aux;
    while(p!=NULL){
        aux = p;
        p = p->prox;
        free(aux);
    }
    return NULL;
}

Lista* proxLista(Lista* p){
    return p->prox;
}
int elementoLista(Lista* p){
    return p->info;
}

void imprime(Lista* l){
    Lista* p = l;
    while(p!=NULL){
        printf("%d ",p->info);
        p = p->prox;
    }
    printf("\n");
}