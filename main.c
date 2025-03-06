#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Временная константа для удобства написания кода
#define MAX_NAME_LEN 100
#define FILL(node, comes, sender, name, weight, count, images, worker) {\
    strcpy(node->comes, comes);\
    strcpy(node->sender, sender);\
    strcpy(node->name, name);\
    node->weight =  weight;\
    node->count = count;\
    strcpy(node->worker, worker);\
}

typedef enum Images { fragile, toxic, perishable, acrid, inflammable, frozen };
// Реализация линейного списка
typedef struct
{
    char comes[11]; // DD.MM.YYYY - дата поступления 
    char sender[MAX_NAME_LEN]; // Грузоотправитель
    char name[MAX_NAME_LEN]; // Наименование товара
    int weight; // Вес
    int count; // Количество
    enum Images images;
    char worker[MAX_NAME_LEN]; // Принял рабоник (Фамилия)
    LS* next; // Указатель на след. элемента 
} LS;

// Взаимодействие с линейным списком
LS* add_node(LS* node, char comes[11], char sender[MAX_NAME_LEN], char name[MAX_NAME_LEN], \
            int weight, int count, enum Images images, char worker[MAX_NAME_LEN]) // Добавить элемент в ЛС node - укаель на последний э-нт
{
    if (node == NULL) // Создаём корневой узел
    {
        LS* new_node = (LS*)malloc(sizeof(LS));
        FILL(new_node, comes, sender, name, weight, count, images, worker);
        new_node->next = NULL;
    }
    else // Добавляем элемент
    {
        LS* new_node = (LS*)malloc(sizeof(LS));
        FILL(new_node, comes, sender, name, weight, count, images, worker);
        
        new_node->next = node->next;
        node->next = new_node;
    }
}

void del_node()
{

}


// Команды к БД:
void select()
{
    return;
}

void delete()
{
    return;
}

void insert()
{
    return;
}

void update()
{
    return;
}

void uniq()
{
    return;
}


int main()
{
    // TODO read from file
    // TODO work with terminal
    return 1;
}