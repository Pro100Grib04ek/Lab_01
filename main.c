/* 
тут какая-то магия для работы с файлами и памятью.
не трогать! может сломаться!
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// счетчики чтобы следить за тем как память гуляет (спасибо заданию)
size_t malloc_счетчик = 0;
size_t realloc_счетчик = 0;
size_t calloc_счетчик = 0;
size_t free_счетчик = 0;

// максимальные размеры чтобы ничего не ломалось (надеюсь)
#define MAX_NAME_LEN 100
#define FILL(node, comes, sender, name, weight, count, images, worker) {\
    strcpy(node->comes, comes);\
    strcpy(node->sender, sender);\
    strcpy(node->name, name);\
    node->weight =  weight;\
    node->count = count;\
    strcpy(node->worker, worker);\
}

// типы картинок (не спрашивайте почему так)
typedef enum Images { fragile, toxic, perishable, acrid, inflammable, frozen };

// главная структура где всё хранится
typedef struct
{
    char comes[11];        // когда пришло (формат какой-то)
    char sender[MAX_NAME_LEN]; // кто прислал (название)
    char name[MAX_NAME_LEN];   // как обозвать (тип товара)
    int weight;            // тяжелая штука (в кг)
    int count;             // сколько их тут валяется
    enum Images images;    // картинка для красоты
    char worker[MAX_NAME_LEN]; // кто принял (фамилия)
    struct LS* next;       // ссылка на соседа в цепочке
} LS;

// пытается добавить новый элемент в цепочку
void add_node(LS* node, char comes[11], char sender[MAX_NAME_LEN], char name[MAX_NAME_LEN], \
            int weight, int count, enum Images images, char worker[MAX_NAME_LEN]) 
{
    if (node == NULL) // если список пустой, делаем первый элемент
    {
        LS* new_node = (LS*)malloc(sizeof(LS));
        FILL(new_node, comes, sender, name, weight, count, images, worker);
        new_node->next = NULL;
    }
    else // цепляем новую запись в конец
    {
        LS* new_node = (LS*)malloc(sizeof(LS));
        FILL(new_node, comes, sender, name, weight, count, images, worker);
        
        new_node->next = node->next;
        node->next = new_node;
    }
}

// выкидывает элемент из цепочки (освобождает память)
void del_node(LS* node) 
{
    if (node->next != NULL)
    {
        LS* удаляемый = node->next; // запоминаем что удаляем
        node->next = node->next->next; // перецепляем ссылки
        free(удаляемый); // освобождаем память (не забыть!)
    }
    else
    {
        printf("тут нечего удалять, сорян"); 
    }
}

// ищет что-то в списке по непонятным условиям
void select()
{
    // TODO: дописать когда-нибудь потом
    return;
}

// пытается что-то обновить в существующих записях
void update()
{
    // TODO: самая сложная часть, оставим на десерт
    return;
}

// главная функция где всё начинается
int main()
{
    LS* голова = NULL; // начало нашей цепочки
    
    // пытаемся прочитать файлик (если он есть)
    FILE* input = fopen("input.txt", "r");
    char строка[1024]; // буфер для команд
    
    while (fgets(строка, sizeof(строка), input) 
    {
        // выкидываем пустые строки (они нам не нужны)
        if (strlen(строка) < 2) continue;
        
        // разбиваем строку на части
        char* команда = strtok(строка, " ");
        if (!команда) continue; // странная строка, пропускаем
        
        if (strcmp(команда, "insert") == 0) 
        {
            // TODO: дописать обработчик вставки
            printf("вставка пока не работает, сорян\n");
        }
        // ... другие команды
    }
    
    // записываем статистику по памяти (чтобы отчитаться)
    FILE* mem = fopen("memstat.txt", "w");
    fprintf(mem, "malloc:%zu\n", malloc_счетчик);
    // ... остальные счетчики
    fclose(mem);
    
    return 0;
}
