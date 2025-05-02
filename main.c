#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#pragma warning(disable : 4996)

struct LS* Head = NULL;

// Временная константа для удобства написания кода
#define MAX_NAME_LEN 100

typedef enum Images 
{ 
    fragile, 
    toxic, 
    perishable, 
    acrid, 
    inflammable, 
    frozen,
    UNKNOWN // Для отслеживания неккореректных запросов
};

enum Images str_to_image(char* str)
{
    if (strcmp(str, "fragile") == 0) { return fragile; }
    if (strcmp(str, "toxic") == 0) { return toxic; }
    if (strcmp(str, "perishable") == 0) { return perishable; }
    if (strcmp(str, "acrid") == 0) { return acrid; }
    if (strcmp(str, "inflammable") == 0) { return inflammable; }
    if (strcmp(str, "frozen") == 0) { return frozen; }
    return UNKNOWN;
}
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
    struct LS* next; // Указатель на след. элемента 
} LS;

/* Взаимодействие с линейным списком */

// Вспомогательная штука - создаёт node и возращает указатель на него
LS* create_node(char* comes, char* sender, char* name, \
    int weight, int count, enum Images images, char* worker) {
    LS* new_node = (LS*)malloc(sizeof(LS));
    if (new_node == NULL) {
        printf("Ошибка выделения памяти!\n");
        exit(1);
    }
    strcpy(new_node->comes, comes);
    strcpy(new_node->sender, sender);
    strcpy(new_node->name, name);
    new_node->weight = weight;
    new_node->count = count;
    new_node->images = images;
    strcpy(new_node->worker, worker);

    new_node->next = NULL;
    return new_node;
}

// Вот через это добавляем ноды
void add_node(char* comes, char* sender, char* name, \
    int weight, int count, enum Images images, char worker[MAX_NAME_LEN]) {

    LS* new_node = create_node(comes, sender, name, weight, count, images, worker);

    if (Head == NULL) {
        Head = new_node;
        return;
    }

    LS* last = Head;
    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_node;
}

/*
void add_node(LS* node, char comes[11], char sender[MAX_NAME_LEN], char name[MAX_NAME_LEN], \
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
*/

void del_node(LS* node) // Удалить элемент из ЛС node - укаель на элемент после которого удаляем э-нт
{
    if (node->next != NULL)
    {
        LS* free_pointer = node->next;
       // node->next = node->next->next;
        free(free_pointer);
    }
    else
    {
        printf("Nothing to delete");
    }
}

// Команды к БД:
void select(char* args)
{
    return;
}

void delete(char* args)
{
    return;
}

void insert(char* args)
{
    char* comes = NULL;
    char* sender = NULL;
    char* name = NULL;
    int weight;
    int count;
    enum Images images;
    char* worker = NULL;
    short was_sender = 0, was_comes = 0, was_name = 0, was_weight = 0, was_count = 0, was_images = 0, was_worker = 0;
    char* pair = strtok(args, ",");
    while (pair != NULL) // str format: field=value
    {
        char* field = pair;
        char* equal_pos = strchr(pair, '=');
        size_t key_len = equal_pos - field;
        field[key_len] = '\0';
        char* value = equal_pos + 1;

        if (strcmp(field, "comes") == 0 && was_comes == 1) { printf("input error"); return; }
        if (strcmp(field, "comes") == 0 && was_comes == 0) { comes = value; was_comes = 1; }

        if (strcmp(field, "sender") == 0 && was_sender == 1) { printf("input error"); return; }
        if (strcmp(field, "sender") == 0 && was_sender == 0) { sender = value; was_sender = 1; }

        if (strcmp(field, "name") == 0 && was_name == 1) { printf("input error"); return; }
        if (strcmp(field, "name") == 0 && was_name == 0) { name = value; was_name = 1; }

        if (strcmp(field, "weight") == 0 && was_weight == 1) { printf("input error"); return; }
        if (strcmp(field, "weight") == 0 && was_weight == 0) { weight = atoi(value); was_weight = 1; }

        if (strcmp(field, "count") == 0 && was_count == 1) { printf("input error"); return; }
        if (strcmp(field, "count") == 0 && was_count == 0) { count = atoi(value); was_count = 1; }

        if (strcmp(field, "images") == 0 && was_images == 1) { printf("input error"); return; }
        if (strcmp(field, "images") == 0 && was_images == 0) 
        { 
            images = str_to_image(value); 
            if (images == (enum Images)UNKNOWN)
            {
                printf("Unknown color!");
                return;
            }
            was_images = 1; 
        }

        if (strcmp(field, "worker") == 0 && was_worker == 1) { printf("input error"); return; }
        if (strcmp(field, "worker") == 0 && was_worker == 0) { worker = value; was_worker = 1; }
        pair = strtok(NULL, ",");
    }
    if (was_comes == 1 && was_count == 1 && was_images == 1 && was_name == 1 && was_sender == 1 && was_weight == 1 && was_worker == 1)
    {
        add_node(comes = comes, sender = sender, name = name, weight = weight, count = count, images = images, worker = worker);
    }
    else
    {
        printf("Not everyone fields: input error");
        return;
    }
    return;
}

void update(char* args)
{
    return;
}

void uniq(char* args)
{
    return;
}

void print_nodes(LS* node) // node -> указатель на первый э-нт ЛС
{
    if (node != NULL)
    {
        printf("%s\n", node->comes);
        printf("%s\n", node->sender);
        printf("%s\n", node->name);
        printf("%d\n", node->weight);
        printf("%d\n", node->count);
        switch (node->images)
        {
        case fragile:
            printf("fragile\n");
            break;
        case toxic:
            printf("toxic\n");
            break;
        case perishable:
            printf("perishable\n");
            break;
        case acrid:
            printf("acrid\n");
            break;
        case inflammable:
            printf("inflammable\n");
            break;
        case frozen:
            printf("frozen\n");
            break;
        }
        printf("%s\n", node->worker);
        printf("\\/\n\n");
    }
    else
    {
        return;
    }
    print_nodes(node->next);
}




int main()
{
    setlocale(LC_ALL, "rus");
    FILE* fi = fopen("commands.txt", "r");
    char command[1000];
    while (fgets(command, 1000, fi))
    {
        char *str; // select, delete, insert, update, uniq
        str = strtok(command, " ");
        if (!strcmp(str, "insert")) { insert(strtok(NULL, "\0")); }
        if (!strcmp(str, "select")) { select(strtok(NULL, "\0")); }
        if (!strcmp(str, "delete")) { delete(strtok(NULL, "\0")); }
        if (!strcmp(str, "update")) { update(strtok(NULL, "\0")); }
        if (!strcmp(str, "uniq")) {     uniq(strtok(NULL, "\0")); }
    }
    // TODO work with terminal
    print_nodes(Head);
    return 1;
}