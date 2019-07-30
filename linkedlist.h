#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H
/*
 * Project definitions
 */
#include <cmsis_os.h>
#include "memory_i.h"


/**
 * @brief User rights
 * Тип параметра
 *
 **/
typedef enum
{
	paramtype_char=0x0,
	paramtype_int1=0x1,
	paramtype_int4=0x2,
	paramtype_int8=0x3,
	paramtype_float=0x4,
	paramtype_ticks=0x5,
	paramtype_list=0x6,
	paramtype_char_tel =0x7, //телефон
} ParamType;


/*
 * Структура с параметрам узла списка
 * */
typedef struct param_table_item
{
	ParamType paramtype;	// тип пункта
	uint8_t numplaces; //максимальная длина - для строк и чисел, количество пунктов для списков
	uint8_t editable; //редактируемый параметр (1 - допускается  редактировать, 0 - не допускается редактировать)
	unsigned char* paramtitle; 	// название пункта меню
	void* paramitem;		// ссылка на значение
} param_table_item;

/*
 * Структура со значением параметра-списка (типы paramtype_ticks и paramtype_list)
 */
typedef struct param_tick_item
{
	unsigned char* title; 	// название пункта меню
	int value;		// ссылка на значение
} param_tick_item;



typedef struct _NodeItem {
	uint8_t type; //тип
	char* title; //заголовок для вывода
	param_table_item *value; //значение - указатель на структуру с параметрами
} NodeItem;

NodeItem* createNodeItem(uint8_t type, char* title, param_table_item *value);


/**
 * @brief User rights
 * Положения рекламного текста
 *
 **/
enum
{
	top = 0x00,
	bottom = 0x01,
};

/**
 * @brief
 * Скидка/наценка
 *
 **/
enum
{
	disc = 0x00,
	extr = 0x01,
};

/*
 * General definitions
 */
typedef struct _Node {
	NodeItem *value; //значение - _NodeItem
	struct _Node *next;
	struct _Node *prev;
} Node;

typedef struct _DblLinkedList {
	size_t size; // размер списка
	uint8_t editable; //редактируемый список (товары, отделы)
	Node *head;
	Node *tail;
} DblLinkedList;


//Максимальные размеры списков
#define SIZE_LIST_USERS 10
#define SIZE_LIST_DEPARTS 30
#define SIZE_LIST_FIXES FIXCOUNT
#define SIZE_LIST_CASH_TYPES 4
#define SIZE_LIST_DISCOUNTS 6
#define SIZE_LIST_ADVS 16

#define SIZE_LIST_GOODS 5000
#define SIZE_LIST_KLIENTS 10000

//размеры структур (количество параметров)
#define NUM_PARAMS_USER 3
#define NUM_PARAMS_DEPART 2
#define NUM_PARAMS_FIX 4
#define NUM_PARAMS_DISCOUNTS 3
#define NUM_PARAMS_ADVS 3

#define NUM_PARAMS_GOOD 6
#define NUM_PARAMS_KLIENT 8

#define NUM_PARAMS_KKM 6
#define NUM_PARAMS_CASH 1
#define NUM_PARAMS_PRINT 4
#define NUM_PARAMS_DEV 1
#define NUM_PARAMS_DEV_WEIGHTS 4
#define NUM_PARAMS_DEV_TERM 7
#define NUM_PARAMS_CONNECT 11

#define NUM_PARAMS_SVED 4
#define NUM_PARAMS_SVED_USER 10

#define NUM_PARAMS_REC 9 //+2*k - количество позиций


extern DblLinkedList *list_users;
extern DblLinkedList *list_departs;
extern DblLinkedList *list_fixes;
extern DblLinkedList *list_discounts;
extern DblLinkedList *list_advs;
extern DblLinkedList *list_cashtypes;
extern DblLinkedList *list_kkm_options;
extern DblLinkedList *list_print_options;
extern DblLinkedList *list_scanner_options;
extern DblLinkedList *list_weights_options;
extern DblLinkedList *list_moneybox_options;
extern DblLinkedList *list_terminal_options;
extern DblLinkedList *list_connect;
extern DblLinkedList *list_ext_indicator_options;
extern DblLinkedList *list_rfidreader_options;

extern DblLinkedList *list_sved;
extern DblLinkedList *list_1Cconnect;
extern DblLinkedList *list_OFDconnect;
extern DblLinkedList *list_GPRSconnect;
extern DblLinkedList *list_WIFIconnect;
extern DblLinkedList *list_LANconnect;
extern DblLinkedList *list_GeneralNetConnect;
extern DblLinkedList *list_printers;

