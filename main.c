#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#pragma warning(disable : 4996)

#define MAX_NAME_LEN 100


/*** Структуры и перечислеия ***/
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

typedef struct
{
    time_t comes; // UNIX_timestamp's
    char sender[MAX_NAME_LEN]; // Грузоотправитель
    char name[MAX_NAME_LEN]; // Наименование товара
    int weight; // Вес
    int count; // Количество
    enum Images images;
    char worker[MAX_NAME_LEN]; // Принял рабоник (Фамилия)
    struct LS* next; // Указатель на след. элемента 
} LS;
struct LS* Head;


/*** Функции и процедуры ***/


/** Обяъвние **/

char** split(char* str, char delim, int* count);
LS** AND(LS** list1, LS** list2);
LS** check_conditions(char* token, LS* Head); // передаёшь "сырое" ( field(op)value ) условие и голову списка, получаешь указатель на список указателей подходящих эл-ов
void upd_node(LS* node, char* update_field);
/* Работа с датой */
time_t date_to_timestamp(char* date_str);
void timestamp_to_date(time_t timestamp, char* output);
/* Взаимодействие с LS */
LS* create_node(time_t comes, char* sender, char* name, int weight, int count, enum Images images, char* worker); // Вспомогательная штука - создаёт node и возращает указатель на него
void add_node(time_t comes, char* sender, char* name, int weight, int count, enum Images images, char worker[MAX_NAME_LEN]); // Через эту функцию добавляем nod'ы
void del_node(LS* node_to_del);
/* Команды к БД */
void insert(char* args);
void select(char* args);
void delete(char* args);
void update(char* args);
void uniq(char* args);
/* print'ы */
void print_LS(LS* node);
void print_node_fields(LS* node, char* fields);


/** Реализация **/


/* Проверка условий и вспомогательные штуки */

char** split(char* str, char delim, int* count) 
{
    //if (!str || !count) return NULL;

    // Создаем копию строки для безопасной работы
    char* str_copy = strdup(str);
    if (!str_copy) return NULL;

    // Первый проход: подсчет количества токенов
    *count = 1;
    for (const char* p = str; *p; p++) {
        if (*p == delim || *p == '\0' || *p == '\n') (*count)++;
    }

    // Выделяем память под массив указателей + 1 для NULL в конце
    char** tokens = malloc((*count + 1) * sizeof(char*));
    if (!tokens) {
        free(str_copy);
        return NULL;
    }

    // Второй проход: заполнение массива токенами
    int i = 0;
    char* token = str_copy;
    char* end = str_copy;

    while (*end) {
        if (*end == delim || *end == '\0' || *end == '\n') {
            *end = '\0';  // Заменяем разделитель на терминатор
            tokens[i++] = token;
            token = end + 1;
        }
        end++;
    }
    tokens[i++] = token;  // Последний токен
    tokens[i] = NULL;     // Маркер конца массива

    *count = i - 1;  // Корректируем count (без учета NULL)
    return tokens;
}

LS** AND(LS** list1, LS** list2)
{
    // Выделяем память с обнулением (calloc)
    LS** final_list = (LS**)calloc(100, sizeof(LS*));
    if (!final_list) {
        return NULL; // Ошибка выделения памяти
    }

    // Проверка на пустые списки
    if (*list1 == NULL || *list2 == NULL) {
        free(final_list); // Освобождаем память перед возвратом
        return NULL;
    }

    int i = 0;
    // Проход по первому списку
    for (int ptr1 = 0; list1[ptr1] != NULL; ptr1++) {
        // Проход по второму списку
        for (int ptr2 = 0; list2[ptr2] != NULL; ptr2++) {
            // Найдено совпадение
            if (list1[ptr1] == list2[ptr2]) {
                // Проверка на дубликаты в результирующем списке
                int is_duplicate = 0;
                for (int j = 0; j < i; j++) {
                    if (final_list[j] == list1[ptr1]) {
                        is_duplicate = 1;
                        break;
                    }
                }

                // Добавляем элемент, если не дубликат и есть место
                if (!is_duplicate && i < 99) {
                    final_list[i++] = list1[ptr1];
                }
                break; // Прерываем внутренний цикл после нахождения совпадения
            }
        }
    }

    final_list[i] = NULL; // NULL-terminated массив
    return final_list;
}

LS** check_conditions(char* token, LS* head) 
{
    /*
    Условия бывают:
    > ; >= ; < ; <= ;
    field/in/[' ',' ']
    == ; !=
    насчёт include - хз*
    */
    /* example
    weight>84 images==frozen
    */
    char* ptr = token; // Указатель, который "ходит" по строке
    char Field[1000]; // Тут содержится поле, которое нужно проверять
    strcpy(&Field, token);
    char* field = &Field;
    char* cond; // Это вообще не нужная переменная
    char* value; // Тут значение для сравнения по условию (Короче всё, что после символов операции сравнения)
    LS** values_to_return = (LS*)malloc(sizeof(LS*) * 100); // TODO Длину этого массива
    int i = 0;
    LS* copy_head;
    for (ptr; *ptr != '\0'; ptr++) // Определяем тип операции поле, значение, условие
    {
        if (*ptr == '<')
        {
            field[ptr - token] = '\0';
            if (*(ptr + 1) == '=')
            {
                value = ptr + 2;
                if (strcmp(field, "comes") == 0)
                {
                    time_t new_value = date_to_timestamp(value);
                    while (head != NULL)
                    {
                        if (new_value >= head->comes)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "sender") == 0)
                {
                    while (head != NULL)
                    {
                        if (value >= head->sender)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "name") == 0)
                {
                    while (head != NULL)
                    {
                        if (value >= head->name)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "weight") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value >= head->weight)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "count") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value >= head->count)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "images") == 0)
                {
                    while (head != NULL)
                    {
                        enum Images new_value = str_to_image(value);
                        if (new_value >= head->images)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "worker") == 0)
                {
                    while (head != NULL)
                    {
                        if (value >= head->worker)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
            }
            else
            {
                value = ptr + 1;
                if (strcmp(field, "comes") == 0)
                {
                    time_t new_value = date_to_timestamp(value);
                    while (head != NULL)
                    {
                        if (new_value > head->comes)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "sender") == 0)
                {
                    while (head != NULL)
                    {
                        if (value > head->sender)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "name") == 0)
                {
                    while (head != NULL)
                    {
                        if (value > head->name)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "weight") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value > head->weight)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "count") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value > head->count)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "images") == 0)
                {
                    while (head != NULL)
                    {
                        enum Images new_value = str_to_image(value);
                        if (new_value > head->images)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "worker") == 0)
                {
                    while (head != NULL)
                    {
                        if (value > head->worker)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
            }
        }
        if (*ptr == '>')
        {
            field[ptr - token] = '\0';
            if (*(ptr + 1) == '=')
            {
                value = ptr + 2;
                if (strcmp(field, "comes") == 0)
                {
                    time_t new_value = date_to_timestamp(value);
                    while (head != NULL)
                    {
                        if (new_value <= head->comes)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "sender") == 0)
                {
                    while (head != NULL)
                    {
                        if (value <= head->sender)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "name") == 0)
                {
                    while (head != NULL)
                    {
                        if (value <= head->name)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "weight") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value <= head->weight)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "count") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value <= head->count)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "images") == 0)
                {
                    while (head != NULL)
                    {
                        enum Images new_value = str_to_image(value);
                        if (new_value <= head->images)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "worker") == 0)
                {
                    while (head != NULL)
                    {
                        if (value <= head->worker)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
            }
            else
            {
                value = ptr + 1;
                if (strcmp(field, "comes") == 0)
                {
                    time_t new_value = date_to_timestamp(value);
                    while (head != NULL)
                    {
                        if (new_value < head->comes)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "sender") == 0)
                {
                    while (head != NULL)
                    {
                        if (value < head->sender)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "name") == 0)
                {
                    while (head != NULL)
                    {
                        if (value < head->name)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "weight") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value < head->weight)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "count") == 0)
                {
                    int new_value = atoi(value);
                    while (head != NULL)
                    {
                        if (new_value < head->count)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "images") == 0)
                {
                    while (head != NULL)
                    {
                        enum Images new_value = str_to_image(value);
                        if (new_value < head->images)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
                if (strcmp(field, "worker") == 0)
                {
                    while (head != NULL)
                    {
                        if (value < head->worker)
                        {
                            values_to_return[i] = head;
                            i++;
                        }
                        head = head->next;
                    }
                }
            }
        }
        if (*ptr == '=' && *(ptr + 1) == '=')
        {
            field[ptr - token] = '\0';
            value = ptr + 2;
            if (strcmp(field, "comes") == 0)
            {
                time_t new_value = date_to_timestamp(value);
                while (head != NULL)
                {
                    if (new_value == head->comes)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "sender") == 0)
            {
                while (head != NULL)
                {
                    if (strcmp(value, head->sender) == 0)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "name") == 0)
            {
                while (head != NULL)
                {
                    if (strcmp(value, head->name) == 0)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "weight") == 0)
            {
                int new_value = atoi(value);
                while (head != NULL)
                {
                    if (new_value == head->weight)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "count") == 0)
            {
                int new_value = atoi(value);
                while (head != NULL)
                {
                    if (new_value == head->count)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "images") == 0)
            {
                while (head != NULL)
                {
                    enum Images new_value = str_to_image(value);
                    if (new_value == head->images)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "worker") == 0)
            {
                while (head != NULL)
                {
                    if (strcmp(value, head->worker) == 0)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
        }
        if (*ptr == '!' && *(ptr + 1) == '=')
        {
            field[ptr - token] = '\0';
            value = ptr + 2;
            if (strcmp(field, "comes") == 0)
            {
                time_t new_value = date_to_timestamp(value);
                while (head != NULL)
                {
                    if (new_value != head->comes)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "sender") == 0)
            {
                while (head != NULL)
                {
                    if (strcmp(value, head->sender) != 0)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "name") == 0)
            {
                while (head != NULL)
                {
                    if (strcmp(value, head->name) != 0)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "weight") == 0)
            {
                int new_value = atoi(value);
                while (head != NULL)
                {
                    if (new_value != head->weight)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "count") == 0)
            {
                int new_value = atoi(value);
                while (head != NULL)
                {
                    if (new_value != head->count)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "images") == 0)
            {
                while (head != NULL)
                {
                    enum Images new_value = str_to_image(value);
                    if (new_value != head->images)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
            if (strcmp(field, "worker") == 0)
            {
                while (head != NULL)
                {
                    if (strcmp(value, head->worker) != 0)
                    {
                        values_to_return[i] = head;
                        i++;
                    }
                    head = head->next;
                }
            }
        }
        if (*ptr == '/' && *(ptr + 1) == 'i' && *(ptr + 2) == 'n') 
        {
            field[ptr - token] = '\0';
            value = ptr + 5; 
            value[strlen(value) - 1] = '\0'; // value: 'arg1','arg2',...
            char* arg = strtok(value, ",");
            copy_head = head;
            while (arg != NULL)
            {
                arg++;
                arg[strlen(arg) - 1] = '\0'; // Вот отсюда arg имеет нормальный вид
                if (strcmp(field, "comes") == 0)
                {
                    time_t new_value = date_to_timestamp(arg);
                    while (copy_head != NULL)
                    {
                        if (new_value == copy_head->comes)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                if (strcmp(field, "sender") == 0)
                {
                    while (copy_head != NULL)
                    {
                        if (strcmp(arg, copy_head->sender) == 0)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                if (strcmp(field, "name") == 0)
                {
                    while (copy_head != NULL)
                    {
                        if (strcmp(arg, copy_head->name) == 0)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                if (strcmp(field, "weight") == 0)
                {
                    int new_value = atoi(arg);
                    while (copy_head != NULL)
                    {
                        if (new_value == copy_head->weight)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                if (strcmp(field, "count") == 0)
                {
                    int new_value = atoi(arg);
                    while (copy_head != NULL)
                    {
                        if (new_value == copy_head->count)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                if (strcmp(field, "images") == 0)
                {
                    while (copy_head != NULL)
                    {
                        enum Images new_value = str_to_image(arg);
                        if (new_value == copy_head->images)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                if (strcmp(field, "worker") == 0)
                {
                    while (copy_head != NULL)
                    {
                        if (strcmp(arg, copy_head->worker) == 0)
                        {
                            values_to_return[i] = copy_head;
                            i++;
                        }
                        copy_head = copy_head->next;
                    }
                }
                copy_head = head;
                arg = strtok(NULL, ",");
            }  
        }
    }
    values_to_return[i] = NULL;
    return values_to_return;
}

/* Работа с датой */

time_t date_to_timestamp(char* date_str) {
    struct tm tm = { 0 }; // Инициализируем структуру нулями
    time_t timestamp;

    // Разбираем строку в формате DD.MM.YYYY
    if (sscanf(date_str, "%d.%d.%d", &tm.tm_mday, &tm.tm_mon, &tm.tm_year) != 3) {
        return -1; // Ошибка формата
    }

    // Корректировка полей структуры tm
    tm.tm_mon -= 1;    // Месяцы в struct tm: 0-11
    tm.tm_year -= 1900; // Год в struct tm: с 1900

    // Преобразуем в time_t (UNIX-время)
    timestamp = mktime(&tm);

    return timestamp;
}

void timestamp_to_date(time_t timestamp, char* output) {
    struct tm* tm_info = localtime(&timestamp);
    snprintf(output, 11, "%02d.%02d.%04d",
        tm_info->tm_mday,
        tm_info->tm_mon + 1,  // +1, т.к. месяцы 0-11
        tm_info->tm_year + 1900); // +1900, т.к. год отсчитывается с 1900
}

/* Взаимодействие с LS */

LS* create_node(time_t comes, char* sender, char* name, \
    int weight, int count, enum Images images, char* worker) {
    LS* new_node = (LS*)malloc(sizeof(LS));
    if (new_node == NULL) {
        printf("Ошибка выделения памяти!\n");
        exit(1);
    }
    new_node->comes = comes;
    strcpy(new_node->sender, sender);
    strcpy(new_node->name, name);
    new_node->weight = weight;
    new_node->count = count;
    new_node->images = images;
    strcpy(new_node->worker, worker);

    new_node->next = NULL;
    return new_node;
}

void add_node(time_t comes, char* sender, char* name, \
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

void del_node(LS* node_to_del) // Удалить элемент из ЛС node - указатель на удаляемый э-нт
{
    // Если удаляемый узел — голова списка
    if (Head == node_to_del) {
        Head = node_to_del->next;
        free(node_to_del);
        return;
    }

    // Ищем предыдущий узел перед nodeToDelete
    LS* current = Head;
    while (current != NULL && current->next != node_to_del) {
        current = current->next;
    }

    // Если узел не найден в списке
    if (current == NULL) {
        printf("Узел не принадлежит списку!\n");
        return;
    }

    // Перелинковка и удаление
    current->next = node_to_del->next;
    free(node_to_del);
    return;
}

void upd_node(LS* node, char* update_fields)
{
    int count = 0;
    char** tokens = split(update_fields, ',', &count);
    for (int i = 0; i <= count; i++) // tokens[i]: field=update
    {
        char* str = strdup(tokens[i]);
        int j = 0;
        char** string = split(str, '=', &j);
        char* field = string[0];
        char* value = string[1];
        if (strcmp(field, "comes") == 0)
        {
            node->comes = date_to_timestamp(value);
        }
        if (strcmp(field, "sender") == 0)
        {
            strcpy(node->sender, value);
        }
        if (strcmp(field, "name") == 0)
        {
            strcpy(node->name, value);
        }
        if (strcmp(field, "weight") == 0)
        {
            node->weight = (int)value;
        }
        if (strcmp(field, "count") == 0)
        {
            node->count = (int)value;
        }
        if (strcmp(field, "images") == 0)
        {
            node->images = str_to_image(value);
        }
        if (strcmp(field, "worker") == 0)
        {
            strcpy(node->worker, value);
        }
    }
    return;
}
/* Команды к БД */

void select(char* args) // TODO Переписать strtok на что-то адекватное, а то не работает ни ку я
{
    int count = 0;
    char** tokens = split(args, ' ', &count);
    char* fields_out = tokens[0];
    char* token = tokens[1]; // token: field(op)value
    LS** list = check_conditions(token, Head);
    if (count == 2) // Если всего одно условие - выводим все подошедшие записи
    {
        for (int i = 0; *(list + i) != NULL; i++)
        {
            print_node_fields(list[i], fields_out);
        }
    }
    else
    {
        for (int i = 2; i < count; i++) 
        {
            list = AND(list, check_conditions(tokens[i], Head));
        }
        if (list == NULL)
        {
            return;
        }
        for (int i = 0; *(list + i) != NULL; i++)
        {
            print_node_fields(list[i], fields_out);
        }
    }
    return;
}

void delete(char* args)
{
    int count = 0;
    char** tokens = split(args, ' ', &count);
    LS** list = check_conditions(tokens[0], Head);
    if (count == 1)
    {
        for (int i = 0; *(list + i) != NULL; i++)
        {
            del_node(list[i]);
        }

    }
    else
    {
        for (int i = 1; i < count; i++)
        {
            list = AND(list, check_conditions(tokens[i], Head));
        }
        if (list == NULL)
        {
            return;
        }
        for (int i = 0; *(list + i) != NULL; i++)
        {
            del_node(list[i]);
        }
    }
    return;
}

void insert(char* args)
{
    time_t comes;
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
        if (value[strlen(value) - 1] == '\n') // проверка для нормального вывода в будущем
        {
            value[strlen(value) - 1] = '\0';
        }
        if (strcmp(field, "comes") == 0 && was_comes == 1) { printf("input error"); return; }
        if (strcmp(field, "comes") == 0 && was_comes == 0) { comes = date_to_timestamp(value); was_comes = 1; }

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
    int count = 0;
    char** tokens = split(args, ' ', &count);
    LS** list = check_conditions(tokens[1], Head);
    if (count == 1) // В случае, если не указаны условия - меняем всё
    {
        LS* node = Head;
        while (node != NULL)
        {
            upd_node(node, tokens[0]);
            node = node->next;
        }
    }
    if (count == 2)
    {
        for (int i = 0; *(list + i) != NULL; i++)
        {
            upd_node(list[i], tokens[0]);
        }

    }
    else
    {
        for (int i = 2; i < count; i++)
        {
            list = AND(list, check_conditions(tokens[i], Head));
        }
        if (list == NULL)
        {
            return;
        }
        for (int i = 0; *(list + i) != NULL; i++)
        {
            upd_node(list[i], tokens[0]);
        }
    }
    return;
}

void uniq(char* args) {
    if (Head == NULL) {
        return;
    }

    LS* current = Head;
    LS* previous = NULL;
    int count = 0;
    char** fields = split(args, ',', &count);
    while (current != NULL) 
    {
        LS* runner = Head;
        int is_equale = 0;
        
        // Проверяем, есть ли дубликат перед текущим узлом
        while (runner != current) 
        {
            for (int i = 0; i < count; i++)
            {
                if (strcmp(fields[i], "comes") == 0)
                {
                    if (current->comes == runner->comes)
                    {
                        is_equale = 1;
                    }
                }
                if (strcmp(fields[i], "sender") == 0)
                {
                    if (strcmp(current->sender, runner->sender) == 0)
                    {
                        is_equale = 1;
                    }
                }
                if (strcmp(fields[i], "name") == 0)
                {
                    if (strcmp(current->name, runner->name) == 0)
                    {
                        is_equale = 1;
                    }
                }
                if (strcmp(fields[i], "weight") == 0)
                {
                    if (current->weight == runner->weight)
                    {
                        is_equale = 1;
                    }
                }
                if (strcmp(fields[i], "count") == 0)
                {
                    if (current->count == runner->count)
                    {
                        is_equale = 1;
                    }
                }
                if (strcmp(fields[i], "images") == 0)
                {
                    if (current->images == runner->images)
                    {
                        is_equale = 1;
                    }
                }
                if (strcmp(fields[i], "worker") == 0)
                {
                    if (strcmp(current->worker, runner->worker) == 0)
                    {
                        is_equale = 0;
                    }
                }
            }
            if (is_equale == 1)
            {
                break;
            }
            runner = runner->next;
        }

        if (is_equale) {
            LS* to_delete = current;
            if (previous != NULL) {
                previous->next = current->next;
            }
            else {
                Head = current->next; //
            }
            current = current->next;
            free(to_delete);
        }
        else {
            previous = current;
            current = current->next;
        }
    }
}


/* print'ы */
void print_LS(LS* node) // node -> указатель на первый э-нт ЛС
{
    if (node != NULL)
    {
        char comes[11];
        timestamp_to_date(node->comes, comes);
        printf("comes=%s ", comes);
        printf("sender=%s ", node->sender);
        printf("name=%s ", node->name);
        printf("weight=%d ", node->weight);
        printf("count=%d ", node->count);
        printf("images=");
        switch (node->images)
        {
        case fragile:
            printf("fragile ");
            break;
        case toxic:
            printf("toxic ");
            break;
        case perishable:
            printf("perishable ");
            break;
        case acrid:
            printf("acrid ");
            break;
        case inflammable:
            printf("inflammable ");
            break;
        case frozen:
            printf("frozen ");
            break;
        }
        printf("worker=%s", node->worker);
        printf("\n");
    }
    else
    {
        return;
    }
    print_LS(node->next);
}

void print_node_fields(LS* node, char* Fields) // node - Указаель на выводимый элемент fields - поля для вывода через запятую
{
    char* fields = strdup(Fields);
    char* field = strtok(fields, ",");
    while (field != NULL)
    {
        if (strcmp(field, "comes") == 0)
        {
            char comes[11];
            timestamp_to_date(node->comes, comes);
            printf("comes=%s ", comes);
        }
        if (strcmp(field, "sender") == 0)
        {
            printf("sender=%s ", node->sender);
        }
        if (strcmp(field, "name") == 0)
        {
            printf("name=%s ", node->name);
        }
        if (strcmp(field, "weight") == 0)
        {
            printf("weight=%d ", node->weight);
        }
        if (strcmp(field, "count") == 0)
        {
            printf("count=%d ", node->count);
        }
        if (strcmp(field, "images") == 0)
        {
            printf("images=");
            switch (node->images)
            {
            case fragile:
                printf("fragile ");
                break;
            case toxic:
                printf("toxic ");
                break;
            case perishable:
                printf("perishable ");
                break;
            case acrid:
                printf("acrid ");
                break;
            case inflammable:
                printf("inflammable ");
                break;
            case frozen:
                printf("frozen ");
                break;
            }
        }
        if (strcmp(field, "worker") == 0)
        {
            printf("worker=%s ", node->worker);
        }
        field = strtok(NULL, ",");
    }
    printf("\n");
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

        if (!strcmp(str, "print")) {print_LS(Head); }
    }
    // TODO work with terminal
    //print_LS(Head);
    return 1;
}