extern DblLinkedList *list_bank_pay_agent;
extern DblLinkedList *list_bank_pay_subagent;
extern DblLinkedList *list_pay_agent;
extern DblLinkedList *list_pay_subagent;
extern DblLinkedList *list_poverenyy;
extern DblLinkedList *list_komissioner;
extern DblLinkedList *list_agent;

void clearmas();
DblLinkedList* createDblLinkedList(uint8_t editable);
void deleteDblLinkedList(DblLinkedList **list);
void pushFront(DblLinkedList *list, NodeItem *data);
NodeItem* popFront(DblLinkedList *list);
void pushBack(DblLinkedList *list, NodeItem *value);
NodeItem* popBack(DblLinkedList *list);
Node* getNthq(DblLinkedList *list, size_t index);
void insert(DblLinkedList *list, size_t index, NodeItem *value);
NodeItem* deleteNth(DblLinkedList *list, size_t index);
void ShowListUsers();
void ShowListBankAgents();
void ShowListBankSubAgents() ;
void ShowListPayAgents();
void ShowListPaySubAgents();
void ShowListPover();
void ShowListKomiss();
void ShowListOtherAgents();

void ShowListDeparts();
void ShowListGoods();
void ShowListKlients();
uint8_t ShowListGoodsByDeps(uint8_t numdep);
uint8_t ShowNodeGood(uint16_t index, uint8_t numdep) ;
uint8_t ShowNodeKlient(uint16_t index);
uint8_t ShowCreateNodeGood( uint8_t numdep, uint16_t *newgoodpos);
uint8_t ShowNodeRecFromControlLine(DblList *MyR,uint32_t num) ;
uint8_t ShowCreateNodeKlient(uint16_t *newpos);
void drawGood(char *str_header, uint16_t  index, uint16_t listsize, uint8_t numdep);
void drawKlient(char *str_header, uint16_t  index, uint16_t listsize);
void PrintNode(DblLinkedList *list, int num, int numpar);
void PrintAllNodes(DblLinkedList *list,  int numpar) ;
void PrintGood( uint16_t  index, uint8_t numdep) ;
void PrintAllGoods(uint8_t numdep, uint16_t listsize);

void reg_drawGood(char *str_header, uint16_t  index,  uint8_t k, uint16_t listsize, uint8_t numdep) ;
uint8_t reg_ShowListGoods(char *str_header, uint8_t k,
		struct memory_good *mygood);

uint8_t reg_ShowListGoodsByDeps(uint8_t numdep, uint8_t k,
		struct memory_good *mygood);


uint8_t ShowListSved(uint8_t par, uint8_t code, char *regnum, char *inn, char *zavnum);
void ShowListFixes();
void ShowListDiscounts();
void ShowListAdvs();
void ShowListCashTypes();
void ShowListPrinters() ;
void ShowListKKMOptions();
void ShowListPrintOptions();
void ShowListScannerOptions();
void ShowListMoneyBoxOptions();
void ShowListRfidReaderOptions();
void ShowListExtIndicatorOptions() ;
void ShowListTerminalOptions();
void ShowListOnlineSettings();

void ShowListDateTime(DblLinkedList *list);
void InitFix(struct memory_fix *myfix);
void InitDepart(struct memory_depart *mydep) ;
void EditDepartName(char *name, uint8_t depnum);
void EditDepart(char *name, uint8_t fix_type, uint8_t code, uint8_t pr, uint8_t depnum);
uint8_t InitGood(char *name, uint32_t code, char *barcode, int depart, double price, uint8_t flag, uint8_t flag_service);
uint8_t InitKlient(char *name, char *id,uint8_t dics_num, uint32_t disc_proc,uint32_t ext_proc,
		uint64_t disc_val, uint64_t ext_val, uint64_t sum);
void InitUser(char *name, char * pass, uint8_t rights, char *usertitle, int i);
void InitDiscount(struct memory_discounts * mydisc);
void InitCashType(struct memory_cashtypes * mycashtype);
void InitAdvs(struct memory_advs * myadv);
uint8_t ShowNode(DblLinkedList *list, int num, int numpar,int startpar);
uint8_t drawNode(NodeItem *nodeitem, int curnum, int numpar, int startpar);
uint8_t ShowList(DblLinkedList *list, char *title, int numpar);
void drawnodeitem(DblLinkedList *list, char *title, int curnum);
void drawnodeitem_pos(DblLinkedList *list, char *title, int curnum, uint8_t numpos) ;
char *get_cash_type_name(uint8_t cashtype);

#endif
